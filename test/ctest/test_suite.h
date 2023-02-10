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
            fprintf(stderr, "\n\033[1;31mAssertion failed; %s (%d) != %s (%d) @ line [%d] file [%s]\n\033[0m", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkLongsEqual(LHS, RHS) \
    do {                          \
        long lhs = (LHS);         \
        long rhs = (long) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "\n\033[1;31mAssertion failed; %s (%zu) != %s (%zu) @ line [%d] file [%s]\n\033[0m", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkPtrsEqual(LHS, RHS) \
    do {                         \
        void* lhs = (void*) (LHS); \
        void* rhs = (void*) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "\n\033[1;31mAssertion failed; %s (%p) != %s (%p) @ line [%d] file [%s]\n\033[0m", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkFloatsEqual(LHS, RHS) \
    do {                           \
        double lhs = (LHS);        \
        double rhs = (double) (RHS);\
        if (lhs != rhs) { \
            fprintf(stderr, "\n\033[1;31mAssertion failed; %s (%g) != %s (%g) @ line [%d] file [%s]\n\033[0m", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;             \
        }                 \
    } while(0)

#define checkStringsEqual(LHS, RHS) \
    do {                            \
        const char* lhs = (LHS);                                \
        const char* rhs = (RHS);    \
        if (strlen(lhs) != strlen(rhs) || memcmp(lhs, rhs, strlen(lhs)) != 0) { \
            fprintf(stderr, "\n\033[1;31mAssertion failed; %s (%s) != %s (%s) @ line [%d] file [%s]\n\033[0m", #LHS, lhs, #RHS, rhs, __LINE__, __FILE__); \
            err_code = TEST_FAILED;                                     \
        }                                \
    } while(0)

#define checkTrue(statement) \
do {                         \
    if (!statement) {        \
        fprintf(stderr, "\n\033[1;31mExpected " #statement " to be true, but it was false @ line [%d] file " __FILE__ "\n\033[0m", __LINE__);                         \
    }                             \
} while(0)
#define assertNotNull(ptr) \
    do {                   \
        if (!(ptr)) {      \
            fprintf(stderr, "\n\033[1;31mAssertion failed; pointer %s was null @ line [%d] file [%s]\n\033[0m", #ptr, __LINE__, __FILE__); \
            return TEST_FAILED;      \
        }                  \
    } while(0)

#endif //CLOX_TEST_SUITE_H
