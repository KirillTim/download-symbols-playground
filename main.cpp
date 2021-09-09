#include <iostream>
#include <string>
#include <sstream>

#include "Windows.h"

#include <atlbase.h>
#include "dia2.h"
#include "diacreate.h"
#include <ImageHlp.h>

#include "dia_callback.h"
#include "symsrv_callback.h"
#include "util.h"

const char* libdia_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\msdia140.dll)";
const char* symsrv_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\symsrv.dll)";

CComPtr<IDiaDataSource> DiaDataSource;
CComPtr<IDiaSession> DiaSession;

std::string dia_error_string(HRESULT error_code) {
    switch (error_code) {
        case E_PDB_NOT_FOUND:
            return "Can't open file";
        case E_PDB_FORMAT:
            return "Invalid file format";
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


bool load_libdia_and_symsrv() {
    HMODULE loaded_libdia = LoadLibraryA(libdia_path);
    if (!loaded_libdia) {
        fprintf(stderr, "can't load libdia, error code: %lu\n", GetLastError());
        return false;
    }
    HMODULE loaded_symsrv = LoadLibraryA(symsrv_path);
    if (!loaded_symsrv) {
        fprintf(stderr, "can't load symsrv, error code: %lu\n", GetLastError());
        return false;
    }

    HRESULT HR;
    const wchar_t *msdia_dll = L"msdia140.dll";
    if (FAILED(HR = NoRegCoCreate(msdia_dll, CLSID_DiaSource, IID_IDiaDataSource,
                                  reinterpret_cast<LPVOID *>(&DiaDataSource)))) {
        fprintf(stderr, "NoRegCoCreate: %s\n", dia_error_string(HR).c_str());
        return false;
    }

    return true;
}

// see https://jetbrains.team/p/llvm/repositories/llvm-project/files/6f6d0a12e1260e0fd79fbcd1f5df2ebb48002db8/lldb/source/Plugins/SymbolFile/PDB/SymbolFilePDB.cpp?tab=source&line=436
bool setup_symsrv() {
    // already loaded in `load_libdia_and_symsrv` actually, only need a handle
    auto symSrvMod = LoadLibraryA(symsrv_path);
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

    m_sym_srv_set_opt(SSRVOPT_TRACE, TRUE);
    m_sym_srv_set_opt(SSRVOPT_UNATTENDED, TRUE);
    m_sym_srv_set_opt(SSRVOPT_CALLBACK, reinterpret_cast<ULONG64>(&symbolservercallback));
    return true;
}

bool do_load_pdb(const std::string &library_path) {
    if (!PathFileExistsA(library_path.c_str())) {
        fprintf(stderr, "do_load_pdb, no such file %s\n", library_path.c_str());
        return false;
    }
    fprintf(stderr, "do_load_pdb: %s\n", library_path.c_str());
    HRESULT HR;
    if (FAILED(HR = DiaDataSource->loadDataForExe(to_wstring(library_path).c_str(), nullptr, &libdiaCallback))) {
        fprintf(stderr, "loadDataForExe('%s'): %s\n", library_path.c_str(), dia_error_string(HR).c_str());
        DiaDataSource.Release();
        CoUninitialize();
        return false;
    }

    if (FAILED(HR = DiaDataSource->openSession(&DiaSession))) {
        fprintf(stderr, "openSession: %s\n", dia_error_string(HR).c_str());
        DiaDataSource.Release();
        CoUninitialize();
        return false;
    }

    CComPtr<IDiaSymbol> pSymbol;
    if (FAILED(HR = DiaSession->get_globalScope(&pSymbol))) {
        fprintf(stderr, "can't DiaSession->get_globalScope: %s\n", dia_error_string(HR).c_str());
    } else {
        GUID pdbGuid;
        if (FAILED(HR = pSymbol->get_guid(&pdbGuid))) {
            fprintf(stderr, "can't pSymbol->get_guid: %s\n", dia_error_string(HR).c_str());
        } else {
            OLECHAR szGUID[64] = { 0 };
            StringFromGUID2(pdbGuid, szGUID, 64);
            auto guid_str = guid_to_string(&pdbGuid);
            fprintf(stderr, "library: %s, GUID: %s\n", library_path.c_str(), guid_str.c_str());
        }
    }

    return true;
}

void do_print_nt_symbol_path() {
    char* buffer = new char[1024];
    DWORD result = GetEnvironmentVariable("_NT_SYMBOL_PATH", buffer, 1024);
    if (result == 0) {
        fprintf(stderr, "can't GetEnvironmentVariable: %lu\n", GetLastError());
        return;
    }
    fprintf(stderr, "_NT_SYMBOL_PATH: '%s'\n", buffer);
    delete[] buffer;
}

const std::string bin_server_jvm_dll = "bin\\server\\jvm.dll";

const std::string jvmdll_symbols_same_folder = R"(C:\Users\Kirill.Timofeev\Downloads\openjdk-jdk17-windows-x86_64-server-release\jdk\)" + bin_server_jvm_dll;
const std::string jvmdll_no_symbol = "C:\\Users\\Kirill.Timofeev\\.jdks\\corretto-1.8.0_302\\jre\\" + bin_server_jvm_dll;

const std::string jbr_symbols_on_server = "C:\\Users\\Kirill.Timofeev\\AppData\\Local\\JetBrains\\Toolbox\\apps\\IDEA-U\\ch-0\\213.3358\\jbr\\";

const std::string ntdll_local_symsrv_lookup = R"(C:\Windows\SYSTEM32\ntdll.dll)";

int main() {
    if (!load_libdia_and_symsrv()) return -1;
    if (!setup_symsrv()) return -2;

    do_print_nt_symbol_path();


    if(!do_load_pdb(jvmdll_symbols_same_folder)) return -3;
    if(!do_load_pdb(jvmdll_no_symbol)) return -3;
    //if(!do_load_pdb(ntdll_local_symsrv_lookup)) return -3;

    return 0;
}
