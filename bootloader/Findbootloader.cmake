include_guard(GLOBAL)

set(grub_path ${CMAKE_CURRENT_LIST_DIR}/grub)
set(grub_mkimage_path ${grub_path}/grub-mkimage)

set(grub_target x86_64)
set(grub_platform efi)
set(grub_modules configfile fat part_gpt gzio multiboot reboot cpuid echo sleep video video_bochs video_cirrus efi_gop efi_uga normal chain boot multiboot2)
set(grub_config_path "${CMAKE_CURRENT_LIST_DIR}/grub.cfg")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR})

