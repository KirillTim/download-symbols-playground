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

struct Result {
    std::string error_message;

    static const Result OK;

    static Result error(std::string message) {
        return Result(std::move(message));
    }

    static Result winapi_error(const std::string &message) {
        return winapi_error(message, GetLastError());
    }

    static Result winapi_error(const std::string &message, DWORD code) {
        auto result_message = message + ", winapi error code: " + std::to_string(code);
        return error(result_message);
    }

    explicit operator bool() const {
        return error_message.empty();
    }

    explicit Result(std::string errorMessage) : error_message(std::move(errorMessage)) {}
};

class WindowsSymbolsResolver {
public:
    // Error Description OR Resolved symbols
    // guarded by mutex
    static std::variant<std::string, std::vector<ResolvedSymbol>>
    doResolve(const std::string &_libDiaPath, const std::string &_libSymsrvPath,
              const std::string &_additionalNtLibraryPath,
              const std::string &_dllPath, const std::vector<DWORD> &_offsets
    );

    static void doUnloadLibraries();

private:
    static std::mutex mutex;
    static HMODULE libDia; // guarded by mutex
};


#endif //ASYNC_PROFILER_WINDOWS_WINDOWSSYMBOLSRESOLVER_H
