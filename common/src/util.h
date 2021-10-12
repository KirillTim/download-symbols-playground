#ifndef _UTIL_H
#define _UTIL_H

#include <string>
#include <Windows.h>

std::wstring to_wstring(const std::string &str);

std::string to_string(const std::wstring &wstr);

#endif //_UTIL_H
