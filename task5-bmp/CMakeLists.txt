cmake_minimum_required(VERSION 3.10)
project(bmp_utilities C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_library(bmp STATIC
    src/bmp.c
)

target_include_directories(bmp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(converter
    converter/converter.c
)

target_link_libraries(converter PRIVATE bmp)

add_executable(comparer
    comparer/comparer.c
)

target_link_libraries(comparer PRIVATE bmp)
