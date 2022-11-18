#include <assert.h>
#include "chunk.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->firstLine = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(FreeList* freeList, Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->count);
    for (Line* line = chunk->firstLine; line;) {
        Line* next = line->next;
        reallocate(freeList, line, sizeof(Line), 0);
        line = next;
    }
    freeValueArray(freeList, &chunk->constants);
    initChunk(chunk);
}

static Line* newLine(FreeList* freeList, uint32_t lineNumber) {
    Line* line = NULL;
    line = reallocate(freeList, line, 0, sizeof(Line));
    assert(line);
    line->lineNumber = lineNumber;
    line->instructions = 0;
    line->next = NULL;

    return line;
}

static void writeLine(FreeList* freeList, Chunk* chunk, uint32_t line) {
    if (!chunk->firstLine) {
        chunk->firstLine = newLine(freeList, line);
    }

    Line* closest = chunk->firstLine;
    while (closest && closest->next) {
        if (closest->lineNumber == line) break;
        closest = closest->next;
    }
    assert(closest);

    if (closest->lineNumber == line) {
        closest->instructions += 1;
    } else {
        Line* nextLine = newLine(freeList, line);
        nextLine->instructions += 1;

        // assuming that line numbers never decrease, otherwise this will overwrite existing links
        assert(!closest->next);
        closest->next = nextLine;
    }
}

void writeChunk(FreeList* freeList, Chunk* chunk, uint8_t byte, uint32_t line) {
    if (chunk->capacity < chunk->count + 1) {
        int32_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    writeLine(freeList, chunk, line);

    chunk->count++;
}

void writeConstant(FreeList* freeList, Chunk* chunk, Value value, uint32_t line) {
    writeValue(freeList, &chunk->constants, value);
    uint32_t index = chunk->constants.count - 1;

    if (index <= 255) {
        writeChunk(freeList, chunk, OP_CONSTANT, line);
        writeChunk(freeList, chunk, index, line);
    } else {
        assert(index < 1 << 24);

        writeChunk(freeList, chunk, OP_CONSTANT_LONG, line);
        writeChunk(freeList, chunk, (uint8_t) (index >> 16), line);
        writeChunk(freeList, chunk, (uint8_t) (index >> 8), line);
        writeChunk(freeList, chunk, (uint8_t) (index >> 0), line);
    }
}

uint32_t getLine(Chunk* chunk, uint32_t instructionIndex) {
    uint32_t line = UINT32_MAX;
    for (Line* nextLine = chunk->firstLine; nextLine; nextLine = nextLine->next) {
        if (instructionIndex <= nextLine->instructions) {
            line = nextLine->lineNumber;
            break;
        }
        instructionIndex -= nextLine->instructions;
    }

    // hopefully nobody's writing 4 billion lines of lox code
    assert(line != UINT32_MAX);
    return line;
}
