cmake_minimum_required(VERSION 3.21)
project(get_symbols_playground)

set(CMAKE_CXX_STANDARD 17)

include_directories("$ENV{VSINSTALLDIR}/DIA SDK/include")

# for util.h
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

add_executable(get_symbols_playground
        dia_callback.h
        symsrv_callback.h
        util.h
        main.cpp)

set(LIB_DIA_GUIDS "$ENV{VSINSTALLDIR}/DIA SDK/lib/amd64/diaguids.lib")

target_link_libraries(get_symbols_playground ntdll.lib Winmm.lib "${LIB_DIA_GUIDS}")