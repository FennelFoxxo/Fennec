# musllibc sets a lot of global compilation and link flags that are needed when building a project to run on seL4,
# but will break normal compilation of projects (e.g. a C project that runs on the host machine to generate files).
# By wrapping the core seL4 compilation in its own CMakeLists.txt, we can call add_subdirectory which will not bring
# variables or compiler flags into the parent scope, but it will make targets visible. Projects linking with sel4muslcsys
# will then pick up on these flags.

cmake_minimum_required(VERSION 3.16.0)
project(sel4_wrapper)

include(${CMAKE_CURRENT_LIST_DIR}/../settings.cmake)

find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)
find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)

sel4_import_kernel()
elfloader_import_project()

musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()

util_libs_import_libraries()
sel4_libs_import_libraries()

# Pick up on flags set using add_compile_options and add_link_options
get_property(COMPILE_OPTIONS_PROPERTY DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_OPTIONS)
get_property(LINK_OPTIONS_PROPERTY DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_OPTIONS)

# Keep C and CXX options separate
list(APPEND MUSLLIBC_C_FLAGS ${COMPILE_OPTIONS_PROPERTY})
list(APPEND MUSLLIBC_CXX_FLAGS ${COMPILE_OPTIONS_PROPERTY} ${CMAKE_CXX_FLAGS})
list(APPEND MUSLLIBC_LINK_FLAGS ${LINK_OPTIONS_PROPERTY} ${CMAKE_EXE_LINKER_FLAGS})

# Sometimes the options may have spaces in between - replace with semicolons to treat as list
string(REPLACE " " ";" MUSLLIBC_C_FLAGS "${MUSLLIBC_C_FLAGS}")
string(REPLACE " " ";" MUSLLIBC_CXX_FLAGS "${MUSLLIBC_CXX_FLAGS}")
string(REPLACE " " ";" MUSLLIBC_LINK_FLAGS "${MUSLLIBC_LINK_FLAGS}")

# Standalone target for compile and link options
add_library(sel4_flags INTERFACE)

target_compile_options(sel4_flags INTERFACE $<$<COMPILE_LANGUAGE:C>:${MUSLLIBC_C_FLAGS}>)
target_compile_options(sel4_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${MUSLLIBC_CXX_FLAGS}>)
target_link_options(sel4_flags INTERFACE ${MUSLLIBC_LINK_FLAGS})

# Projects using sel4muslcsys also need to link with crt0, crti, crtbegin, crtend, and crtn to provide the prologue and epilogue for _init and _fini
# CRTObjFiles and FinObjFiles are set in musllibc_set_environment_flags
string(REPLACE " " ";" CRT_OBJ_FILES_LIST "${CRTObjFiles}")
string(REPLACE " " ";" FIN_OBJ_FILES_LIST "${FinObjFiles}")

add_library(sel4_crt INTERFACE)
#Add the list of libraries as an option to ensure it comes before all other libraries in the link command
target_link_options(sel4_crt INTERFACE ${CRT_OBJ_FILES_LIST} ${FIN_OBJ_FILES_LIST})
add_dependencies(sel4_crt sel4runtime_crt)

# Make sure any projects linking with sel4muslcsys pick up on these flags and crt files
target_link_libraries(sel4muslcsys INTERFACE sel4_crt sel4_flags)
