#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  // returns a number to tell the caller the offset of the beginning of the next instruction
  // OP_CONSTANT occupies two bytes, so the offset is incremented 2
  return offset + 2;
}

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  // returns a number to tell the caller the offset of the beginning of the next instruction
  // OP_RETURN occupies one byte, so the offset is incremented by 1
  return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("Unknown opcode %d", instruction);
      return offset + 1;
  }
}