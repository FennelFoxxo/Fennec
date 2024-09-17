include_guard(GLOBAL)

find_package(grub_mkimage)

set(efi_partition_folder "${CMAKE_BINARY_DIR}/efi_partition_folder")
set(efi_partition_img_path "efi_partition.img")
set(root_task_final_path "${efi_partition_folder}/${PROJECT_NAME}/root_task")
set(kernel_final_path "${efi_partition_folder}/${PROJECT_NAME}/kernel")
set(grub_config_final_path "${efi_partition_folder}/EFI/BOOT/grub.cfg")

set(efi_partition_size_mb 40)
set(root_task_min_size_mb 1)

macro(efi_partition_create_directories)
    FILE(MAKE_DIRECTORY
        ${efi_partition_folder}/EFI/BOOT
        ${efi_partition_folder}/${PROJECT_NAME}
    )
endmacro()


macro(create_efi_partition)
    grub_create_bootloader("${efi_partition_folder}/EFI/BOOT/BOOTX64.EFI")
    
    # Copy grub config file and do string replacement
    file(READ ${grub_config_path} grub_config_file_contents)
    string(REPLACE "{ROOT_TASK_PATH}" "/${PROJECT_NAME}/root_task" grub_config_file_contents "${grub_config_file_contents}")
    string(REPLACE "{KERNEL_PATH}" "/${PROJECT_NAME}/kernel" grub_config_file_contents "${grub_config_file_contents}")
    file(WRITE ${grub_config_final_path} "${grub_config_file_contents}")
    
    # Copy root task
    add_custom_command(
        OUTPUT ${root_task_final_path}
        DEPENDS ${sel4_root_task_path}
        COMMAND dd if=/dev/zero of=${root_task_final_path} bs=1M count=${root_task_min_size_mb} status=none
        COMMAND dd if=${sel4_root_task_path} of=${root_task_final_path} conv=notrunc status=none
        VERBATIM
    )
    
    # Copy kernel
    add_custom_command(
        OUTPUT ${kernel_final_path}
        DEPENDS ${sel4_kernel_path}
        COMMAND cp ${sel4_kernel_path} ${kernel_final_path}
        VERBATIM
    )
    
    add_custom_command(
        OUTPUT ${efi_partition_img_path}
        DEPENDS grub_bootloader ${root_task_final_path} ${kernel_final_path} ${grub_config_final_path}
        COMMAND dd if=/dev/zero of=${efi_partition_img_path} bs=1M count=${efi_partition_size_mb} status=none
        COMMAND mkfs.vfat -F 32 -n "EFI SYSTEM" ${efi_partition_img_path} > /dev/null
        COMMAND mcopy -i ${efi_partition_img_path} -s ${efi_partition_folder}/* ::
    )
    
    add_custom_target(efi_partition ALL DEPENDS ${efi_partition_img_path})
endmacro()
