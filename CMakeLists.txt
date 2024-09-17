cmake_minimum_required(VERSION 3.7.2)
project(Fennec)

list(
    APPEND
        CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_LIST_DIR}/sel4
        ${CMAKE_CURRENT_LIST_DIR}/cmake
)

find_package(grub_mkimage)
find_package(efi_partition)
find_package(disk_layout)
find_package(sel4_build)


grub_compile_if_needed()
efi_partition_create_directories()



# add_executable(main main.c)


build_sel4()
create_efi_partition()
create_disk_image()


message(STATUS "grub_mkimage path: ${grub_mkimage_path}")
message(STATUS "image name: ${sel4_root_task_path}")
message(STATUS "kernel image name: ${sel4_kernel_path}")

