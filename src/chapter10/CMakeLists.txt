cmake_minimum_required(VERSION 3.13)

add_executable(ch10-land-and-waves-blended WIN32)
target_sources(ch10-land-and-waves-blended PRIVATE ch10-land-and-waves-blended.cpp LandAndWavesBlended.cpp)
target_link_libraries(ch10-land-and-waves-blended  PRIVATE DirectXMath framework)
target_compile_features(ch10-land-and-waves-blended  PRIVATE cxx_std_17)
add_custom_command(TARGET ch10-land-and-waves-blended  POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch10-land-and-waves-blended  PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch10-land-and-waves-blended  PRIVATE -Wall -Wextra -pedantic -Werror)
endif()