#include "pch.h"

#include "Archive.hpp"

//Raw dump
void Archive::_DumpChunk_Raw(const std::string& dirOut, Chunk* pChunk) {
	std::ofstream outFile;
	outFile.open(dirOut + pChunk->name + ".dat", std::ios::binary);

	try {
		if (!outFile.is_open())
			throw std::string("Cannot open file for writing");

		outFile.write((char*)pChunk->data.data(), pChunk->data.size());

		printf("Success\n");
	}
	catch (std::string e) {
		std::string err = "Failure: ";
		err += e + "\n";
		printf(err.c_str());
	}

	outFile.close();
}

//Dump textures
void Archive::_DumpChunk_TXTR(const std::string& dirOut, Chunk* pChunk) {
	stdfs::create_directories(dirOut);

	try {
		byte* pMemOrg = pChunk->data.data();
		byte* pMem = pMemOrg;

		size_t chunkSize = pChunk->data.size();
		size_t chunkOff = pChunk->offset + 8;

		auto _VerifySize = [&](size_t size) {
			size_t cOff = pMem - pMemOrg;
			if (cOff + size > chunkSize)
				throw std::string("Invalid chunk size; archive may be corrupted");
		};
		_VerifySize(sizeof(uint32_t));

		size_t texCount = *(uint32_t*)(pMem);
		pMem += sizeof(uint32_t);

		if (texCount > 0) {
			struct _TextureInfo {
				uint32_t unk1;
				uint32_t unk2;
				uint32_t pos;
			};
			struct TextureInfo {
				_TextureInfo t;
				uint32_t texOff;
				uint32_t size;
			};

			std::vector<uint32_t> listPosTexInfo;
			listPosTexInfo.resize(texCount);

			std::vector<TextureInfo> listTexInfo;
			listTexInfo.resize(texCount);

			_VerifySize(sizeof(uint32_t) * texCount);
			for (size_t i = 0; i < texCount; ++i) {
				listPosTexInfo[i] = *(uint32_t*)(pMem);
				pMem += sizeof(uint32_t);
			}

			{
				_VerifySize(sizeof(_TextureInfo) * texCount);
				for (size_t i = 0; i < texCount; ++i) {
					TextureInfo texInfo;
					texInfo.size = 0;

					size_t infoOff = listPosTexInfo[i] - chunkOff;
					pMem = pMemOrg + infoOff;
					memcpy(&(texInfo.t), pMem, sizeof(_TextureInfo));

					texInfo.texOff = texInfo.t.pos - chunkOff;
					listTexInfo[i] = texInfo;
				}

				//Determine the sizes of each textures
				for (size_t i = 0; i < texCount; ++i) {
					TextureInfo* cInfo = &listTexInfo[i];

					size_t texSize = 0;
					if (i != texCount - 1) {
						TextureInfo* nInfo = &listTexInfo[i + 1];
						texSize = nInfo->t.pos - cInfo->t.pos;
					}
					else {
						texSize = chunkSize - cInfo->texOff;
					}

					pMem = pMemOrg + cInfo->texOff;
					_VerifySize(texSize);

					cInfo->size = texSize;
				}

				for (size_t i = 0; i < texCount; ++i) {
					printf("   Dumping texture %u...", i);

					TextureInfo* pInfo = &listTexInfo[i];

					std::ofstream outFile;

					try {
						outFile.open(dirOut + std::to_string(i) + ".png", std::ios::binary);
						if (!outFile.is_open())
							throw std::string("Cannot open file for writing");

						pMem = pMemOrg + pInfo->texOff;
						outFile.write((char*)pMem, pInfo->size);

						printf(outFile.good() ? "Success\n" : "Failure\n");
					}
					catch (std::string e) {
						std::string err = e + "\n";
						printf(err.c_str());
					}

					outFile.close();
				}
			}
		}
	}
	catch (std::string e) {
		std::string err = "Failed to dump chunk \"TXTR\": ";
		err += e + "\n";
		printf(err.c_str());
	}
}

//Dump audio files
void Archive::_DumpChunk_AUDO(const std::string& dirOut, Chunk* pChunk) {
	stdfs::create_directories(dirOut);

	try {
		byte* pMemOrg = pChunk->data.data();
		byte* pMem = pMemOrg;

		size_t chunkSize = pChunk->data.size();
		size_t chunkOff = pChunk->offset + 8;

		auto _VerifySize = [&](size_t size) {
			size_t cOff = pMem - pMemOrg;
			if (cOff + size > chunkSize)
				throw std::string("Invalid chunk size; archive may be corrupted");
		};
		_VerifySize(sizeof(uint32_t));

		size_t audCount = *(uint32_t*)(pMem);
		pMem += sizeof(uint32_t);

		if (audCount > 0) {
			struct AudioInfo {
				uint32_t off;
				uint32_t size;
				std::string format;
			};

			std::vector<AudioInfo> listAudio;
			listAudio.resize(audCount);

			_VerifySize(sizeof(uint32_t) * audCount);
			for (size_t i = 0; i < audCount; ++i) {
				listAudio[i].off = *(uint32_t*)(pMem);
				pMem += sizeof(uint32_t);
			}

			{
				{
					//Load sizes and determine formats
					for (size_t i = 0; i < audCount; ++i) {
						AudioInfo* pInfo = &listAudio[i];

						uint32_t audOff = pInfo->off - chunkOff;

						pMem = pMemOrg + audOff;
						_VerifySize(sizeof(uint32_t));

						pInfo->size = *(uint32_t*)(pMem);
						pMem += 4;
						_VerifySize(pInfo->size);

						if (memcmp(pMem, "RIFF", 4) == 0)
							pInfo->format = "wav";
						else if (memcmp(pMem, "OggS", 4) == 0)
							pInfo->format = "ogg";
						else
							pInfo->format = "mp3";
					}
				}

				for (size_t i = 0; i < audCount; ++i) {
					printf("   Dumping audio ");

					AudioInfo* pInfo = &listAudio[i];
					uint32_t audOff = pInfo->off - chunkOff;

					std::ofstream outFile;

					try {
						std::string fileName = std::to_string(i) + "." + pInfo->format;
						printf("%s...", fileName.c_str());

						outFile.open(dirOut + fileName, std::ios::binary);
						if (!outFile.is_open())
							throw std::string("Cannot open file for writing");

						pMem = pMemOrg + audOff + sizeof(uint32_t);
						outFile.write((char*)pMem, pInfo->size);

						printf(outFile.good() ? "Success\n" : "Failure\n");
					}
					catch (std::string e) {
						std::string err = e + "\n";
						printf(err.c_str());
					}

					outFile.close();
				}
			}
		}
	}
	catch (std::string e) {
		std::string err = "Failed to dump chunk \"TXTR\": ";
		err += e + "\n";
		printf(err.c_str());
	}
}