cmake_minimum_required(VERSION 3.13)

add_executable(ch11-mirror WIN32)
target_sources(ch11-mirror PRIVATE ch11-mirror.cpp Mirror.cpp)
target_link_libraries(ch11-mirror  PRIVATE DirectXMath framework)
target_compile_features(ch11-mirror  PRIVATE cxx_std_17)
add_custom_command(TARGET ch11-mirror POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../data ${CMAKE_CURRENT_BINARY_DIR}/data)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch11-mirror  PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch11-mirror  PRIVATE -Wall -Wextra -pedantic -Werror)
endif()