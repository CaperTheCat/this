cmake_minimum_required(VERSION 3.10)

add_executable(this tlex.cpp tparse.cpp thisc.cpp thisvm.cpp this.cpp)

target_include_directories(this PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# need to test o2 vs o3 later
target_compile_features(this PUBLIC cxx_std_17)