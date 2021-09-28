#ifndef ASYNC_PROFILER_WINDOWS_WINDOWSSYMBOLSRESOLVER_H
#define ASYNC_PROFILER_WINDOWS_WINDOWSSYMBOLSRESOLVER_H

#include <string>
#include <mutex>
#include <variant>
#include <vector>

#include "Windows.h"

// for now, we store displacement only for unresolved symbols. `name` is equal to target dll path
// for resolved symbols we only store result string in `name`
struct ResolvedSymbol {
    ResolvedSymbol(std::string name, DWORD displacement)
            : name(std::move(name)), displacement(displacement) {}

    ResolvedSymbol() : ResolvedSymbol("", 0) {}

    std::string name;
    DWORD displacement;
};

class WindowsSymbolsResolver {
public:
    // Error Description OR Resolved symbols
    static std::variant<std::string, std::vector<ResolvedSymbol>>
    doResolve(const std::string &_libDiaPath, const std::string &_libSymsrvPath,
              const std::string &_additionalNtLibraryPath,
              const std::string &_dllPath, const std::vector<DWORD> &_offsets
    );

    static void doUnloadLibraries();

private:
    static std::mutex mutex;
    static HMODULE libDia; // guarded by mutex
    static HMODULE libSymsrv; // guarded by mutex
};


#endif //ASYNC_PROFILER_WINDOWS_WINDOWSSYMBOLSRESOLVER_H
