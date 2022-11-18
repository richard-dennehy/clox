#ifndef CLOX_TEST_SUITE_H
#define CLOX_TEST_SUITE_H

#include "stdio.h"

#define TEST_FAILED 1
#define TEST_SUCCEEDED 0

// will make the test fail if LHS != RHS, but continues running the test
#define checkIntsEqual(LHS, RHS) \
    do {                         \
        int lhs = (LHS);         \
        int rhs = (int) (RHS);                         \
        if (lhs != rhs) { \
            fprintf(stderr, "Assertion failed; %s (%d) != %s (%d) @ line [%d] file [%s]\n", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkLongsEqual(LHS, RHS) \
    do {                          \
        long lhs = (LHS);         \
        long rhs = (long) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "Assertion failed; %s (%zu) != %s (%zu) @ line [%d] file [%s]\n", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkPtrsEqual(LHS, RHS) \
    do {                         \
        void* lhs = (void*) (LHS); \
        void* rhs = (void*) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "Assertion failed; %s (%p) != %s (%p) @ line [%d] file [%s]\n", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkFloatsEqual(LHS, RHS) \
    do {                           \
        double lhs = (LHS);        \
        double rhs = (double) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "Assertion failed; %s (%g) != %s (%g) @ line [%d] file [%s]\n", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define assertNotNull(ptr) \
    do {                   \
        if (!(ptr)) {      \
            fprintf(stderr, "Assertion failed; pointer %s was null @ line [%d] file [%s]\n", #ptr, __LINE__, __FILE__); \
            return TEST_FAILED;      \
        }                  \
    } while(0)

#endif //CLOX_TEST_SUITE_H
