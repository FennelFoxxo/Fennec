cmake_minimum_required(VERSION 3.7.2)
project(Fennec)

list(
    APPEND
        CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_LIST_DIR}/sel4
        ${CMAKE_CURRENT_LIST_DIR}/bootloader
        ${CMAKE_CURRENT_LIST_DIR}/cmake
)

find_package(bootloader REQUIRED)
find_package(disk_image REQUIRED)
find_package(sel4_build REQUIRED)

bootloader_compile_if_needed()

build_sel4()
create_disk_image()
