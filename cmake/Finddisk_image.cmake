include_guard(GLOBAL)

set(efi_partition_size_mb 40)
set(root_task_min_size_mb 1)

set(efi_partition_folder "${CMAKE_BINARY_DIR}/efi_partition_folder")
set(efi_partition_img_path "efi_partition.img")
set(disk_img_path "disk.img")
set(root_task_final_path "${efi_partition_folder}/${PROJECT_NAME}/root_task")
set(kernel_final_path "${efi_partition_folder}/${PROJECT_NAME}/kernel")
set(grub_config_final_path "${efi_partition_folder}/EFI/BOOT/grub.cfg")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/disk_image)