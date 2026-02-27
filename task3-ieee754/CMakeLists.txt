cmake_minimum_required(VERSION 3.10)
project(ieee754 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_library(ieee754 STATIC
    src/ieee754.c
)

target_include_directories(ieee754 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
