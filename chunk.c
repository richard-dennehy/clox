#include <assert.h>
#include "chunk.h"

void initChunk(VM* vm, Compiler* compiler, Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->firstLine = NULL;
    initValueArray(vm, compiler, &chunk->constants);
}

void freeChunk(VM* vm, Chunk* chunk) {
    VM_FREE_ARRAY(uint8_t, chunk->code, chunk->count);
    for (Line* line = chunk->firstLine; line;) {
        Line* next = line->next;
        VM_FREE(Line, line);
        line = next;
    }
    freeValueArray(vm, &chunk->constants);
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->firstLine = NULL;
}

static Line* newLine(VM* vm, Compiler* compiler, uint32_t lineNumber) {
    Line* line = NULL;
    line = COMPILER_ALLOCATE(Line, 1);
    assert(line);
    line->lineNumber = lineNumber;
    line->instructions = 0;
    line->next = NULL;

    return line;
}

static void writeLine(VM* vm, Compiler* compiler, Chunk* chunk, uint32_t line) {
    if (!chunk->firstLine) {
        chunk->firstLine = newLine(vm, compiler, line);
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
        Line* nextLine = newLine(vm, compiler, line);
        nextLine->instructions += 1;

        // assuming that line numbers never decrease, otherwise this will overwrite existing links
        assert(!closest->next);
        closest->next = nextLine;
    }
}

void writeChunk(VM* vm, Compiler* compiler, Chunk* chunk, uint8_t byte, uint32_t line) {
    if (chunk->capacity < chunk->count + 1) {
        uint32_t oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = COMPILER_GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    writeLine(vm, compiler, chunk, line);

    chunk->count++;
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
    if (line == UINT32_MAX) {
        assert(!"Oh no");
    }
    return line;
}
