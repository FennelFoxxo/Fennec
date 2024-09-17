include_guard(GLOBAL)

find_package(efi_partition)

set(disk_img_path "disk.img")

macro (create_disk_image)
    add_custom_command(
        OUTPUT ${disk_img_path}
        DEPENDS efi_partition
        COMMAND dd if=${efi_partition_img_path} of=${disk_img_path} oflag=seek_bytes bs=1M seek=2048b status=none
        COMMAND parted -s ${disk_img_path} mklabel gpt mkpart '"EFI SYSTEM"' fat16 2048s 100% set 1 esp on
        VERBATIM
    )
    
    add_custom_target(disk_img ALL DEPENDS ${disk_img_path})
endmacro()