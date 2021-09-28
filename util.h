#ifndef GET_SYMBOLS_PLAYGROUND_UTIL_H
#define GET_SYMBOLS_PLAYGROUND_UTIL_H

#include <codecvt>

std::wstring to_wstring(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

std::string to_string(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
    return conv.to_bytes(wstr);
}

// https://stackoverflow.com/questions/1672677/print-a-guid-variable/1672698
static std::string guid_to_string(GUID* guid)
{
    char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator

    snprintf(guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
             "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2],
             guid->Data4[3], guid->Data4[4], guid->Data4[5],
             guid->Data4[6], guid->Data4[7]);

    return guid_string;
}


#endif //GET_SYMBOLS_PLAYGROUND_UTIL_H
