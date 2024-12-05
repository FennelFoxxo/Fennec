cmake_minimum_required(VERSION 3.7.2)
project(root_task C CXX ASM)

# Set flags
set(KernelHugePage OFF CACHE BOOL "Compile for huge-page enabled processors")
set(PLATFORM "x86_64" CACHE STRING "Platform to compile for")
set(SIMULATION TRUE CACHE BOOL "Build for simulation")

include(${CMAKE_CURRENT_LIST_DIR}/settings/settings.cmake)

find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)
find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)

# These packages need to be linked with our root task, but should be included before special compilation flags get set
find_package(elf_parser REQUIRED)

# Setting build flags, etc.
sel4_import_kernel()
elfloader_import_project()
musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_libs_import_libraries()

# Packages which should be compiled with sel4 runtime
find_package(memory_allocator REQUIRED)

# Our root task
find_package(root_task REQUIRED)

include(rootserver)
DeclareRootserver(root_task)

if(SIMULATION)
    include(simulation)
    if(KernelSel4ArchX86_64)
        SetSimulationScriptProperty(MEM_SIZE "3G")
    endif()
    if(KernelPlatformQEMUArmVirt)
        SetSimulationScriptProperty(MEM_SIZE "2G")
    endif()
    GenerateSimulateScript()
endif()

