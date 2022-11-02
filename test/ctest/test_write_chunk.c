#include "chunk.h"
#include "assert_macro.h"

int testWriteChunk() {
    int err_code = TEST_SUCCEEDED;
    Chunk chunk;
    initChunk(&chunk);

    checkEqual(chunk.count, 0);
    checkEqual(chunk.capacity, 0);
    checkEqual(chunk.firstLine, NULL);
    checkEqual(chunk.code, NULL);

    writeChunk(&chunk, OP_RETURN, 0);

    checkEqual(chunk.count, 1);
    checkEqual(chunk.capacity, 8);
    assertNotNull(chunk.code);
    checkEqual(chunk.code[0], OP_RETURN);

    assertNotNull(chunk.firstLine);
    checkEqual(chunk.firstLine[0].lineNumber, 0);
    checkEqual(chunk.firstLine[0].instructions, 1);
    checkEqual(chunk.firstLine[0].next, NULL);

    for (int i = 1; i < 9; ++i) {
        writeChunk(&chunk, OP_RETURN, i);
    }

    checkEqual(chunk.count, 9);
    checkEqual(chunk.capacity, 16);
    assertNotNull(chunk.code);

    uint32_t length = 0;
    for (Line* nextLine = chunk.firstLine; nextLine; nextLine = nextLine->next) {
        checkEqual(nextLine->lineNumber, length);
        checkEqual(nextLine->instructions, 1);
        length++;
    }
    checkEqual(length, 9);

    freeChunk(&chunk);
    return err_code;
}

int main() {
    return testWriteChunk();
}