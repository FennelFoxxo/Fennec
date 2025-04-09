cmake_minimum_required(VERSION 3.16.0)
project(Fennec)

list(
    APPEND
        CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_LIST_DIR}/boot
        ${CMAKE_CURRENT_LIST_DIR}/system
)

set(RUNTIME_CONFIGS "${CMAKE_CURRENT_LIST_DIR}/runtime-configs")

find_package(system REQUIRED)
find_package(boot REQUIRED)

set_bootloader_module("grub")
set_image_module("uefi")

build_system()
build_image()