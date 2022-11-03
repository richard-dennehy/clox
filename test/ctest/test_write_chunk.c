#include "chunk.h"
#include "test_suite.h"

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

int testWriteConstant() {
    int err_code = TEST_SUCCEEDED;

    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 123, 1);
    checkEqual(chunk.count, 2);
    assertNotNull(chunk.code);
    checkEqual(chunk.code[0], OP_CONSTANT);
    checkEqual(chunk.code[1], 0);

    checkEqual(chunk.constants.count, 1);
    assertNotNull(chunk.constants.values);
    checkEqual(chunk.constants.values[0], 123);

    for (int i = 0; i < 256; i++) {
        writeConstant(&chunk, i * 2, i + 1);
    }

    checkEqual(chunk.constants.count, 257);
    checkEqual(chunk.count, 516);
    assertNotNull(chunk.code);
    checkEqual(chunk.code[512], OP_CONSTANT_LONG);
    checkEqual(chunk.code[513], 0);
    checkEqual(chunk.code[514], 1);
    checkEqual(chunk.code[515], 0);

    freeChunk(&chunk);

    return err_code;
}

int main() {
    return testWriteChunk() | testWriteConstant();
}