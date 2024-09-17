# find_package(bootloader)

function(efi_partition_create_directories)
    FILE(MAKE_DIRECTORY
        ${efi_partition_folder}/EFI/BOOT
        ${efi_partition_folder}/${PROJECT_NAME}
    )
endfunction()

function(create_efi_partition)
    efi_partition_create_directories()
    create_bootloader("${efi_partition_folder}/EFI/BOOT/BOOTX64.EFI")
    copy_bootloader_config(${grub_config_final_path})
    
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
    
    # Create and format EFI partition image
    add_custom_command(
        OUTPUT ${efi_partition_img_path}
        DEPENDS bootloader ${root_task_final_path} ${kernel_final_path} ${grub_config_final_path}
        COMMAND dd if=/dev/zero of=${efi_partition_img_path} bs=1M count=${efi_partition_size_mb} status=none
        COMMAND mkfs.vfat -F 32 -n "EFI SYSTEM" ${efi_partition_img_path} > /dev/null
        COMMAND mcopy -i ${efi_partition_img_path} -s ${efi_partition_folder}/* ::
    )
    
    add_custom_target(efi_partition ALL DEPENDS ${efi_partition_img_path})
endfunction()

# Combine partitions into disk image
function(create_disk_image)
    create_efi_partition()
    add_custom_command(
        OUTPUT ${disk_img_path}
        DEPENDS efi_partition
        COMMAND dd if=${efi_partition_img_path} of=${disk_img_path} oflag=seek_bytes bs=1M seek=2048b status=none
        COMMAND parted -s ${disk_img_path} mklabel gpt mkpart '"EFI SYSTEM"' fat16 2048s 100% set 1 esp on
        VERBATIM
    )
    
    add_custom_target(disk_img ALL DEPENDS ${disk_img_path})
endfunction()