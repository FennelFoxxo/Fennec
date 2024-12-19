#include "assert.h"

#include <stdio.h>

#define ERROR_MESSAGE_BUFFER_SIZE 1000

static void defaultAssertFailCallback(const char* err) {
    printf("%s\n", err);
    while (1);
}

static AssertFailCallback _assert_fail_callback = defaultAssertFailCallback;
static char _err[ERROR_MESSAGE_BUFFER_SIZE];


void setAssertFailCallback(AssertFailCallback f) {
    _assert_fail_callback = f;
}

void _assertFailWrapper1(const char* ex, const char* file, int line) {
    snprintf(_err, ERROR_MESSAGE_BUFFER_SIZE, "Failed assertion: '%s'\n    in %s:%d\n", ex, file, line);
    _assert_fail_callback(_err);
}

void _assertFailWrapper2(const char* ex, const char* file, int line, int code) {
    snprintf(_err, ERROR_MESSAGE_BUFFER_SIZE, "Failed assertion: '%s'\n    With error code %d\n    in %s:%d\n", ex, code, file, line);
    _assert_fail_callback(_err);
}