cmake_minimum_required(VERSION 3.13)

add_executable(ch01-playground "")
target_link_libraries(ch01-playground DirectXMath)
target_sources(ch01-playground PRIVATE ch01-playground.cpp)
target_compile_features(ch01-playground PUBLIC cxx_std_17)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch01-playground PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch01-playground PRIVATE -Wall -Wextra -pedantic -Werror)
endif()