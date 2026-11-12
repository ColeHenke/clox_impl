#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"

#include "scanner.h"

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  // since c doesn't have exceptions, this flag will be used to suppress any cascaded errors until a
  //    `synchronization point` is reached, which will be statement boundaries in this case
  bool panicMode;
} Parser;

Parser parser;

Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
  // if parser is in panic mode, don't return any more error info
  if (parser.panicMode) return;

  parser.panicMode = true;
  // print where the error occurred
  fprintf(stderr, "[line %d] Error", token->line);

  // show lexeme is human-readable
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // noop
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  // print error message itself
  fprintf(stderr, ": %s\n", message);

  // records if any errors occurred during compilation (duh)
  parser.hadError = true;
}

// error at previous token - more common than error at current token
static void error(const char* message) {
  errorAt(&parser.previous, message);
}

// error on current token
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
  // good for getting the lexeme after matching a token
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    // since clox doesn't report lexical errors, the parser must report them
    if (parser.current.type == TOKEN_ERROR) break;
    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  // if token type is not the expected type, error
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

// add a single byte to the chunk - could be an opcode or an operand
static void emitByte(uint8_t byte) {
  // pass the previous line as well so token errors are associated with that line
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitReturn() {
  emitByte(OP_RETURN);
}

static void endCompiler() {
  emitReturn();
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression");
  endCompiler();
  return !parser.hadError;
}
