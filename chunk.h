#ifndef CLOX_IMPL_CHUNK_H
#define CLOX_IMPL_CHUNK_H

#include "common.h"

// "chunk" = sequence of bytecode
typedef enum {
  // return from current function
  OP_RETURN
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);
void freeChunk(Chunk* chunk);

#endif //CLOX_IMPL_CHUNK_H
