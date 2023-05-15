#include "chunk.h"
#include "compiler.c"
#include "test_suite.h"

int testLineCounter(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    initMemory(&freeList, 256 * 1024);
    VM vm = {.freeList = &freeList};
    Chunk chunk;
    initChunk(&vm, NULL, &chunk);

    for (int i = 0; i < 5; i++) {
        writeChunk(&vm, NULL, &chunk, OP_RETURN, 10);
    }
    writeChunk(&vm, NULL, &chunk, OP_RETURN, 11);
    for (int i = 0; i < 100; i++) {
        writeChunk(&vm, NULL, &chunk, OP_RETURN, 99);
    }

    checkIntsEqual(chunk.count, 106);
    checkIntsEqual(chunk.capacity, 128);

    Line* nextLine = chunk.firstLine;
    assertNotNull(nextLine);
    checkIntsEqual(nextLine->lineNumber, 10);
    checkIntsEqual(nextLine->instructions, 5);

    nextLine = nextLine->next;
    assertNotNull(nextLine);
    checkIntsEqual(nextLine->lineNumber, 11);
    checkIntsEqual(nextLine->instructions, 1);

    nextLine = nextLine->next;
    assertNotNull(nextLine);
    checkIntsEqual(nextLine->lineNumber, 99);
    checkIntsEqual(nextLine->instructions, 100);
    checkPtrsEqual(nextLine->next, NULL);

    freeChunk(&vm, &chunk);
    freeMemory(&freeList);

    return err_code;
}

int main(void) {
    return testLineCounter();
}
