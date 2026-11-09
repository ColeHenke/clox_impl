#ifndef CLOX_IMPL_VM_H
#define CLOX_IMPL_VM_H

#include "chunk.h"

typedef struct {
  Chunk* chunk;
  // points to the instruction about to be executed
  uint8_t* ip;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);

#endif //CLOX_IMPL_VM_H
