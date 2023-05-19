#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC
#define UINT8_COUNT (UINT8_MAX + 1)
#define UNUSED __attribute__((__unused__))

typedef int (Printer)(const char* format, ...);
typedef struct VM VM;
typedef struct Compiler Compiler;

#endif //CLOX_COMMON_H
