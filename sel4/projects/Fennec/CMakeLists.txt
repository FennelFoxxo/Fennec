cmake_minimum_required(VERSION 3.7.2)

# declare the hello-world CMake project and the languages it is written in (just C)
project(Fennec C CXX ASM)

include(settings.cmake)

# In future tutorials, these setup steps will be replaced with
# sel4_tutorials_setup_roottask_tutorial_environment()
find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)
find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)

find_package(memory_allocator REQUIRED)
import_memory_allocator()

sel4_import_kernel()
elfloader_import_project()

# This sets up environment build flags and imports musllibc and runtime libraries.
musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_libs_import_libraries()

# Name the executable and list source files required to build it
add_executable(Fennec
	src/rootserver.cpp
	src/globals/globals.cpp
	src/setup/setup.cpp
	src/setup/memory.cpp
	src/threads/thread.cpp
	src/threads/memalloc.cpp
)

target_include_directories(Fennec PRIVATE src)

target_compile_options(Fennec PRIVATE -O0 -g -fno-inline-small-functions)

# List of libraries to link with the application.
target_link_libraries(Fennec sel4muslcsys memory_allocator)

# Tell the build system that this application is the root task. 
include(rootserver)
DeclareRootserver(Fennec)

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
