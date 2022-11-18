#include "test_suite.h"
#include "vm.h"

int testVmStack() {
    int err_code = TEST_SUCCEEDED;

    VM vm;
    initVM(&vm);

    Value v0 = 1.0;
    push(&vm, v0);
    checkFloatsEqual(pop(&vm), v0);

    Value v1 = 2.0;
    Value v2 = 4.0;
    push(&vm, v1);
    push(&vm, v2);

    checkFloatsEqual(pop(&vm), v2);
    checkFloatsEqual(pop(&vm), v1);

    for (int i = 0; i < STACK_MAX; i++) {
        push(&vm, i);
    }
    checkFloatsEqual(pop(&vm), STACK_MAX - 1);

    freeVM(&vm);
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
    interpret(&vm, &chunk);
    checkFloatsEqual(vm.stack[0], -2.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 3.0, 2);
    writeConstant(&freeList, &chunk, 4.0, 2);
    writeChunk(&freeList, &chunk, OP_MULTIPLY, 2);
    interpret(&vm, &chunk);
    checkFloatsEqual(vm.stack[1], 12.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 5.0, 3);
    writeConstant(&freeList, &chunk, 6.0, 3);
    writeChunk(&freeList, &chunk, OP_ADD, 3);
    interpret(&vm, &chunk);
    checkFloatsEqual(vm.stack[2], 11.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 7.0, 4);
    writeConstant(&freeList, &chunk, 8.0, 4);
    writeChunk(&freeList, &chunk, OP_SUBTRACT, 4);
    interpret(&vm, &chunk);
    checkFloatsEqual(vm.stack[3], -1.0);
    resetChunk(&freeList, &chunk);

    writeConstant(&freeList, &chunk, 9.0, 5);
    writeConstant(&freeList, &chunk, 10.0, 5);
    writeChunk(&freeList, &chunk, OP_DIVIDE, 5);
    interpret(&vm, &chunk);
    checkFloatsEqual(vm.stack[4], 0.9);

    freeChunk(&freeList, &chunk);
    freeVM(&vm);
    freeMemory(&freeList);

    return err_code;
}

int main() {
    return testVmStack() | testVmArithmetic();
}