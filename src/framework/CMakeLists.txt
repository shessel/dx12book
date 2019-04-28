cmake_minimum_required(VERSION 3.12)

add_library(framework)
target_sources(framework PRIVATE
    ArcBallCamera.cpp
    AppBase.cpp
    D3D12Util.cpp
    DebugUtil.cpp
    Mesh.cpp
    Timer.cpp)
target_compile_features(framework PUBLIC cxx_std_17)
target_include_directories(framework INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(framework PRIVATE dxgi.lib d3d12.lib d3dcompiler.lib)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(framework PRIVATE /W4 /WX /permissive-)
    target_compile_definitions(framework PRIVATE UNICODE _UNICODE)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(framework PRIVATE -Wall -Wextra -pedantic -Werror)
endif()