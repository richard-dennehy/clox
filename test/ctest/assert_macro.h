#ifndef CLOX_ASSERT_MACRO_H
#define CLOX_ASSERT_MACRO_H

#include "stdio.h"

#define assertEqual(LHS, RHS) \
    do {                 \
        if ((LHS) != (RHS)) { \
            printf("Assertion failed; %s != %s @ line [%d] file [%s]", #LHS, #RHS, __LINE__, __FILE__); \
            err_code = 1;             \
        }                 \
    } while(0)

#define assertNotEqual(LHS, RHS) \
    do {                 \
        if ((LHS) == (RHS)) { \
            printf("Assertion failed; %s == %s @ line [%d] file [%s]", #LHS, #RHS, __LINE__, __FILE__); \
            err_code = 1;             \
        }                 \
    } while(0)

#endif //CLOX_ASSERT_MACRO_H
