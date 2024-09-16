#pragma once

extern void* _binary_memory_allocator_bin_start;
extern void* _binary_memory_allocator_bin_size;

void* memory_allocator_elf_start = &_binary_memory_allocator_bin_start;
long long unsigned memory_allocator_elf_size = (long long unsigned)&_binary_memory_allocator_bin_size;;