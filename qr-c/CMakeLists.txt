cmake_minimum_required(VERSION 3.10)
project(qr_serial C)

add_executable(qr_serial
        src/main.c
        src/qr_core.c
        src/logging.c
)

target_include_directories(qr_serial
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(qr_serial
        PRIVATE
        -Wall
        -Wextra
)
