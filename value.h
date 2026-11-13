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

// use these to check a Value's type before converting to a C value
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)

// unpack a dynamic lox Value to a static C value
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
// no AS_NIL macro because there is only one nil value, so a Value with type VAL_NIL doesn't carry any extra data.

// promote a native C value to a clox Value
// static C values to dynamic lox Values
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

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
