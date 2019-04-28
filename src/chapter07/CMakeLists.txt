cmake_minimum_required(VERSION 3.13)

add_executable(ch07-shapes-demo WIN32)
target_sources(ch07-shapes-demo PRIVATE ch07-shapes-demo.cpp ShapesDemo.cpp)
target_compile_features(ch07-shapes-demo PRIVATE cxx_std_17)
target_link_libraries(ch07-shapes-demo PRIVATE framework DirectXMath)
add_custom_command(TARGET ch07-shapes-demo POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch07-shapes-demo PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch07-shapes-demo PRIVATE -Wall -Wextra -pedantic -Werror)
endif()