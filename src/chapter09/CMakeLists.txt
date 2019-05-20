cmake_minimum_required(VERSION 3.13)

add_executable(ch09-land-and-waves-textured WIN32)
target_sources(ch09-land-and-waves-textured PRIVATE ch09-land-and-waves-textured.cpp LandAndWavesTextured.cpp)
target_link_libraries(ch09-land-and-waves-textured  PRIVATE DirectXMath framework)
target_compile_features(ch09-land-and-waves-textured  PRIVATE cxx_std_17)
add_custom_command(TARGET ch09-land-and-waves-textured  POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch09-land-and-waves-textured  PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch09-land-and-waves-textured  PRIVATE -Wall -Wextra -pedantic -Werror)
endif()