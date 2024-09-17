cmake_minimum_required(VERSION 3.7.2)

function(bootloader_compile_if_needed)
    if (NOT EXISTS ${grub_mkimage_path})
        message(STATUS "Bootstrapping GRUB for configuration, this might take a while...")

        execute_process(
            COMMAND ./bootstrap
            WORKING_DIRECTORY ${grub_path}
            OUTPUT_QUIET
            ERROR_VARIABLE last_command_err
            RESULT_VARIABLE last_command_result
        )
        if (NOT last_command_result EQUAL 0)
            message(FATAL_ERROR "Configuration bootstrap failed! Error log:\n${last_command_err}")
        endif()

        message(STATUS "Configuring GRUB for compilation, this might take a while...")
        execute_process(
            COMMAND ./configure --target=${grub_target} --with-platform=${grub_platform}
            WORKING_DIRECTORY ${grub_path}
            OUTPUT_QUIET
            ERROR_VARIABLE last_command_err
            RESULT_VARIABLE last_command_result
        )
        if (NOT last_command_result EQUAL 0)
            message(FATAL_ERROR "Configuration failed! Error log:\n${last_command_err}")
        endif()

        message(STATUS "Compiling GRUB, this might take a while...")
        execute_process(
            COMMAND make
            WORKING_DIRECTORY ${grub_path}
            OUTPUT_QUIET
            ERROR_VARIABLE last_command_err
            RESULT_VARIABLE last_command_result
        )
        if (NOT last_command_result EQUAL 0)
            message(FATAL_ERROR "Compilation failed! Error log:\n${last_command_err}")
        endif()
        
    endif()
    
endfunction()

function(copy_bootloader_config dest_path)
    # Copy grub config file and do string replacement
    file(READ ${grub_config_path} grub_config_file_contents)
    string(REPLACE "{ROOT_TASK_PATH}" "/${PROJECT_NAME}/root_task" grub_config_file_contents "${grub_config_file_contents}")
    string(REPLACE "{KERNEL_PATH}" "/${PROJECT_NAME}/kernel" grub_config_file_contents "${grub_config_file_contents}")
    file(WRITE ${dest_path} "${grub_config_file_contents}")
endfunction()

macro(create_bootloader dest_path)
    add_custom_command(
        OUTPUT ${dest_path}
        COMMAND ${grub_mkimage_path} -d ${grub_path}/grub-core -o ${dest_path} -O x86_64-efi -p "" ${grub_modules}
        VERBATIM
    )
    add_custom_target(bootloader DEPENDS ${dest_path})
endmacro()