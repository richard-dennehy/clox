#include "test_suite.h"
#include "vm.c"

int testVmStack() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 16 * 1024);
    VM vm;
    initVM(&vm);

    Value v0 = 1.0;
    push(&freeList, &vm, v0);
    checkFloatsEqual(pop(&vm), v0);

    Value v1 = 2.0;
    Value v2 = 4.0;
    push(&freeList, &vm, v1);
    push(&freeList, &vm, v2);

    checkFloatsEqual(pop(&vm), v2);
    checkFloatsEqual(pop(&vm), v1);

    for (int i = 0; i < 257; i++) {
        push(&freeList, &vm, i);
    }
    checkFloatsEqual(pop(&vm), 256);

    freeVM(&freeList, &vm);
    freeMemory(&freeList);
    return err_code;
}

static void resetChunk(FreeList* freeList, Chunk* chunk) {
    freeChunk(freeList, chunk);
    initChunk(chunk);
}

int testVmArithmetic() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    Chunk chunk;
    initMemory(&freeList, 16 * 1024);
    initVM(&vm);
    initChunk(&chunk);

    writeConstant(&freeList, &chunk, 2.0, 1);
    writeChunk(&freeList, &chunk, OP_NEGATE, 1);
    interpretChunk(&freeList, &vm, &chunk);
    checkFloatsEqual(vm.stack.values[0], -2.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 3.0, 2);
    writeConstant(&freeList, &chunk, 4.0, 2);
    writeChunk(&freeList, &chunk, OP_MULTIPLY, 2);
    interpretChunk(&freeList, &vm, &chunk);
    checkFloatsEqual(vm.stack.values[1], 12.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 5.0, 3);
    writeConstant(&freeList, &chunk, 6.0, 3);
    writeChunk(&freeList, &chunk, OP_ADD, 3);
    interpretChunk(&freeList, &vm, &chunk);
    checkFloatsEqual(vm.stack.values[2], 11.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 7.0, 4);
    writeConstant(&freeList, &chunk, 8.0, 4);
    writeChunk(&freeList, &chunk, OP_SUBTRACT, 4);
    interpretChunk(&freeList, &vm, &chunk);
    checkFloatsEqual(vm.stack.values[3], -1.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 9.0, 5);
    writeConstant(&freeList, &chunk, 10.0, 5);
    writeChunk(&freeList, &chunk, OP_DIVIDE, 5);
    interpretChunk(&freeList, &vm, &chunk);
    checkFloatsEqual(vm.stack.values[4], 0.9);

    freeChunk(&freeList, &chunk);
    freeVM(&freeList, &vm);
    freeMemory(&freeList);

    return err_code;
}

int main() {
    return testVmStack() | testVmArithmetic();
}