#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

// takes a bucket of entries and finds out which one the key belongs in
// looks up existing entries and decides where to insert new ones
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {

  // use modulo to map the key's hash code to an index w/in the array bounds
  // that will yield a bucket index where we can find or place the entry
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // we found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // we found the key.
      return entry;
    }

    // reaching here means the bucket has a different key in here - linear probe time
    // the modulo here wraps us around if we go outside the array bounds
    index = (index + 1) % capacity;
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  // if table is empty, the value can't be in it
  if (table->count == 0) return false;

  // look for key in buckets
  Entry* entry = findEntry(table->entries, table->capacity, key);

  // not present in bucket array
  if (entry->key == NULL) return false;

  // present
  *value = entry->value;
  return true;
}

// change the capacity of our hash table
static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  // create new empty buckets
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // don't copy over tombstones to new bucket array
  table->count = 0;
  // rebuild the table from scratch by re-inserting every entry into the new empty array to avoid having new collisions
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    // increment the count only on non-tombstone entries
    table->count++;
  }

  // release memory from the old array
  FREE_ARRAY(Entry, table->entries, table->capacity);

  // store new array and capacity
  table->entries = entries;
  table->capacity = capacity;
}

// add a given key-value pair to the hash table
bool tableSet(Table* table, ObjString* key, Value value) {

  // make sure there is capacity to add an item when the table is more than 75% capacity
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  // see if the key already exists in the table
  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;

  // if key is new, the table count goes up by one (duh)
  if (isNewKey && IS_NIL(entry->value)) table->count++;

  // add the key-value pair
  entry->key = key;
  entry->value = value;
  return isNewKey;
}

// walks the bucket array of the source hash table
// whenever it finds a non-empty bucket, it adds the entry to the destination hash table
void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = &table->entries[index];
    if (entry->key == NULL) {
      // stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
      // we found it.
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // place a tombstone in the entry.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}