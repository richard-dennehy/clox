#include "chunk.h"
#include "test_suite.h"

int testWriteChunk() {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    initMemory(&freeList, 256 * 1024);
    Chunk chunk;
    initChunk(&chunk);

    checkIntsEqual(chunk.count, 0);
    checkIntsEqual(chunk.capacity, 0);
    checkPtrsEqual(chunk.firstLine, NULL);
    checkPtrsEqual(chunk.code, NULL);

    writeChunk(&freeList, &chunk, OP_RETURN, 0);

    checkIntsEqual(chunk.count, 1);
    checkIntsEqual(chunk.capacity, 8);
    assertNotNull(chunk.code);
    checkIntsEqual(chunk.code[0], OP_RETURN);

    assertNotNull(chunk.firstLine);
    checkIntsEqual(chunk.firstLine[0].lineNumber, 0);
    checkIntsEqual(chunk.firstLine[0].instructions, 1);
    checkPtrsEqual(chunk.firstLine[0].next, NULL);

    for (int i = 1; i < 9; ++i) {
        writeChunk(&freeList, &chunk, OP_RETURN, i);
    }

    checkIntsEqual(chunk.count, 9);
    checkIntsEqual(chunk.capacity, 16);
    assertNotNull(chunk.code);

    uint32_t length = 0;
    for (Line* nextLine = chunk.firstLine; nextLine; nextLine = nextLine->next) {
        checkIntsEqual(nextLine->lineNumber, length);
        checkIntsEqual(nextLine->instructions, 1);
        length++;
    }
    checkIntsEqual(length, 9);

    freeChunk(&freeList, &chunk);
    freeMemory(&freeList);

    return err_code;
}

int main() {
    return testWriteChunk();
}
