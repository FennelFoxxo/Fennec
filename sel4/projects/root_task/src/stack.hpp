#pragma once

#include <string.h>

class Stack {
public:
    Stack(void* stack_top) : stack_top_(stack_top) {}
    Stack(long long unsigned stack_top) : stack_top_((void*)stack_top) {}
    
    template <typename T>
    void* push(const T& data) {
        stack_top_ = (char*)stack_top_ - sizeof(T);
        *(T*)stack_top_ = data;
        return stack_top_;
    }
    
    void* pushString(const char* str) {
        long long unsigned bytes = strlen(str) + 1;
        stack_top_ = (char*)stack_top_ - bytes;
        memcpy(stack_top_, str, bytes);
        return stack_top_;
    }
    

    template <typename T>
    T pop() {
        T data = *(T*)stack_top_;
        stack_top_ = (char*)stack_top_ + sizeof(T);
        return data;
    }
    
    long long unsigned getStackTop() {
        return (long long unsigned)stack_top_;
    }
    
private:
    void* stack_top_;
};