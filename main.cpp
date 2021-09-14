#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>

#include "Windows.h"

#include <atlbase.h>
#include "dia2.h"
#include "diacreate.h"
#include <ImageHlp.h>

#include "dia_callback.h"
#include "symsrv_callback.h"
#include "util.h"

const char* dbghelp_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\dbghelp.dll)";
const char* libdia_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\msdia140.dll)";
const char* symsrv_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\symsrv_last_version\symsrv.dll)";

const char* NT_SYMBOL_PATH = "_NT_SYMBOL_PATH";

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

static std::runtime_error dia_exception(const std::string &message, const HRESULT HR) {
    return std::runtime_error(message + ": " + dia_error_string(HR));
}

class DiaLibraryWrapper {
public:
    DiaLibraryWrapper(const std::string &library_path) {
        if (!PathFileExistsA(library_path.c_str())) {
            throw std::runtime_error("do_load_pdb, no such file: " + library_path);
        }
        _library_path = library_path;

        HRESULT HR;
        if (FAILED(HR = NoRegCoCreate(msdia_dll, CLSID_DiaSource, IID_IDiaDataSource,
                                      reinterpret_cast<LPVOID *>(&_diaDataSource)))) {
            throw dia_exception("NoRegCoCreate", HR);
        }

        if (FAILED(HR = _diaDataSource->loadDataForExe(to_wstring(library_path).c_str(), nullptr, &libdiaCallback))) {
            throw dia_exception("loadDataForExe('" + library_path + "')", HR);
        }

        if (FAILED(HR = _diaDataSource->openSession(&_diaSession))) {
            throw dia_exception("openSession", HR);
        }
    }

    bool do_load_pdb() {
        HRESULT HR;
        CComPtr<IDiaSymbol> pSymbol;
        if (FAILED(HR = _diaSession->get_globalScope(&pSymbol))) {
            throw dia_exception("can't DiaSession->get_globalScope", HR);
        } else {
            GUID pdbGuid;
            if (FAILED(HR = pSymbol->get_guid(&pdbGuid))) {
                throw dia_exception("can't pSymbol->get_guid", HR);
            } else {
                OLECHAR szGUID[64] = {0};
                StringFromGUID2(pdbGuid, szGUID, 64);
                auto guid_str = guid_to_string(&pdbGuid);
                fprintf(stderr, "library: %s, GUID: %s\n", _library_path.c_str(), guid_str.c_str());
            }
        }
        return true;
    }


private:
    const static constexpr wchar_t *msdia_dll = L"msdia140.dll";
    std::string _library_path;
    CComPtr<IDiaDataSource> _diaDataSource;
    CComPtr<IDiaSession> _diaSession;
};

HMODULE load_libdia() {
    HMODULE loaded_libdia = LoadLibraryA(libdia_path);
    if (!loaded_libdia) {
        fprintf(stderr, "can't load libdia, error code: %lu\n", GetLastError());
        return nullptr;
    }
    return loaded_libdia;
}

HMODULE load_dbghelp() {
    HMODULE loaded_dbghelp = LoadLibraryA(dbghelp_path);
    if (!loaded_dbghelp) {
        fprintf(stderr, "can't load dbghelp, error code: %lu\n", GetLastError());
        return nullptr;
    }
    return loaded_dbghelp;
}

HMODULE load_symsrv() {
    HMODULE loaded_symsrv = LoadLibraryA(symsrv_path);
    if (!loaded_symsrv) {
        fprintf(stderr, "can't load symsrv, error code: %lu\n", GetLastError());
        return nullptr;
    }
    return loaded_symsrv;
}

// see https://jetbrains.team/p/llvm/repositories/llvm-project/files/6f6d0a12e1260e0fd79fbcd1f5df2ebb48002db8/lldb/source/Plugins/SymbolFile/PDB/SymbolFilePDB.cpp?tab=source&line=436
bool setup_symsrv(HMODULE symSrvMod) {
    if (!symSrvMod) {
        fprintf(stderr, "can't LoadLibraryA for symsrv: %lu", GetLastError());
        return false;
    }

    PSYMBOLSERVERSETOPTIONSPROC m_sym_srv_set_opt = reinterpret_cast<PSYMBOLSERVERSETOPTIONSPROC>(
            GetProcAddress(symSrvMod, "SymbolServerSetOptions"));
    if (!m_sym_srv_set_opt) {
        fprintf(stderr, "Can't find `SymbolServerSetOptions` function: %lu", GetLastError());
        return false;
    }

    if (!m_sym_srv_set_opt(SSRVOPT_CALLBACK, reinterpret_cast<ULONG64>(&symbolservercallback))) {
        return false;
    }
    return true;
}

void do_print_nt_symbol_path() {
    char* buffer = new char[1024];
    DWORD result = GetEnvironmentVariable(NT_SYMBOL_PATH, buffer, 1024);
    if (result == 0) {
        fprintf(stderr, "can't GetEnvironmentVariable: %lu\n", GetLastError());
        return;
    }
    fprintf(stderr, "_NT_SYMBOL_PATH: '%s'\n", buffer);
    delete[] buffer;
}

bool do_append_nt_symbol_path(const std::string& value) {
    char* buffer = new char[1024];
    DWORD result = GetEnvironmentVariable(NT_SYMBOL_PATH, buffer, 1024);
    if (result == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_ENVVAR_NOT_FOUND) {
            fprintf(stderr, "can't GetEnvironmentVariable: %lu\n", err);
            return false;
        }
    }
    std::string new_value = std::string(buffer) + ";" + value;
    return SetEnvironmentVariable(NT_SYMBOL_PATH, new_value.c_str());
}

const std::string bin_server_jvm_dll = "bin\\server\\jvm.dll";

const std::string jvmdll_symbols_same_folder = R"(C:\Users\Kirill.Timofeev\Downloads\openjdk-jdk17-windows-x86_64-server-release\jdk\)" + bin_server_jvm_dll;
const std::string jvmdll_no_symbol = "C:\\Users\\Kirill.Timofeev\\.jdks\\corretto-1.8.0_302\\jre\\" + bin_server_jvm_dll;

// the same file located in: data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
// C:\Users\Kirill.Timofeev\Work\get-symbols-playground\data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
const std::string jbr_symbols_on_server = "C:\\Users\\Kirill.Timofeev\\AppData\\Local\\JetBrains\\Toolbox\\apps\\IDEA-U\\ch-0\\213.3358\\jbr\\" + bin_server_jvm_dll;

const std::string ntdll_local_symsrv_lookup = R"(C:\Windows\SYSTEM32\ntdll.dll)";

void do_load_pdb_for_file(const std::string& library_path) {
    DiaLibraryWrapper _dia_wrapper(library_path);
    _dia_wrapper.do_load_pdb();
    CoUninitialize();
}

int main() {
    if (!do_append_nt_symbol_path("srv*C:\\\\JBRSymbols*https://resources.jetbrains.com/pdb")) return -3;
    do_print_nt_symbol_path();

    if (!load_libdia()) return -1;
    // doesn't affect events sent to `symbolservercallback`
    // if (!load_dbghelp()) return -1;
    if (!setup_symsrv(load_symsrv())) return -2;

    try {
        do_load_pdb_for_file(jvmdll_symbols_same_folder);
        //do_load_pdb_for_file(jvmdll_no_symbol);
        do_load_pdb_for_file(jbr_symbols_on_server);
        do_load_pdb_for_file(ntdll_local_symsrv_lookup);
    } catch (const std::runtime_error &ex) {
        fprintf(stderr, "%s", ex.what());
    }

    return 0;
}
