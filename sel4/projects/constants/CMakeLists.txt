cmake_minimum_required(VERSION 3.7.2)
project(constants C)

add_library(constants INTERFACE)
target_include_directories(constants INTERFACE include)