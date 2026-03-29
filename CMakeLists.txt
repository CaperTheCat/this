cmake_minimum_required(VERSION 3.10)

add_executable(thisc tlex.cpp tparse.cpp thisc.cpp)
add_executable(thisvm tlex.cpp tparse.cpp thisvm.cpp)

target_include_directories(thisc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(thisvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# need to test o2 vs o3 later
target_compile_options(thisc PUBLIC -O2 -flto -Wall -Wextra -pedantic -Werror)
target_compile_options(thisvm PUBLIC -O2 -flto -Wall -Wextra -pedantic -Werror)