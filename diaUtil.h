#ifndef ASYNC_PROFILER_WINDOWS_DIAUTIL_H
#define ASYNC_PROFILER_WINDOWS_DIAUTIL_H

#include <string>

#include "dia2.h"
#include "diacreate.h"
#include <ImageHlp.h>

// This constant may be missing from DbgHelp.h.  See the documentation for
// IDiaSymbol::get_undecoratedNameEx.
#ifndef UNDNAME_NO_ECSU
#define UNDNAME_NO_ECSU 0x8000  // Suppresses enum/class/struct/union.
#endif  // UNDNAME_NO_ECSU

static const DWORD UNDECORATE_OPTIONS = UNDNAME_NO_MS_KEYWORDS |
                                        UNDNAME_NO_FUNCTION_RETURNS |
                                        UNDNAME_NO_ALLOCATION_MODEL |
                                        UNDNAME_NO_ALLOCATION_LANGUAGE |
                                        UNDNAME_NO_THISTYPE |
                                        UNDNAME_NO_ACCESS_SPECIFIERS |
                                        UNDNAME_NO_THROW_SIGNATURES |
                                        UNDNAME_NO_MEMBER_TYPE |
                                        UNDNAME_NO_RETURN_UDT_MODEL |
                                        UNDNAME_NO_ECSU |
                                        UNDNAME_NO_SPECIAL_SYMS;

std::string dia_error_string(HRESULT error_code);

struct DiaAllowEverythingSettingsCallBack : public IDiaLoadCallback2 {
    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
    _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override {
        if (!ppvObject)
            return E_POINTER;

        if (riid == IID_IDiaLoadCallback) {
            *ppvObject = this;
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    //------------------------------------------------------------------
    ULONG STDMETHODCALLTYPE AddRef() override {
        return 1; // On stack
    }

    //------------------------------------------------------------------
    ULONG STDMETHODCALLTYPE Release() override {
        return 1; // On stack
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyDebugDir(BOOL fExecutable,
    DWORD cbData,
            BYTE *pbData) override {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(LPCOLESTR dbgPath,
    HRESULT resultCode) override {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(LPCOLESTR pdbPath,
    HRESULT resultCode) override {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess() override {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess() override {
        return S_OK;
    }

    HRESULT RestrictOriginalPathAccess() override {
        return S_OK;
    }

    HRESULT RestrictReferencePathAccess() override {
        return S_OK;
    }

    HRESULT RestrictDBGAccess() override {
        return S_OK;
    }

    HRESULT RestrictSystemRootAccess() override {
        return S_OK;
    }
};

#endif //ASYNC_PROFILER_WINDOWS_DIAUTIL_H
