cmake_minimum_required(VERSION 4.3)
project(clox_impl C)

set(CMAKE_C_STANDARD 17)

add_executable(clox_impl main.c
        chunk.c
        memory.c
        debug.c
        value.c
        vm.c
        scanner.c
        compiler.c)
