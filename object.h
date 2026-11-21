#ifndef CLOX_IMPL_OBJECT_H
#define CLOX_IMPL_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// determine if casts from Obj* to ObjString* are safe
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// take a Value that is expected to contain a pointer to a valid ObjString on the heap add retune a ObjString*
// the second on steps thru that ot return the char array
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  // points to the next Obj in the chain
  struct Obj* next;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  // cache the strings hash code
  uint32_t hash;
};

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

// have this logic in a separate function because it uses `value` twice
// since macros are expanded by inserting the argument expression every place the parameter name appears
//    in the body. if a macro uses a parameter more that once, the expression is evaluated multiple times
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_IMPL_OBJECT_H
