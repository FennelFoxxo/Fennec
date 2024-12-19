#pragma once

typedef void (*AssertFailCallback)(const char* err);

#ifdef __cplusplus
extern "C" {
#endif

void setAssertFailCallback(AssertFailCallback f);

// Private, these should not be called directly
void _assertFailWrapper1(const char* ex, const char* file, int line);
void _assertFailWrapper2(const char* ex, const char* file, int line, int code);

#ifdef __cplusplus
}
#endif

#define ASSERT_1_ARGS(EX)       (void)((EX) || (_assertFailWrapper1(#EX, __FILE__, __LINE__), 0))
#define ASSERT_2_ARGS(EX, CODE) (void)((EX) || (_assertFailWrapper2(#EX, __FILE__, __LINE__, CODE), 0))

// Macro magic to call different functions depending on # of arguments
#define ASSERT_GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
#define ASSERT_MACRO_CHOOSER(...) ASSERT_GET_3RD_ARG(__VA_ARGS__, ASSERT_2_ARGS, ASSERT_1_ARGS)

#define assert(...) ASSERT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)