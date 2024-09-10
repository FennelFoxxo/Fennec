set(MEMORY_ALLOCATOR_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MEMORY_ALLOCATOR_DIR)

macro(import_memory_allocator)
    add_subdirectory(${MEMORY_ALLOCATOR_DIR} memory_allocator)
endmacro()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(memory_allocator DEFAULT_MSG MEMORY_ALLOCATOR_DIR)
