cmake_minimum_required(VERSION 3.12)

add_executable(ch04-list-adapters)
target_sources(ch04-list-adapters PRIVATE ch04-list-adapters.cpp)
target_compile_features(ch04-list-adapters PUBLIC cxx_std_17)

target_link_libraries(ch04-list-adapters PRIVATE DXGI.lib)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch04-list-adapters PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch04-list-adapters PRIVATE -Wall -Wextra -pedantic -Werror)
endif()