#include "chunk.h"
#include "debug.h"

int main(int argc, const char** argv) {
    FreeList freeList;
    initMemory(&freeList, 64 * 1024 * 1024);
    Chunk chunk;
    initChunk(&chunk);

    for (int i = 0; i < 257; i++) {
        writeConstant(&freeList, &chunk, i * 2, i + 1);
    }

    writeChunk(&freeList, &chunk, OP_RETURN, 258);

    disassembleChunk(&chunk, "Test chunk");
    freeChunk(&freeList, &chunk);
    freeMemory(&freeList);

    return 0;
}