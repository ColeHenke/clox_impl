#ifndef CLOX_IMPL_VALUE_H
#define CLOX_IMPL_VALUE_H

#include "common.h"

// this will abstract how values are concretely represented in c
// that way, the representation can be changed w/o having ot go back and fix existing code that passes value around
typedef double Value;

// stores constants found in the program
typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif //CLOX_IMPL_VALUE_H
