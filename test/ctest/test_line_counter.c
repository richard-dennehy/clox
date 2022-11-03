#include "chunk.h"
#include "test_suite.h"

int testLineCounter() {
    int err_code = TEST_SUCCEEDED;
    Chunk chunk;
    initChunk(&chunk);

    for (int i = 0; i < 5; i++) {
        writeChunk(&chunk, OP_RETURN, 10);
    }
    writeChunk(&chunk, OP_RETURN, 11);
    for (int i = 0; i < 100; i++) {
        writeChunk(&chunk, OP_RETURN, 99);
    }

    checkEqual(chunk.count, 106);
    checkEqual(chunk.capacity, 128);

    Line* nextLine = chunk.firstLine;
    assertNotNull(nextLine);
    checkEqual(nextLine->lineNumber, 10);
    checkEqual(nextLine->instructions, 5);

    nextLine = nextLine->next;
    assertNotNull(nextLine);
    checkEqual(nextLine->lineNumber, 11);
    checkEqual(nextLine->instructions, 1);

    nextLine = nextLine->next;
    assertNotNull(nextLine);
    checkEqual(nextLine->lineNumber, 99);
    checkEqual(nextLine->instructions, 100);
    checkEqual(nextLine->next, NULL);

    freeChunk(&chunk);

    return err_code;
}

int main() {
    return testLineCounter();
}