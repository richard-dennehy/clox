#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "compiler.h"
#include "scanner.h"
#include "debug.h"
#include "object.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} Precedence;

typedef void (* ParseFn)(Parser* parser, bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int32_t depth;
} Local;

typedef struct Compiler {
    // TODO make this dynamic
    Local locals[UINT8_COUNT];
    uint32_t localCount;
    uint32_t scopeDepth;
} Compiler;

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->hadError = true;
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->previous, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;

    for (;;) {
        parser->current = scanToken(parser->scanner);
        if (parser->current.type != TOKEN_ERROR) break;

        errorAt(parser, &parser->current, parser->current.start);
    }
}

static Chunk* currentChunk(Parser* parser) {
    return parser->compilingChunk;
}

static void emitByte(Parser* parser, uint8_t byte) {
    writeChunk(parser->freeList, currentChunk(parser), byte, parser->previous.line);
}

static void emitBytes(Parser* parser, uint8_t byte1, uint8_t byte2) {
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

static void emitVariableWidth(Parser* parser, uint8_t byteOp, uint8_t longOp, uint32_t value) {
    if (value <= 255) {
        emitBytes(parser, byteOp, (uint8_t) value);
    } else {
        emitByte(parser, longOp);
        emitByte(parser, (uint8_t) (value >> 16));
        emitByte(parser, (uint8_t) (value >> 8));
        emitByte(parser, (uint8_t) (value >> 0));
    }
}

static uint32_t makeConstant(Parser* parser, Value value) {
    writeValue(parser->freeList, &currentChunk(parser)->constants, value);
    uint32_t index = currentChunk(parser)->constants.count - 1;
    assert(index < 1 << 24 || !"Way too many constants");
    return index;
}

static void emitConstant(Parser* parser, Value value) {
    uint32_t index = makeConstant(parser, value);
    emitVariableWidth(parser, OP_CONSTANT, OP_CONSTANT_LONG, index);
}

static ParseRule* getRule(TokenType type);

static void statement(Parser* parser);

static void declaration(Parser* parser);

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void parsePrecedence(Parser* parser, Precedence precedence) {
    advance(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (!prefixRule) {
        error(parser, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(parser, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        assert(infixRule || !"Attempting to parse non-infix operator as infix");
        infixRule(parser, canAssign);
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        error(parser, "Invalid assignment target.");
    }
}

static void expression(Parser* parser) {
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

static void consume(Parser* parser, TokenType expected, const char* message) {
    if (parser->current.type == expected) {
        advance(parser);
        return;
    }

    errorAt(parser, &parser->current, message);
}

static void printStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(parser, OP_PRINT);
}

static void expressionStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(parser, OP_POP);
}

static void synchronise(Parser* parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: {
                advance(parser);
            }
        }
    }
}

static void addLocal(Parser* parser, Token name) {
    Compiler* compiler = parser->compiler;
    if (compiler->localCount == UINT8_COUNT) {
        error(parser, "Too many local variables in function.");
        return;
    }

    Local* local = &compiler->locals[compiler->localCount++];
    local->name = name;
    local->depth = -1;
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable(Parser* parser) {
    Compiler* compiler = parser->compiler;
    if (compiler->scopeDepth == 0) return;

    Token* name = &parser->previous;
    for (int32_t i = (int32_t) compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth != -1 && local->depth < compiler->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(parser, "Already a variable with this name in scope.");
        }
    }
    addLocal(parser, *name);
}

static void markInitialised(Parser* parser) {
    Compiler* compiler = parser->compiler;
    compiler->locals[compiler->localCount - 1].depth = (int32_t) compiler->scopeDepth;
}

static void defineVariable(Parser* parser, uint32_t global) {
    if (parser->compiler->scopeDepth > 0) {
        markInitialised(parser);
        return;
    }

    emitVariableWidth(parser, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, global);
}

static uint32_t identifierConstant(Parser* parser, Token* name) {
    return makeConstant(parser, OBJ_VAL(copyString(parser->vm, name->start, name->length)));
}

static uint32_t parseVariable(Parser* parser, const char* errorMessage) {
    consume(parser, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(parser);
    if (parser->compiler->scopeDepth > 0) return 0;

    return identifierConstant(parser, &parser->previous);
}

static void varDeclaration(Parser* parser) {
    uint32_t global = parseVariable(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    } else {
        emitByte(parser, OP_NIL);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(parser, global);
}

static void declaration(Parser* parser) {
    if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    } else {
        statement(parser);
    }

    if (parser->panicMode) synchronise(parser);
}

static void beginScope(Parser* parser) {
    parser->compiler->scopeDepth++;
}

static void endScope(Parser* parser) {
    Compiler* compiler = parser->compiler;
    compiler->scopeDepth--;

    while (compiler->localCount > 0 && compiler->locals[compiler->localCount - 1].depth > compiler->scopeDepth) {
        emitByte(parser, OP_POP);
        compiler->localCount--;
    }
}

static void block(Parser* parser) {
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement(Parser* parser) {
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    } else if (match(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    } else {
        expressionStatement(parser);
    }
}

static void number(Parser* parser, bool canAssign) {
    Value value = NUMBER_VAL(strtod(parser->previous.start, NULL));
    emitConstant(parser, value);
}

static void string(Parser* parser, bool canAssign) {
    Value value = OBJ_VAL(copyString(parser->vm, parser->previous.start + 1, parser->previous.length - 2));
    emitConstant(parser, value);
}

static void grouping(Parser* parser, bool canAssign) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(Parser* parser, bool canAssign) {
    TokenType operator = parser->previous.type;
    parsePrecedence(parser, PREC_UNARY);

    switch (operator) {
        case TOKEN_MINUS:
            emitByte(parser, OP_NEGATE);
            break;
        case TOKEN_NOT:
            emitByte(parser, OP_NOT);
            break;
        default:
            return;
    }
}

static void binary(Parser* parser, bool canAssign) {
    TokenType operator = parser->previous.type;
    ParseRule* rule = getRule(operator);
    parsePrecedence(parser, (Precedence) (rule->precedence + 1));

    switch (operator) {
        case TOKEN_PLUS:
            emitByte(parser, OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(parser, OP_SUBTRACT);
            break;
        case TOKEN_ASTERISK:
            emitByte(parser, OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(parser, OP_DIVIDE);
            break;
        case TOKEN_NOT_EQUAL:
            emitBytes(parser, OP_EQUAL, OP_NOT);
            break;
        case TOKEN_DOUBLE_EQUAL:
            emitByte(parser, OP_EQUAL);
            break;
        case TOKEN_GREATER_THAN:
            emitByte(parser, OP_GREATER);
            break;
        case TOKEN_GREATER_THAN_EQUAL:
            emitBytes(parser, OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS_THAN:
            emitByte(parser, OP_LESS);
            break;
        case TOKEN_LESS_THAN_EQUAL:
            emitBytes(parser, OP_GREATER, OP_NOT);
            break;
        default:
            return;
    }
}

static void literal(Parser* parser, bool canAssign) {
    switch (parser->previous.type) {
        case TOKEN_NIL:
            emitByte(parser, OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(parser, OP_TRUE);
            break;
        case TOKEN_FALSE:
            emitByte(parser, OP_FALSE);
            break;

        default:
            return;
    }
}

static int32_t resolveLocal(Parser* parser, Compiler* compiler, Token* name) {
    for (int32_t i = (int32_t) compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(parser, "Can't read local variable in its own initialiser.");
            }
            return i;
        }
    }

    return -1;
}

static void namedVariable(Parser* parser, Token name, bool canAssign) {
    OpCode getOp, setOp, getOpLong, setOpLong;

    int32_t argument = resolveLocal(parser, parser->compiler, &name);
    if (argument != -1) {
        // TODO need to fix this after extending max Local count
        getOp = OP_GET_LOCAL;
        getOpLong = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
        setOpLong = OP_SET_LOCAL;
    } else {
        argument = (int32_t) identifierConstant(parser, &name);
        getOp = OP_GET_GLOBAL;
        getOpLong = OP_GET_GLOBAL_LONG;
        setOp = OP_SET_GLOBAL;
        setOpLong = OP_SET_GLOBAL_LONG;
    }
    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitVariableWidth(parser, setOp, setOpLong, argument);
    } else {
        emitVariableWidth(parser, getOp, getOpLong, argument);
    }
}

static void variable(Parser* parser, bool canAssign) {
    namedVariable(parser, parser->previous, canAssign);
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_ASTERISK] = {NULL, binary, PREC_FACTOR},
        [TOKEN_NOT] = {unary, NULL, PREC_NONE},
        [TOKEN_NOT_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOUBLE_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER_THAN] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_THAN_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_THAN] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_THAN_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, NULL, PREC_NONE},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, NULL, PREC_NONE},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void endCompiler(Parser* parser) {
    emitByte(parser, OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) {
        disassembleChunk(currentChunk(parser), "code");
    }
#endif
}

void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
}

bool compile(VM* vm, const char* source, Chunk* chunk) {
    Scanner scanner;
    initScanner(&scanner, source);
    Compiler compiler;
    initCompiler(&compiler);

    Parser parser;
    parser.scanner = &scanner;
    parser.hadError = false;
    parser.panicMode = false;
    parser.compilingChunk = chunk;
    parser.vm = vm;
    parser.compiler = &compiler;
    parser.freeList = vm->freeList;

    advance(&parser);
    while (!match(&parser, TOKEN_EOF)) declaration(&parser);
    endCompiler(&parser);

    return !parser.hadError;
}
