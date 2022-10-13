#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
uint32_t disassembleInstruction(Chunk* chunk, uint32_t offset);

#endif //CLOX_DEBUG_H
