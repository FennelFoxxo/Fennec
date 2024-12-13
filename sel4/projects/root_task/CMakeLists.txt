cmake_minimum_required(VERSION 3.7.2)
project(root_task C CXX ASM)

add_executable(root_task)

target_sources(root_task PRIVATE
    src/rootserver.cpp
    	src/graphics.cpp
	src/mem_tree.cpp
	src/globals/global_vars.cpp

	src/setup/break_memory_into_chunks.cpp
	src/setup/setup_mapping.cpp
	src/setup/setup_new_cspace.cpp
	src/setup/init_boot_info.cpp
	src/setup/launch_memory_allocator_thread.cpp
	src/setup/setup_graphics.cpp
)

target_include_directories(root_task PRIVATE src)
target_link_libraries(root_task sel4muslcsys memory_allocator elf_parser constants)
target_compile_options(root_task PRIVATE -O0 -g -fno-inline-small-functions -Wall)
