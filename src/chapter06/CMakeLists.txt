cmake_minimum_required(VERSION 3.12)

add_executable(ch06-box-demo WIN32)
target_sources(ch06-box-demo PRIVATE ch06-box-demo.cpp)
target_compile_features(ch06-box-demo PRIVATE cxx_std_17)
target_link_libraries(ch06-box-demo PRIVATE framework)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch06-box-demo PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch06-box-demo PRIVATE -Wall -Wextra -pedantic -Werror)
endif()