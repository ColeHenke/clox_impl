#ifndef CLOX_IMPL_DEBUG_H
#define CLOX_IMPL_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif //CLOX_IMPL_DEBUG_H
