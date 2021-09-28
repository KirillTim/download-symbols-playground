#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <vector>

#include "Windows.h"

#include "WindowsSymbolsResolver.h"

const char* libdia_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\msdia140.dll)";
const char* symsrv_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\symsrv_last_version\symsrv.dll)";

const std::string bin_server_jvm_dll = "bin\\server\\jvm.dll";

const std::string jvmdll_symbols_same_folder = R"(C:\Users\Kirill.Timofeev\Downloads\openjdk-jdk17-windows-x86_64-server-release\jdk\)" + bin_server_jvm_dll;
const std::string jvmdll_no_symbol = "C:\\Users\\Kirill.Timofeev\\.jdks\\corretto-1.8.0_302\\jre\\" + bin_server_jvm_dll;

// the same file located in: data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
// C:\Users\Kirill.Timofeev\Work\get-symbols-playground\data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
const std::string jbr_symbols_on_server = "C:\\Users\\Kirill.Timofeev\\AppData\\Local\\JetBrains\\Toolbox\\apps\\IDEA-U\\ch-0\\213.3358\\jbr\\" + bin_server_jvm_dll;

const std::string ntdll_local_symsrv_lookup = R"(C:\Windows\SYSTEM32\ntdll.dll)";

int main() {
    const std::string nt_lib_path = "srv*C:\\JBRSymbols*https://resources.jetbrains.com/pdb;";
    std::vector<DWORD> offsets;
    auto result = WindowsSymbolsResolver::doResolve(libdia_path, symsrv_path, nt_lib_path, jbr_symbols_on_server, offsets);
    if (std::holds_alternative<std::vector<ResolvedSymbol>>(result)) {
        auto resolvedSymbols = std::get<std::vector<ResolvedSymbol>>(result);
        std::cerr << "resolvedSymbols.size(): " << resolvedSymbols.size() << std::endl;
    } else {
        auto errorMessage = std::get<std::string>(result);
        std::cerr << "error message: " << errorMessage << std::endl;
    }
    return 0;
}
