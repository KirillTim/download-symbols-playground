#ifndef GET_SYMBOLS_PLAYGROUND_DIA_CALLBACK_H
#define GET_SYMBOLS_PLAYGROUND_DIA_CALLBACK_H

#include "dia2.h"
#include "diacreate.h"

// This constant may be missing from DbgHelp.h.  See the documentation for
// IDiaSymbol::get_undecoratedNameEx.
#ifndef UNDNAME_NO_ECSU
#define UNDNAME_NO_ECSU 0x8000  // Suppresses enum/class/struct/union.
#endif  // UNDNAME_NO_ECSU

struct DiaLoggerCallBack: public IDiaLoadCallback2 {
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
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 1; // On stack
    }

    //------------------------------------------------------------------
    ULONG STDMETHODCALLTYPE Release() override
    {
        return 1; // On stack
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyDebugDir(BOOL fExecutable,
    DWORD cbData,
            BYTE* pbData) override
    {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(LPCOLESTR dbgPath,
    HRESULT resultCode) override
    {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(LPCOLESTR pdbPath,
    HRESULT resultCode) override
    {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess() override
    {
        return S_OK;
    }

    //------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess() override
    {
        // TODO: `symsrv.dll` required to access symbol server
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
} libdiaCallback;


#endif //GET_SYMBOLS_PLAYGROUND_DIA_CALLBACK_H
