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
		{ "AUDO", &Archive::_DumpChunk_AUDO },
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
			_DumpChunk_Raw(baseOut, pChunk);
		}
	}

	return true;
}