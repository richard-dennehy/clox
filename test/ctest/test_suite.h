#ifndef CLOX_TEST_SUITE_H
#define CLOX_TEST_SUITE_H

#include "stdio.h"

#define TEST_FAILED 1
#define TEST_SUCCEEDED 0

// will make the test fail if LHS != RHS, but continues running the test
#define checkIntsEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            fprintf(stderr, "Assertion failed; %s (%d) != %s (%d) @ line [%d] file [%s]", #LHS, (LHS), #RHS, (RHS), __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkLongsEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            fprintf(stderr, "Assertion failed; %s (%zu) != %s (%zu) @ line [%d] file [%s]", #LHS, (LHS), #RHS, (unsigned long)(RHS), __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkPtrsEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            fprintf(stderr, "Assertion failed; %s (%p) != %s (%p) @ line [%d] file [%s]", #LHS, (LHS), #RHS, (RHS), __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkFloatsEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            fprintf(stderr, "Assertion failed; %s (%g) != %s (%g) @ line [%d] file [%s]", #LHS, (LHS), #RHS, (double)(RHS), __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define assertNotNull(ptr) \
    do {                   \
        if (!(ptr)) {      \
            fprintf(stderr, "Assertion failed; pointer %s was null @ line [%d] file [%s]", #ptr, __LINE__, __FILE__); \
            return TEST_FAILED;      \
        }                  \
    } while(0)

#endif //CLOX_TEST_SUITE_H
