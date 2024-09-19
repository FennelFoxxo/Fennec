cmake_minimum_required(VERSION 3.7.2)
project(root_task C CXX ASM)

add_executable(root_task)

target_sources(root_task PRIVATE
    src/rootserver.cpp
	src/globals/globals.cpp
	src/setup/setup.cpp
	src/setup/memory.cpp
	src/threads/thread.cpp
	src/threads/memalloc.cpp
)

target_include_directories(root_task PRIVATE src)
target_link_libraries(root_task sel4muslcsys memory_allocator elf_parser)
target_compile_options(root_task PRIVATE -O0 -g -fno-inline-small-functions)
