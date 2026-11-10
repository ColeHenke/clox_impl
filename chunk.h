#ifndef CLOX_IMPL_CHUNK_H
#define CLOX_IMPL_CHUNK_H

#include "common.h"
#include "value.h"

// "chunk" = sequence of bytecode
typedef enum {
  // two bytes - opcode and constant index
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  // return from current function
  // one byte - opcode
  OP_RETURN
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  ValueArray constants;
  // this will parallel the bytecode array
  int* lines;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_IMPL_CHUNK_H
