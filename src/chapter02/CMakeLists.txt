cmake_minimum_required(VERSION 3.12)

add_executable(ch02-matrix-transpose)
target_sources(ch02-matrix-transpose PRIVATE ch02-matrix-transpose.cpp)
target_compile_features(ch02-matrix-transpose PUBLIC cxx_std_17)

add_executable(ch02-determinant)
target_sources(ch02-determinant PRIVATE ch02-determinant.cpp)
target_compile_features(ch02-determinant PUBLIC cxx_std_17)

# adapted from https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(ch02-matrix-transpose PRIVATE /W4 /WX /permissive-)
	target_compile_options(ch02-determinant PRIVATE /W4 /WX /permissive-)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(ch02-matrix-transpose PRIVATE -Wall -Wextra -pedantic -Werror)
	target_compile_options(ch02-determinant PRIVATE -Wall -Wextra -pedantic -Werror)
endif()