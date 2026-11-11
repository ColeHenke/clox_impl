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

// check if char is digit by comparing ascii codes
static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

// consume the next char and return it
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}

static char peek() {
  return *scanner.current;
}

// peek ahead one character - `peek()` only looks at the current char
static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
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

// whitespace is not meaningful to us
static void skipWhitespace() {
  for (;;) {
    // ensure no non-whitespace chars are consumed by peeking first
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      // encountering a newline means that we increment the line number
      scanner.line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        // a comment goes until the end of the line - consume it all
        // don't consume `\n` right away. instead leave it so the next iteration will fire on `case '\n'` and then
        //    increment the line number
        while (peek() != '\n' && !isAtEnd()) advance();
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}

static Token string() {
  // consume all chars between opening `"` and closing `"`
  while (peek() != '"' && !isAtEnd()) {
    // support multiline strings
    if (peek() == '\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // the closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken() {
  skipWhitespace();

  // each call to this function scans a complete token, each calls means we are at the beginning of a new token
  scanner.start = scanner.current;

  // if at end of source code, return EOF and stop
  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();

  if (isDigit(c)) return number();

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);

    // if we see a `!`, `<`, or `>`, we need to check to see if the next token is an `=`
    case '!':
      return makeToken(
          match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(
          match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(
          match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(
          match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    case '"': return string();
  }

  // if this line is reached, we don't know the current token, so we throw an error
  return errorToken("Unexpected Character.");
}

