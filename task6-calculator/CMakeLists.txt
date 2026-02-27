cmake_minimum_required(VERSION 3.10)
project(calculator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_library(calculator_lib STATIC
    src/arraylist.c
    src/bignum.c
    src/rpn.c
)

target_include_directories(calculator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(calculator
    calculator/calculator.c
)

target_link_libraries(calculator PRIVATE calculator_lib)
