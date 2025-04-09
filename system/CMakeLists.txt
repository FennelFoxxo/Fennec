cmake_minimum_required(VERSION 3.7.2)
project(tailspring_get_sel4_info C CXX ASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/sel4-wrapper")

find_package(sel4_wrapper REQUIRED)


include_sel4_configuration("common")
include_sel4_configuration("x86_64")
finalize_sel4_configuration()

find_package(Tailspring REQUIRED)

tailspring_compile()
DeclareRootserver(tailspring)

finalize_sel4_build()