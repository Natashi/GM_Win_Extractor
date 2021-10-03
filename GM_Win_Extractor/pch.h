#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include <cstdarg>

#include <list>
#include <vector>
#include <map>
#include <unordered_map>

#include <filesystem>
namespace stdfs = std::filesystem;

typedef unsigned char byte;

template<typename T> static constexpr inline void ptr_delete(T*& ptr) {
	if (ptr) delete ptr;
	ptr = nullptr;
}
template<typename T> static constexpr inline void ptr_delete_scalar(T*& ptr) {
	if (ptr) delete[] ptr;
	ptr = nullptr;
}