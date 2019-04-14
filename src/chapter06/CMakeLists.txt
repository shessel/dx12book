cmake_minimum_required(VERSION 3.13)

add_executable(ch06-box-demo WIN32)
target_sources(ch06-box-demo PRIVATE ch06-box-demo.cpp BoxDemo.cpp)
target_compile_features(ch06-box-demo PRIVATE cxx_std_17)
target_link_libraries(ch06-box-demo PRIVATE framework DirectXMath)
add_custom_command(TARGET ch06-box-demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch06-box-demo PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch06-box-demo PRIVATE -Wall -Wextra -pedantic -Werror)
endif()