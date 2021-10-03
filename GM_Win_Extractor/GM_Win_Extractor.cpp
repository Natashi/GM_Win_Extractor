#include "pch.h"

#include "Archive.hpp"

int main(int argc, char** argv) {
	if (argc != 3) {
		printf("Usage: [.win file] [output folder]");
		return 0;
	}

	try {
		Archive archive;

		if (archive.Load(argv[1])) {
			std::string baseOut = argv[2];
			if (baseOut.back() != '\\' && baseOut.back() != '/')
				baseOut.push_back('\\');

			archive.Dump(baseOut);
		}
	}
	catch (std::string e) {
		printf("Error: %s", e.c_str());
	}

	return 0;
}