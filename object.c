#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

// allocate an object of a given size on the heap
// size is not just the object - it includes room for extra payload fields
static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  return object;
}

// create a new ObjString on the heap and initializes it's fields
static ObjString* allocateString(char* chars, int length) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

// ObjString can't point back to the original characters in the source string
// some ObjString's are created dynamically at runtime and thus memory for it must be freed when it's no longer needed
// even string literals are added to the heap
ObjString* copyString(const char* chars, int length) {
  // allocate space for a new array on the heap
  char* heapChars = ALLOCATE(char, length + 1);

  // move chars to the heap
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0'; // we need to terminate the string ourselves b/c the lexeme points at a range of
                            // chars inside a monolithic source string that isn't terminated
  return allocateString(heapChars, length);
}
