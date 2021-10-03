#pragma once
#include "pch.h"

static std::string StringFormat(const char* str, va_list va) {
	//The size returned by _vsnprintf does NOT include null terminator
	int size = _vsnprintf(nullptr, 0U, str, va);
	std::string res;
	if (size > 0) {
		res.resize(size + 1);
		_vsnprintf((char*)res.c_str(), res.size(), str, va);
		res.pop_back();	//Don't include the null terminator
	}
	return res;
}
static std::string StringFormat(const char* str, ...) {
	va_list	vl;
	va_start(vl, str);
	std::string res = StringFormat(str, vl);
	va_end(vl);
	return res;
}