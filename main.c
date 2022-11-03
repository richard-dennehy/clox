#include "chunk.h"
#include "debug.h"

int main(int argc, const char** argv) {
    Chunk chunk;
    initChunk(&chunk);

    for (int i = 0; i < 257; i++) {
        writeConstant(&chunk, i * 2, i + 1);
    }

    writeChunk(&chunk, OP_RETURN, 258);

    disassembleChunk(&chunk, "Test chunk");
    freeChunk(&chunk);

    return 0;
}