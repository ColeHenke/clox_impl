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

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

Token scanToken() {
  // each call to this function scans a complete token, each calls means we are at the beginning of a new token
  scanner.start = scanner.current;

  // if at end of source code, return EOF and stop
  if (isAtEnd()) return makeToken(TOKEN_EOF);

  // if this line is reached, we don't know the current token, so we throw an error
  return errorToken("Unexpected Character.");
}

