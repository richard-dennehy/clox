#ifndef CLOX_ASSERT_MACRO_H
#define CLOX_ASSERT_MACRO_H

#include "stdio.h"

#define TEST_FAILED 1
#define TEST_SUCCEEDED 0

// will make the test fail if LHS != RHS, but continues running the test
#define checkEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            printf("Assertion failed; %s != %s @ line [%d] file [%s]", #LHS, #RHS, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

// will make the test fail if LHS == RHS, but continues running the test
#define checkNotEqual(LHS, RHS) \
    do {                 \
        if ((LHS) == (RHS)) { \
            printf("Assertion failed; %s == %s @ line [%d] file [%s]", #LHS, #RHS, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define assertNotNull(ptr) \
    do {                   \
        if (!(ptr)) {      \
            printf("Assertion failed; pointer %s was null @ line [%d] file [%s]", #ptr, __LINE__, __FILE__); \
            return TEST_FAILED;      \
        }                  \
    } while(0)

#endif //CLOX_ASSERT_MACRO_H
