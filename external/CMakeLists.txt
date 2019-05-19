cmake_minimum_required(VERSION 3.12)

add_library(DirectXMath INTERFACE)
target_include_directories(DirectXMath INTERFACE DirectXMath/Inc)

add_library(DDSTextureLoader)
target_sources(DDSTextureLoader PRIVATE DDSTextureLoader/DDSTextureLoader12.cpp)
target_include_directories(DirectXMath INTERFACE DDSTextureLoader)