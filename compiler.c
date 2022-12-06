#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "compiler.h"
#include "scanner.h"

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

typedef void (*ParseFn)(Parser*);
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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

static void emitConstant(Parser* parser, Value value) {
    writeConstant(parser->freeList, currentChunk(parser), value, parser->previous.line);
}

static ParseRule* getRule(TokenType type);
static void parsePrecedence(Parser* parser, Precedence precedence) {
    advance(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (!prefixRule) {
        errorAt(parser, &parser->previous, "Expect expression.");
        return;
    }

    prefixRule(parser);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        assert(infixRule || !"Attempting to parse non-infix operator as infix");
        infixRule(parser);
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

static void number(Parser* parser) {
    Value value = strtod(parser->previous.start, NULL);
    emitConstant(parser, value);
}

static void grouping(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(Parser* parser) {
    TokenType operator = parser->previous.type;
    parsePrecedence(parser, PREC_UNARY);

    switch (operator) {
        case TOKEN_MINUS:
            emitByte(parser, OP_NEGATE);
            break;
        default:
            return;
    }
}

static void binary(Parser* parser) {
    TokenType operator = parser->previous.type;
    ParseRule* rule = getRule(operator);
    parsePrecedence(parser, (Precedence)(rule->precedence + 1));

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
        default:
            return;
    }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = { grouping, NULL, PREC_NONE },
        [TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
        [TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
        [TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
        [TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
        [TOKEN_DOT] = { NULL, NULL, PREC_NONE },
        [TOKEN_MINUS] = { unary, binary, PREC_TERM },
        [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
        [TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
        [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
        [TOKEN_ASTERISK] = { NULL, binary, PREC_FACTOR },
        [TOKEN_NOT] = { NULL, NULL, PREC_NONE },
        [TOKEN_NOT_EQUAL] = { NULL, NULL, PREC_NONE },
        [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
        [TOKEN_DOUBLE_EQUAL] = { NULL, NULL, PREC_NONE },
        [TOKEN_GREATER_THAN] = { NULL, NULL, PREC_NONE },
        [TOKEN_GREATER_THAN_EQUAL] = { NULL, NULL, PREC_NONE },
        [TOKEN_LESS_THAN] = { NULL, NULL, PREC_NONE },
        [TOKEN_LESS_THAN_EQUAL] = { NULL, NULL, PREC_NONE },
        [TOKEN_IDENTIFIER] = { NULL, NULL, PREC_NONE },
        [TOKEN_STRING] = { NULL, NULL, PREC_NONE },
        [TOKEN_NUMBER] = { number, NULL, PREC_NONE },
        [TOKEN_AND] = { NULL, NULL, PREC_NONE },
        [TOKEN_CLASS] = { NULL, NULL, PREC_NONE },
        [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
        [TOKEN_FALSE] = { NULL, NULL, PREC_NONE },
        [TOKEN_FOR] = { NULL, NULL, PREC_NONE },
        [TOKEN_FUN] = { NULL, NULL, PREC_NONE },
        [TOKEN_IF] = { NULL, NULL, PREC_NONE },
        [TOKEN_NIL] = { NULL, NULL, PREC_NONE },
        [TOKEN_OR] = { NULL, NULL, PREC_NONE },
        [TOKEN_PRINT] = { NULL, NULL, PREC_NONE },
        [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
        [TOKEN_SUPER] = { NULL, NULL, PREC_NONE },
        [TOKEN_THIS] = { NULL, NULL, PREC_NONE },
        [TOKEN_TRUE] = { NULL, NULL, PREC_NONE },
        [TOKEN_VAR] = { NULL, NULL, PREC_NONE },
        [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
        [TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
        [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void endCompiler(Parser* parser) {
    emitByte(parser, OP_RETURN);
}

bool compile(FreeList* freeList, const char* source, Chunk* chunk) {
    Scanner scanner;
    initScanner(&scanner, source);

    Parser parser;
    parser.scanner = &scanner;
    parser.hadError = false;
    parser.panicMode = false;
    parser.compilingChunk = chunk;
    parser.freeList = freeList;

    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression.");
    endCompiler(&parser);

    return !parser.hadError;
}
