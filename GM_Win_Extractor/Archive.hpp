#pragma once
#include "pch.h"

#include "Util.hpp"

struct Chunk {
	std::string name;
	uint64_t offset;
	std::vector<byte> data;
};

class Archive {
public:
	std::ifstream file_;

	uint64_t fileSize_;
	uint32_t format_;

	std::unordered_map<std::string, Chunk*> listChunks_;
public:
	Archive();
	~Archive();

	bool Load(const std::string& path);
	bool Dump(const std::string& baseOut);
private:
	void _DumpChunk_Regular(const std::string& dirOut, Chunk* pChunk);
	void _DumpChunk_TXTR(const std::string& dirOut, Chunk* pChunk);
};