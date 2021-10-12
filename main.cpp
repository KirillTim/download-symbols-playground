#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <vector>
#include <fstream>

#include "Windows.h"

#include "common/src/WindowsSymbolsResolver.h"
#include "timer.h"

const char* libdia_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\msdia140.dll)";
const char* symsrv_path = R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\libs\symsrv_last_version\symsrv.dll)";

const std::string bin_server_jvm_dll = "bin\\server\\jvm.dll";

const std::string jvmdll_symbols_same_folder = R"(C:\Users\Kirill.Timofeev\Downloads\openjdk-jdk17-windows-x86_64-server-release\jdk\)" + bin_server_jvm_dll;
const std::string jvmdll_no_symbol = "C:\\Users\\Kirill.Timofeev\\.jdks\\corretto-1.8.0_302\\jre\\" + bin_server_jvm_dll;

// the same file located in: data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
// C:\Users\Kirill.Timofeev\Work\get-symbols-playground\data\jbr_jvm_dll\debugsymbols_on_server\jvm.dll
const std::string jbr_symbols_on_server = "C:\\Users\\Kirill.Timofeev\\AppData\\Local\\JetBrains\\Toolbox\\apps\\IDEA-U\\ch-0\\213.3358\\jbr\\" + bin_server_jvm_dll;

const std::string ntdll_local_symsrv_lookup = R"(C:\Windows\SYSTEM32\ntdll.dll)";

using namespace std;
vector<string> split (const string& s, const string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}
int main() {const std::string nt_lib_path = "srv*C:\\JBRSymbols*https://resources.jetbrains.com/pdb;";
    std::ifstream offsets_file(R"(C:\Users\Kirill.Timofeev\Work\get-symbols-playground\data\frames\folder8\jvm.dll.frames)");
    std::vector<DWORD> offsets;
    std::string dll_path;
    std::string line;
    while (offsets_file >> line) {
        if (line.empty()) continue;
        if (line[0] == '=') continue;
        vector<string> parts = split(line, "+0x");
        if (parts.size() != 2) continue;
        if (dll_path.empty()) {
            dll_path = parts[0];
        }
        offsets.push_back(stoi(parts[1], nullptr, 16));
    }

    std::cerr << "offsets.size(): " << offsets.size() << std::endl;

    //const char* dll_path = R"(C:\Users\Kirill.Timofeev\AppData\Local\JetBrains\Toolbox\apps\IDEA-U\ch-0\213.4708\jbr\bin\server\jvm.dll)";
    CodeExecutionTimer time;
    time.start();
    auto result = WindowsSymbolsResolver::doResolve(libdia_path, symsrv_path, nt_lib_path, dll_path, offsets);
    std::cerr << "resolve time: " << time.elapsed_time_milliseconds() << " (ms)" << std::endl;
    if (std::holds_alternative<std::vector<ResolvedSymbol>>(result)) {
        auto resolvedSymbols = std::get<std::vector<ResolvedSymbol>>(result);
        std::cerr << "resolvedSymbols.size(): " << resolvedSymbols.size() << std::endl;
    } else {
        auto errorMessage = std::get<std::string>(result);
        std::cerr << "error message: " << errorMessage << std::endl;
    }
    return 0;
}
