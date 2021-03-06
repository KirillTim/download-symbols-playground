#ifndef GET_SYMBOLS_PLAYGROUND_SYMSRV_CALLBACK_H
#define GET_SYMBOLS_PLAYGROUND_SYMSRV_CALLBACK_H

#include <cstdio>
#include <string>
#include "util.h"

BOOL symbolservercallback(
        UINT_PTR action,
        ULONG64 data,
        ULONG64 context
) {
    if (action == SSRVACTION_XMLOUTPUT) {
        std::string text = to_string(std::wstring(reinterpret_cast<wchar_t *>(data)));
        if (text.find("<Progress percent=") == std::string::npos) {
            // ignore download progress messages
            // TODO: collect output, log if resolve failed
            // fprintf(stderr, "SSRVACTION_XMLOUTPUT: %s\n", text.c_str());
        }
        return TRUE;
    }
    return FALSE;
}

#endif //GET_SYMBOLS_PLAYGROUND_SYMSRV_CALLBACK_H
