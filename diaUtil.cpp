#include "diaUtil.h"

#include <string>
#include <sstream>

std::string dia_error_string(HRESULT error_code) {
    switch (error_code) {
        case E_PDB_NOT_FOUND:
            return "E_PDB_NOT_FOUND";
        case E_PDB_FORMAT:
            return "E_PDB_FORMAT";
        case E_INVALIDARG:
            return "Invalid parameter";
        case E_UNEXPECTED:
            return "Data source has already been prepared";
        case E_PDB_INVALID_SIG:
            return "Debug info mismatch (invalid signature)";
        case E_PDB_INVALID_AGE:
            return "Debug info mismatch (invalid age)";
        default: {
            std::stringstream ss;
            ss << "Unknown HRESULT: 0x" << std::hex << error_code;
            return ss.str();
        }
    }
}
