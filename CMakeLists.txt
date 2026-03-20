cmake_minimum_required(VERSION 3.10)

project(This)

add_executable(thisc tlex.cpp tparse.cpp thisc.cpp)
add_executable(thisvm tlex.cpp tparse.cpp thisvm.cpp)

target_include_directories(thisc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(thisvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(thisc PUBLIC -Ofast -flto)
target_compile_options(thisvm PUBLIC -Ofast -flto)