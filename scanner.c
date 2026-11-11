#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

// where is the scanner in the file
typedef struct {
  // beginning of the current lexeme being scanned
  const char* start;
  // current char being looked at
  const char* current;
  // line number for error reporting
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  // `start` and `current` pointers should point to the beginning of the file (`source` pointer) duh!
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

