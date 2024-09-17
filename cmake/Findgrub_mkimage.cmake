include_guard(GLOBAL)

set(grub_path ${CMAKE_CURRENT_LIST_DIR}/../grub)
set(grub_mkimage_path ${grub_path}/grub-mkimage)

set(grub_target x86_64)
set(grub_platform efi)
set(grub_modules configfile fat part_gpt gzio multiboot reboot cpuid echo sleep video video_bochs video_cirrus efi_gop efi_uga normal chain boot multiboot2)
set(grub_config_path "${CMAKE_SOURCE_DIR}/grub.cfg")


function (grub_compile_if_needed)
    if (NOT EXISTS ${grub_mkimage_path})
        message(STATUS "Configuring GRUB for compilation, this might take a while...")
        execute_process(
            WORKING_DIRECTORY ${grub_path}
            COMMAND ./bootstrap
            COMMAND ./configure --target=${grub_target} --with-platform=${grub_platform}
            OUTPUT_QUIET 
        )
        
        message(STATUS "Compiling GRUB, this might take a while...")
        execute_process(
            WORKING_DIRECTORY ${grub_path}
            COMMAND make
            OUTPUT_QUIET 
        )
    endif()
endfunction()

function(grub_create_bootloader bootloader_path)
    add_custom_command(
        OUTPUT ${bootloader_path}
        COMMAND ${grub_mkimage_path} -d ${grub_path}/grub-core -o ${bootloader_path} -O x86_64-efi -p "" ${grub_modules}
        VERBATIM
    )
    add_custom_target(grub_bootloader DEPENDS ${bootloader_path})
endfunction()