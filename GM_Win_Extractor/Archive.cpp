#include "pch.h"

#include "Archive.hpp"

Archive::Archive() {
	fileSize_ = 0;
	format_ = 0;
}
Archive::~Archive() {
	file_.close();
	for (auto& i : listChunks_)
		ptr_delete(i.second);
}

bool Archive::Load(const std::string& path) {
	file_.open(path, std::ios::binary);
	if (!file_.is_open())
		throw std::string("Failed to open file.");

	file_.seekg(0, std::ios::end);
	fileSize_ = file_.tellg();
	file_.seekg(0, std::ios::beg);

	auto _VerifySize = [&](uint64_t size) {
		uint64_t cOff = file_.tellg();
		if (cOff + size > fileSize_)
			throw std::string("Invalid chunk size; archive may be corrupted");
	};

	char tmpBuf[64];

	{
		_VerifySize(8);
		file_.read(tmpBuf, 8);

		if (memcmp(tmpBuf, "FORM", 4) != 0)
			throw std::string("Unsupported format.");
		format_ = ((uint32_t*)tmpBuf)[1];
	}
	
	while (true) {
		_VerifySize(8);
		file_.read(tmpBuf, 8);

		Chunk* pChunk = new Chunk;

		pChunk->offset = (uint64_t)file_.tellg() - 8;

		pChunk->name.resize(4);
		memcpy(pChunk->name.data(), tmpBuf, 4);

		listChunks_[pChunk->name] = pChunk;

		size_t chunkSize = ((uint32_t*)tmpBuf)[1];
		_VerifySize(chunkSize);
		
		pChunk->data.resize(chunkSize);
		file_.read((char*)pChunk->data.data(), chunkSize);

		if (file_.eof() || file_.tellg() >= fileSize_) break;
	}

	printf("Successfully read %u chunks\n\n", listChunks_.size());

	file_.clear();
	return true;
}
bool Archive::Dump(const std::string& baseOut) {
	stdfs::create_directories(baseOut);

	static std::map<std::string, void (Archive::*)(const std::string&, Chunk*)> mapDumper = {
		{ "TXTR", &Archive::_DumpChunk_TXTR },
	};

	for (auto itrChunk = listChunks_.begin(); itrChunk != listChunks_.end(); ++itrChunk) {
		const std::string& nameChunk = itrChunk->first;
		Chunk* pChunk = itrChunk->second;

		printf("Dumping chunk \"%s\"...\n", nameChunk.c_str());

		auto itrFuncFind = mapDumper.find(nameChunk);
		if (itrFuncFind != mapDumper.end()) {
			std::string outPath = baseOut + nameChunk + "\\";
			(this->*(itrFuncFind->second))(outPath, pChunk);
		}
		else {
			//_DumpChunk_Regular(baseOut, pChunk);
		}
	}

	return true;
}

void Archive::_DumpChunk_Regular(const std::string& dirOut, Chunk* pChunk) {
	std::ofstream outFile;
	outFile.open(dirOut + pChunk->name + ".dat", std::ios::binary);

	try {
		if (!outFile.is_open())
			throw std::string("Cannot open file for writing");

		outFile.write((char*)pChunk->data.data(), pChunk->data.size());

		printf("Dumped chunk \"%s\"\n", pChunk->name.c_str());
	}
	catch (std::string e) {
		std::string err = StringFormat("Failed to dump chunk \"%s\": ", pChunk->name.c_str());
		err += e + "\n";
		printf(err.c_str());
	}

	outFile.close();
}
void Archive::_DumpChunk_TXTR(const std::string& dirOut, Chunk* pChunk) {
	//TXTR chunk = textures

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