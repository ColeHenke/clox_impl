#ifndef CLOX_IMPL_VALUE_H
#define CLOX_IMPL_VALUE_H

#include "common.h"

// this is how we'll store a value's type
typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

// this will abstract how values are concretely represented in c
// that way, the representation can be changed w/o having ot go back and fix existing code that passes value around
// this currently occupies 16 bytes
typedef struct {
  ValueType type; // 8 bytes (4 for the actual value + 4 of padding)
  // these share the same bits in memory
  union {
    bool boolean;
    double number;
  } as; // 8 bytes
} Value;

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
