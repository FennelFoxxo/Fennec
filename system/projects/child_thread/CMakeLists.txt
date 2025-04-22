cmake_minimum_required(VERSION 3.7.2)
project(child_thread)

add_executable(child_thread src/main.c)
target_link_options(child_thread PRIVATE "-nostdlib")
target_link_libraries(child_thread sel4muslcsys)