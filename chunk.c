#include <assert.h>
#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->firstLine = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->count);
    for (Line* line = chunk->firstLine; line;) {
        Line* next = line->next;
        reallocate(line, sizeof(Line), 0);
        line = next;
    }
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

static void writeLine(Chunk* chunk, uint32_t line) {
    Line** closest = &chunk->firstLine;
    while (*closest && (*closest)->next) {
        if ((*closest)->lineNumber == line) break;
        closest = &(*closest)->next;
    }

    if (!*closest) {
        *closest = reallocate(*closest, 0, sizeof(Line));
        assert(*closest);
        (*closest)->lineNumber = line;
        (*closest)->instructions = 0;
        (*closest)->next = NULL;
    }

    if ((*closest)->lineNumber == line) {
        (*closest)->instructions += 1;
    } else {
        Line* nextLine = NULL;
        nextLine = reallocate(nextLine, 0, sizeof(Line));
        assert(nextLine);
        nextLine->lineNumber = line;
        nextLine->instructions = 1;
        nextLine->next = NULL;

        // assuming that line numbers never decrease, otherwise this will overwrite existing links
        assert(!(*closest)->next);
        (*closest)->next = nextLine;
    }
}

void writeChunk(Chunk* chunk, uint8_t byte, uint32_t line) {
    if (chunk->capacity < chunk->count + 1) {
        int32_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    writeLine(chunk, line);

    chunk->count++;
}

uint32_t getLine(Chunk* chunk, uint32_t instructionIndex) {
    uint32_t line = UINT32_MAX;
    for (Line* nextLine = chunk->firstLine; nextLine; nextLine = nextLine->next) {
        if (instructionIndex < nextLine->instructions) {
            line = nextLine->lineNumber;
            break;
        }
        instructionIndex -= nextLine->instructions;
    }

    // hopefully nobody's writing 4 billion lines of lox code
    assert(line != UINT32_MAX);
    return line;
}

int32_t addConstant(Chunk* chunk, Value value) {
    writeValue(&chunk->constants, value);
    return chunk->constants.count - 1;
}

