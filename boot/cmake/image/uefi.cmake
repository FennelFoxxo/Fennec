set(efi_partition_size_mb 40)
set(root_task_min_size_mb 1)


set(IMAGE_PATH "${CMAKE_CURRENT_BINARY_DIR}/disk.img")

set(esp_root "${CMAKE_CURRENT_BINARY_DIR}/esp_root")
set(esp_img "${CMAKE_CURRENT_BINARY_DIR}/esp.img")

set(bootloader_bin_dest "${esp_root}/EFI/BOOT/BOOTX64.EFI")

get_filename_component(bootloader_config_filename "${BOOTLOADER_CONFIG_PATH}" NAME)
set(bootloader_config_dest "${esp_root}/EFI/BOOT/${bootloader_config_filename}")

set(root_task_dest "${esp_root}/${CMAKE_PROJECT_NAME}/root_task")
set(kernel_dest "${esp_root}/${CMAKE_PROJECT_NAME}/kernel")

macro(copy_file_command in_path out_path comment)
    add_custom_command(
        OUTPUT "${out_path}"
        DEPENDS "${in_path}"
        COMMAND cp "${in_path}" "${out_path}"
        COMMENT "${comment}"
        VERBATIM
    )
endmacro()

macro(copy_file_with_padding_command in_path out_path padding_count block_size comment)
    add_custom_command(
        OUTPUT "${out_path}"
        DEPENDS "${in_path}"
        COMMAND dd if=/dev/zero of=${out_path} count=${padding_count} bs=${block_size} status=none
        COMMAND dd if=${in_path} of=${out_path} conv=notrunc status=none
        COMMENT ${comment}
        VERBATIM
    )
endmacro()

FILE(MAKE_DIRECTORY
    "${esp_root}/EFI/BOOT"
    "${esp_root}/${CMAKE_PROJECT_NAME}"
)

# Copy root task
copy_file_with_padding_command("${ROOTSERVER_PATH}" "${root_task_dest}" ${root_task_min_size_mb} "1M" "Copy root task into disk")

# Copy kernel
copy_file_command("${KERNEL_PATH}" "${kernel_dest}" "Copy kernel into disk")

# Copy bootloader
copy_file_command("${BOOTLOADER_BIN_PATH}" "${bootloader_bin_dest}" "Copy bootloader into disk")

# Copy bootloader config
copy_file_command("${BOOTLOADER_CONFIG_PATH}" "${bootloader_config_dest}" "Copy bootloader config into disk")

# Create and format EFI partition image
add_custom_command(
    OUTPUT "${esp_img}"
    DEPENDS "${root_task_dest}" "${kernel_dest}" "${bootloader_bin_dest}" "${bootloader_config_dest}"
    COMMAND dd if=/dev/zero of=${esp_img} bs=1M count=${efi_partition_size_mb} status=none
    COMMAND mkfs.vfat -F 32 -n "EFI SYSTEM" ${esp_img} > /dev/null
    COMMAND mcopy -i ${esp_img} -s ${esp_root}/* ::
)

# Build disk image from EFI partition image
add_custom_command(
    OUTPUT "${IMAGE_PATH}"
    DEPENDS "${esp_img}"
    COMMAND dd if=${esp_img} of=${IMAGE_PATH} oflag=seek_bytes bs=1M seek=2048b status=none
    COMMAND parted -s ${IMAGE_PATH} mklabel gpt mkpart '"EFI SYSTEM"' fat16 2048s 100% set 1 esp on
    VERBATIM
)
