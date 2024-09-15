cmake_minimum_required(VERSION 3.7.2)

# Compile and link into executable
add_executable(memory_allocator_bin)
target_sources(memory_allocator_bin PRIVATE src/main.c)
target_include_directories(memory_allocator_bin PRIVATE "include_private")

# Wrap entire library elf file in a section which can be imported
add_custom_command(OUTPUT memory_allocator_bin_packaged
		   DEPENDS memory_allocator_bin
                   COMMAND ${CMAKE_OBJCOPY}
		   ARGS --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 memory_allocator_bin memory_allocator_bin_packaged
)


# Magic to make cmake treat object file as library target
# First add a custom target which depends on the object file - then add library target that depends on that custom target
# Finally set the imported location of the library to be the object file

add_custom_target(memory_allocator_temp_target DEPENDS memory_allocator_bin_packaged)

add_library(memory_allocator STATIC IMPORTED GLOBAL)
add_dependencies(memory_allocator memory_allocator_temp_target)
target_include_directories(memory_allocator INTERFACE "include_interface")
set_target_properties(memory_allocator
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/memory_allocator_bin_packaged
)