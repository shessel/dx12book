cmake_minimum_required(VERSION 3.13)

add_executable(ch08-land-and-waves-demo-lit WIN32)
target_sources(ch08-land-and-waves-demo-lit PRIVATE ch08-land-and-waves-demo-lit.cpp LandAndWavesDemoLit.cpp)
target_link_libraries(ch08-land-and-waves-demo-lit  PRIVATE DirectXMath framework)
target_compile_features(ch08-land-and-waves-demo-lit  PRIVATE cxx_std_17)
add_custom_command(TARGET ch08-land-and-waves-demo-lit  POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch08-land-and-waves-demo-lit  PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch08-land-and-waves-demo-lit  PRIVATE -Wall -Wextra -pedantic -Werror)
endif()