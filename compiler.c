#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"

#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  // since c doesn't have exceptions, this flag will be used to suppress any cascaded errors until a
  //    `synchronization point` is reached, which will be statement boundaries in this case
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  // scope depth of where the var was declared
  int depth;
} Local;

// local var state
typedef struct {
  Local locals[UINT8_COUNT];
  // locals in scope (duh)
  int localCount;

  // how many scopes deep are we?
  int scopeDepth;
} Compiler;

Parser parser;

Compiler* current = NULL;

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

// check token type against current token
// return true if they match, false otherwise
static bool check(TokenType type) {
  return parser.current.type == type;
}

// if the current token has the given type, consume and return true - otherwise return false
static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
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

static uint8_t makeConstant(Value value) {
  // add the constant to the constants array
  int constant = addConstant(currentChunk(), value);

  // since we only use one byte on the index operand for the OP_CONSTANT instruction
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
}

static void endCompiler() {
  emitReturn();

#ifdef DEBUG_PRINT_CODE
  // only dump if the parser has errors
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// clox does not cascade call higher-precedence expression types like jlox
//    so we have this method that parses any expression at a given precedence level or higher
static void parsePrecedence(Precedence precedence) {
  advance();
  // read the next token and look up the corresponding ParseRule.
  // if there is no prefix parser, then the token must be a syntax error
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  // allow assigment only in top level expressions or when parsing and assignment expression
  bool canAssign = precedence <= PREC_ASSIGNMENT;

  // call that prefix parse function and let it do its thing. that prefix parser compiles the rest
  //    of the prefix expression, consuming any other tokens it needs, and returns back
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

// output bytecode instructions that defines the new var and stores its initial value
// the index of the variable name in the constant table is the instruction's operand
static void defineVariable(uint8_t global) {
  emitBytes(OP_DEFINE_GLOBAL, global);
}

// takes a token and adds its lexeme to the chunk's constant table as a string
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// requires the next token to be an identifier
static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  return identifierConstant(&parser.previous);
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

// compile the right operand, then emit the binary operation bytecode
static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
  case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
  case TOKEN_GREATER:       emitByte(OP_GREATER); break;
  case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
  case TOKEN_LESS:          emitByte(OP_LESS); break;
  case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
  case TOKEN_PLUS:          emitByte(OP_ADD); break;
  case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
  case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
  case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
  default: return; // unreachable.
  }
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
  case TOKEN_FALSE: emitByte(OP_FALSE); break;
  case TOKEN_NIL: emitByte(OP_NIL); break;
  case TOKEN_TRUE: emitByte(OP_TRUE); break;
  default: return; // unreachable
  }
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON,
          "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

// compile the rest of the print statement once detected
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

// skip tokens indiscriminately until reaching a statement boundary
// find the boundary by looking for a preceding token that can end a statement or look for a subsequent token that
//    starts a statement
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    default:
      ; // do nothing.
    }

    advance();
  }
}

static void statement();
static void declaration();

static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode)
    synchronize();
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else {
    expressionStatement();
  }

}

// recursively call into `expression()` to compile the expression between parentheses
static void grouping(bool canAssign) {
  expression();
  // assume that the `(` has already been consumed
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// compile number literals
static void number(bool canAssign) {
  // assume token has already been consumed and stored in `previous`
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
  // this is where any escape character support should go
  // + 1 and - 2 bounds trim the leading and trailing quotes
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t arg = identifierConstant(&name);

  if (canAssign && match(TOKEN_EQUAL)) {
    // if the token after an identifier expression is an equal sign, then we compile the assigned value
    //    and emit and assignment instruction
    expression();
    emitBytes(OP_SET_GLOBAL, arg);
  } else {
    emitBytes(OP_GET_GLOBAL, arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // compile operand
  // this allows nested unary expressions like `!!`
  parsePrecedence(PREC_UNARY);

  // emit the operator instruction
  // write the negate instruction after its operand’s bytecode
  //    1. we evaluate the operand first which leaves its value on the stack.
  //    2. then we pop that value, negate it, and push the result.
  switch (operatorType) {
  case TOKEN_BANG: emitByte(OP_NOT); break;
  case TOKEN_MINUS: emitByte(OP_NEGATE); break;
  default: return; // unreachable.
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     binary,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     binary,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);

  Compiler compiler;
  initCompiler(&compiler);

  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  // compile declarations until we hit the end of a source file
  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();
  return !parser.hadError;
}
