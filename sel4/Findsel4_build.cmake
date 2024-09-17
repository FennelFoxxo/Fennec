include_guard(GLOBAL)

set(sel4_project_dir ${CMAKE_CURRENT_LIST_DIR})

macro(build_sel4)
    add_subdirectory(${sel4_project_dir} sel4_build)
    get_property(sel4_root_task_path TARGET rootserver_image PROPERTY IMAGE_NAME)
    get_property(sel4_kernel_path TARGET rootserver_image PROPERTY KERNEL_IMAGE_NAME)
endmacro()