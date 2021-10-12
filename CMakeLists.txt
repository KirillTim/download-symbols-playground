cmake_minimum_required(VERSION 3.20)
project(get_symbols_playground)

set(CMAKE_CXX_STANDARD 17)

include_directories("$ENV{VSINSTALLDIR}/DIA SDK/include")

# for util.h
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

list(APPEND COMMON_SOURCES
        common/src/diaUtil.h
        common/src/diaUtil.cpp
        common/src/util.h
        common/src/util.cpp
        common/src/symsrv_callback.h
        common/src/WindowsSymbolsResolver.h
        common/src/WindowsSymbolsResolver.cpp)

add_executable(get_symbols_playground
        ${COMMON_SOURCES}
        timer.h
        md5_util.h
        md5_util.cpp
        main.cpp)

set(LIB_DIA_GUIDS "$ENV{VSINSTALLDIR}/DIA SDK/lib/amd64/diaguids.lib")

target_link_libraries(get_symbols_playground ntdll.lib Winmm.lib "${LIB_DIA_GUIDS}")