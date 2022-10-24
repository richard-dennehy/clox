#include "chunk.h"
#include "assert_macro.h"

int testWriteChunk() {
    int err_code = 0;
    Chunk chunk;
    initChunk(&chunk);

    assertEqual(chunk.count, 0);
    assertEqual(chunk.capacity, 0);
    assertEqual(chunk.lines, NULL);
    assertEqual(chunk.code, NULL);

    writeChunk(&chunk, OP_RETURN, 0);

    assertEqual(chunk.count, 1);
    assertEqual(chunk.capacity, 8);
    // TODO actually test this, probably
    assertNotEqual(chunk.lines, NULL);
    assertNotEqual(chunk.code, NULL);

    for (int i = 0; i < 8; ++i) {
        writeChunk(&chunk, OP_RETURN, i);
    }

    assertEqual(chunk.count, 9);
    assertEqual(chunk.capacity, 16);
    assertNotEqual(chunk.lines, NULL);
    assertNotEqual(chunk.code, NULL);

    freeChunk(&chunk);
    return err_code;
}

int main() {
    return testWriteChunk();
}