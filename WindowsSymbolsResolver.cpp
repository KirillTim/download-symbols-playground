#include "WindowsSymbolsResolver.h"

#include <string>

#include "Windows.h"
#include <atlbase.h>
#include "dia2.h"
#include "diacreate.h"
#include <ImageHlp.h>

#include "diaUtil.h"
#include "symsrv_callback.h"

std::mutex WindowsSymbolsResolver::mutex;
HMODULE WindowsSymbolsResolver::libDia = nullptr;
HMODULE WindowsSymbolsResolver::libSymsrv = nullptr;

const static constexpr char *NT_SYMBOL_PATH = "_NT_SYMBOL_PATH";
const static constexpr wchar_t *msdia_dll = L"msdia140.dll";

void do_print_nt_symbol_path() {
    char *buffer = new char[1024];
    DWORD result = GetEnvironmentVariable(NT_SYMBOL_PATH, buffer, 1024);
    if (result == 0) {
        fprintf(stderr, "can't GetEnvironmentVariable: %lu\n", GetLastError());
        return;
    }
    fprintf(stderr, "_NT_SYMBOL_PATH: '%s'\n", buffer);
    delete[] buffer;
}

class NtSymbolPathHolder {
public:
    bool init() {
        char* buffer = new char[2048];
        DWORD result = GetEnvironmentVariable(NT_SYMBOL_PATH, buffer, 2048);
        if (result == 0) {
            DWORD err = GetLastError();
            if (err != ERROR_ENVVAR_NOT_FOUND) {
                fprintf(stderr, "can't GetEnvironmentVariable: %lu\n", err);
                return false;
            }
        }
        original_value = std::string(buffer);
        return true;
    }

    bool append(const std::string &value) {
        std::string new_value;
        if (original_value.empty()) new_value = value;
        else new_value = original_value + ";" + value;
        return SetEnvironmentVariable(NT_SYMBOL_PATH, new_value.c_str());
    }

    ~NtSymbolPathHolder() {
        SetEnvironmentVariable(NT_SYMBOL_PATH, original_value.c_str());
    }
private:
    std::string original_value;
};

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

static std::string reportError(const std::string &errMessage) {
    return "Error: " + errMessage;
}

static std::string reportDiaError(const std::string &message, const HRESULT HR) {
    return message + ": " + dia_error_string(HR);
}

struct DiaScopedHolder {
    CComPtr<IDiaDataSource> _diaDataSource;
    CComPtr<IDiaSession> _diaSession;
    CComPtr<IDiaSymbol> _diaSymbol;

    virtual ~DiaScopedHolder() {
        CoUninitialize();
    }
};

std::string getName(const CComPtr<IDiaSymbol> &_diaSymbol) {
    BSTR bstr_function_name;
    std::string function_name;
    if (_diaSymbol->get_undecoratedNameEx(UNDECORATE_OPTIONS, &bstr_function_name) != S_OK) {
        if (_diaSymbol->get_name(&bstr_function_name) != S_OK) {
            //std::cerr << "failed to get function name for 0x" << std::hex << rva << std::dec << std::endl;
            SysFreeString(bstr_function_name);
            return "";
        }
    }
    function_name = to_string(std::wstring(bstr_function_name));
    SysFreeString(bstr_function_name);
    return function_name;
}

std::variant<std::string, std::vector<ResolvedSymbol>>
WindowsSymbolsResolver::doResolve(const std::string &_libDiaPath, const std::string &_libSymsrvPath,
                                  const std::string &_additionalNtLibraryPath, const std::string &_dllPath,
                                  const std::vector<DWORD> &_offsets) {

    {
        std::lock_guard guard(mutex);
        if (!libDia && !_libDiaPath.empty()) {
            // libdia could already be loaded (e.g: in `WindowsEngine::start`)
            libDia = LoadLibraryA(_libDiaPath.c_str());
            if (!libDia) {
                return reportError(
                        "can't load libdia from: `" + _libDiaPath + "`, error code: " + std::to_string(GetLastError()));
            }
        }

        fprintf(stderr, "_additionalNtLibraryPath: %s\n", _additionalNtLibraryPath.c_str());
        NtSymbolPathHolder ntSymbolPathHolder;
        if (!_additionalNtLibraryPath.empty()) {
            if (!ntSymbolPathHolder.init()) return reportError("Can't read `NT_SYMBOL_PATH`");
            if (!ntSymbolPathHolder.append(_additionalNtLibraryPath)) {
                reportError("Can't append to `NT_SYMBOL_PATH`");
            }
        }
        do_print_nt_symbol_path();

        if (!libSymsrv && !_libSymsrvPath.empty()) {
            libSymsrv = LoadLibraryA(_libSymsrvPath.c_str());
            if (!libSymsrv) {
                return reportError("can't load libsymsrv, error code: " + std::to_string(GetLastError()));
            }
            if (!setup_symsrv(libSymsrv)) {
                return reportError("can't setup symsrv");
            }
        }
    }
    if (_dllPath.empty()) return reportError("dllPath is empty");
    if (!PathFileExistsA(_dllPath.c_str())) return reportError("no such file: " + _dllPath);
    // if (_offsets.empty()) return reportError("offsets array is empty");

    DiaScopedHolder h;
    HRESULT HR;
    if (FAILED(HR = NoRegCoCreate(msdia_dll, CLSID_DiaSource, IID_IDiaDataSource,
                                  reinterpret_cast<LPVOID *>(&h._diaDataSource)))) {
        return reportDiaError("NoRegCoCreate", HR);
    }
    DiaAllowEverythingSettingsCallBack libdiaCallback;
    if (FAILED(HR = h._diaDataSource->loadDataForExe(to_wstring(_dllPath).c_str(), nullptr, &libdiaCallback))) {
        return reportDiaError("loadDataForExe('" + _dllPath + "')", HR);
    }
    if (FAILED(HR = h._diaDataSource->openSession(&h._diaSession))) {
        return reportDiaError("openSession", HR);
    }

    std::vector<ResolvedSymbol> symbols;
    for (const DWORD rva: _offsets) {
        LONG displacement = -1;
        ResolvedSymbol resolved;
        h._diaSymbol.Release();
        // FIXME: multiple symbols can match the same function. sort them and check forward after each pdb lookup
        if (h._diaSession->findSymbolByRVAEx(rva, SymTagFunction, &h._diaSymbol, &displacement) != S_OK) {
            //std::cerr << "DiaSession->findSymbolByVAEx(0x" << std::hex << rva << std::dec << "): " << dia_error_string(HR) << std::endl;
            resolved = ResolvedSymbol(_dllPath, rva);
        } else {
            std::string function_name = getName(h._diaSymbol);
            if (function_name.empty()) {
                resolved = ResolvedSymbol(_dllPath, rva);
            } else {
                resolved = ResolvedSymbol(function_name, 0);
            }
        }
        symbols.push_back(resolved);
    }
    return symbols;
}

void WindowsSymbolsResolver::doUnloadLibraries() {
    std::lock_guard guard(mutex);
    if (libDia) {
        FreeLibrary(libDia);
    }
    if (libSymsrv) {
        FreeLibrary(libSymsrv);
    }
}

