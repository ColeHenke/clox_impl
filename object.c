#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

// allocate an object of a given size on the heap
// size is not just the object - it includes room for extra payload fields
static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  // insert an Obj to the intrusive list
  object->next = vm.objects;
  vm.objects = object;

  return object;
}

// create a new ObjString on the heap and initializes it's fields
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  // intern every string automatically
  tableSet(&vm.strings, string, NIL_VAL);

  return string;
}

// `FNV-1a` hashing algorithm
static uint32_t hashString(const char* key, int length) {
  // start with an initial hash value
  uint32_t hash = 2166136261u;

  // walk data to be hashed
  // for each byte or word, mix the bits in the hash value and scramble the result bits around
  // we want a uniformity in the resulting distribution of values
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

// unlike `copyString()` this takes ownership of the string given to it
ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  // look up the string in the string table first
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

  // if found, free the memory for the passed in string
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocateString(chars, length, hash);
}

// ObjString can't point back to the original characters in the source string
// some ObjString's are created dynamically at runtime and thus memory for it must be freed when it's no longer needed
// even string literals are added to the heap
// assumes that it cannot take ownership of the characters passed in
ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  // look up the string in the string table first
  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);

  // return a reference to the string if found
  if (interned != NULL) return interned;

  // allocate space for a new array on the heap
  char* heapChars = ALLOCATE(char, length + 1);

  // move chars to the heap
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0'; // we need to terminate the string ourselves b/c the lexeme points at a range of
                            // chars inside a monolithic source string that isn't terminated
  return allocateString(heapChars, length, hash);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  }
}