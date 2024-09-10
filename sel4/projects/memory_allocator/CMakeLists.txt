cmake_minimum_required(VERSION 3.7.2)


project(memory_allocator C)

set(TempExec "${PROJECT_NAME}_exec")
set(TempObj "${PROJECT_NAME}.obj")
set(TempTarget "${PROJECT_NAME}_target")

# Compile and link into executable
add_executable(${TempExec} src/main.c)

# Wrap entire library elf file in a section which can be imported
add_custom_command(OUTPUT ${TempObj}
		   DEPENDS ${TempExec}
                   COMMAND ${CMAKE_OBJCOPY}
		   ARGS --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 ${TempExec} ${TempObj}
)


# Magic to make cmake treat object file as library target
# First add a custom target which depends on the object file - then add library target that depends on that custom target
# Finally set the imported location of the library to be the object file

add_custom_target(${TempTarget} DEPENDS ${TempObj})

add_library(memory_allocator STATIC IMPORTED GLOBAL)
add_dependencies(memory_allocator ${TempTarget})
set_target_properties(memory_allocator
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${TempObj}
)