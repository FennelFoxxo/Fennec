cmake_minimum_required(VERSION 3.7.2)
project(elf_parser)

add_library(elf_parser STATIC)
target_sources(elf_parser PRIVATE ElfParser/src/parse.c ElfParser/src/parse32.c ElfParser/src/parse64.c)
target_include_directories(elf_parser PUBLIC "ElfParser/include")