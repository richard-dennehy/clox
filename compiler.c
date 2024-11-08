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
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALISER,
    TYPE_METHOD,
    TYPE_SCRIPT,
} FunctionType;

typedef struct {
    uint32_t capacity;
    uint32_t count;
    Local* locals;
} LocalArray;

static void initLocalArray(LocalArray* array) {
    array->locals = NULL;
    array->capacity = 0;
    array->count = 0;
}

static void writeLocal(VM* vm, Compiler* compiler, LocalArray* array, Local local) {
    if (array->capacity < array->count + 1) {
        uint32_t oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->locals = COMPILER_GROW_ARRAY(Local, array->locals, oldCapacity, array->capacity);
    }

    array->locals[array->count++] = local;
}

static void freeLocalArray(VM* vm, LocalArray* array) {
    VM_FREE_ARRAY(Value, array->locals, array->capacity);
    initLocalArray(array);
}

struct Compiler {
    Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    LocalArray localArray;
    Upvalue upvalues[UINT8_COUNT];
    uint32_t scopeDepth;
};

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
    return &parser->compiler->function->chunk;
}

static void emitByte(Parser* parser, uint8_t byte) {
    writeChunk(parser->vm, parser->compiler, currentChunk(parser), byte, parser->previous.line);
}

static void emitBytes(Parser* parser, uint8_t byte1, uint8_t byte2) {
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

static void emitReturn(Parser* parser) {
    if (parser->compiler->type == TYPE_INITIALISER) {
        // implicit `return this` in initialisers
        emitBytes(parser, OP_GET_LOCAL, 0);
    } else {
        emitByte(parser, OP_NIL);
    }

    emitByte(parser, OP_RETURN);
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
    writeValue(parser->vm, parser->compiler, &currentChunk(parser)->constants, value);
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

static void namedVariable(Parser* parser, Token name, bool canAssign);

static void initCompiler(Parser* parser, Compiler* compiler, FunctionType type);

static ObjFunction* endCompiler(Parser* parser);

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

static void beginScope(Parser* parser) {
    parser->compiler->scopeDepth++;
}

static void endScope(Parser* parser) {
    Compiler* compiler = parser->compiler;
    compiler->scopeDepth--;

    while (compiler->localArray.count > 0 &&
           compiler->localArray.locals[compiler->localArray.count - 1].depth > (int32_t) compiler->scopeDepth) {
        if (compiler->localArray.locals[compiler->localArray.count - 1].isCaptured) {
            emitByte(parser, OP_CLOSE_UPVALUE);
        } else {
            emitByte(parser, OP_POP);
        }
        compiler->localArray.count--;
    }
}

static void addLocal(Parser* parser, Token name) {
    Compiler* compiler = parser->compiler;

    Local local = {
            .name = name,
            .depth = -1,
            .isCaptured = false,
    };
    writeLocal(parser->vm, compiler, &compiler->localArray, local);
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable(Parser* parser) {
    Compiler* compiler = parser->compiler;
    if (compiler->scopeDepth == 0) return;

    Token* name = &parser->previous;
    for (int32_t i = (int32_t) compiler->localArray.count - 1; i >= 0; i--) {
        Local* local = &compiler->localArray.locals[i];
        if (local->depth != -1 && (uint32_t) local->depth < compiler->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(parser, "Already a variable with this name in scope.");
        }
    }
    addLocal(parser, *name);
}

static void markInitialised(Parser* parser) {
    if (parser->compiler->scopeDepth == 0) return;
    Compiler* compiler = parser->compiler;
    compiler->localArray.locals[compiler->localArray.count - 1].depth = (int32_t) compiler->scopeDepth;
}

// separate declaration/definition to prevent e.g. referring to a variable in its own initialiser i.e. `var a = a`
static void defineVariable(Parser* parser, uint32_t global) {
    if (parser->compiler->scopeDepth > 0) {
        markInitialised(parser);
        return;
    }

    emitVariableWidth(parser, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, global);
}

static uint32_t identifierConstant(Parser* parser, Token* name) {
    return makeConstant(parser, OBJ_VAL(copyString(parser->vm, parser->compiler, name->start, name->length)));
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

static void block(Parser* parser) {
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(Parser* parser, FunctionType type) {
    Compiler compiler;
    initCompiler(parser, &compiler, type);
    beginScope(parser);

    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            Compiler* current = parser->compiler;
            if (current->function->arity == UINT8_MAX) {
                errorAt(parser, &parser->current, "Can't have more than 255 parameters.");
            }
            current->function->arity++;
            uint32_t constant = parseVariable(parser, "Expect parameter name.");
            defineVariable(parser, constant);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after function parameters.");
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(parser);

    ObjFunction* function = endCompiler(parser);
    uint32_t index = makeConstant(parser, OBJ_VAL(function));
    // TODO support wide closure instruction - could have 256 constants then be unable to define any closures (or functions)
    assert(index <= UINT8_MAX || !"Too many constants");
    emitBytes(parser, OP_CLOSURE, index);

    for (uint32_t i = 0; i < function->upvalueCount; i++) {
        emitByte(parser, compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(parser, compiler.upvalues[i].index);
    }
}

static void funDeclaration(Parser* parser) {
    uint32_t global = parseVariable(parser, "Expect function name.");
    markInitialised(parser);
    function(parser, TYPE_FUNCTION);
    defineVariable(parser, global);
}

static void method(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(parser, &parser->previous);

    FunctionType type = TYPE_METHOD;
    if (parser->previous.length == 4 && memcmp(parser->previous.start, "init", 4) == 0) {
        type = TYPE_INITIALISER;
    }

    function(parser, type);
    emitBytes(parser, OP_METHOD, constant);
}

static Token syntheticToken(const char* text) {
    return (Token) {
            .start = text,
            .length = (uint32_t) strlen(text)
    };
}

static void classDeclaration(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser->previous;
    uint8_t nameConstant = identifierConstant(parser, &parser->previous);
    declareVariable(parser);

    emitBytes(parser, OP_CLASS, nameConstant);
    defineVariable(parser, nameConstant);

    ClassCompiler classCompiler = {
            .enclosing = parser->currentClass,
            .hasSuperclass = false
    };
    parser->currentClass = &classCompiler;

    if (match(parser, TOKEN_LESS_THAN)) {
        consume(parser, TOKEN_IDENTIFIER, "Expect superclass name.");
        namedVariable(parser, parser->previous, false);

        if (identifiersEqual(&className, &parser->previous)) {
            error(parser, "A class can't inherit from itself");
        }

        beginScope(parser);
        addLocal(parser, syntheticToken("super"));
        defineVariable(parser, 0);

        namedVariable(parser, className, false);
        emitByte(parser, OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    // push class instance onto the top of the stack so the VM can find it when executing OP_METHOD
    namedVariable(parser, className, false);

    consume(parser, TOKEN_LEFT_BRACE, "Expected '{' before class body.");
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        method(parser);
    }
    consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' before class body.");
    // pop class instance
    emitByte(parser, OP_POP);

    if (parser->currentClass->hasSuperclass) {
        endScope(parser);
    }

    parser->currentClass = classCompiler.enclosing;
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

static void declaration(Parser* parser) {
    if (match(parser, TOKEN_CLASS)) {
        classDeclaration(parser);
    } else if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    } else if (match(parser, TOKEN_FUN)) {
        funDeclaration(parser);
    } else {
        statement(parser);
    }

    if (parser->panicMode) synchronise(parser);
}

static void printStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(parser, OP_PRINT);
}

static int32_t emitJump(Parser* parser, OpCode op) {
    emitByte(parser, op);
    emitBytes(parser, 0xFF, 0xFF);
    return (int32_t) currentChunk(parser)->count - 2;
}

static void patchJump(Parser* parser, int32_t index) {
    int32_t jump = (int32_t) currentChunk(parser)->count - index - 2;
    if (jump > UINT16_MAX) {
        error(parser, "Too much code to jump over.");
    }

    currentChunk(parser)->code[index] = (uint8_t) (jump >> 8);
    currentChunk(parser)->code[index + 1] = (uint8_t) jump;
}

static void emitLoop(Parser* parser, uint32_t loopStart) {
    emitByte(parser, OP_LOOP);

    uint32_t offset = currentChunk(parser)->count - loopStart + 2;
    if (offset > UINT16_MAX) error(parser, "Loop body too large.");

    emitByte(parser, (uint8_t) (offset >> 8));
    emitByte(parser, (uint8_t) offset);
}

static void ifStatement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int32_t thenJump = emitJump(parser, OP_JUMP_IF_FALSE);
    // pop condition result
    emitByte(parser, OP_POP);
    statement(parser);
    int32_t elseJump = emitJump(parser, OP_JUMP);
    patchJump(parser, thenJump);
    emitByte(parser, OP_POP);

    if (match(parser, TOKEN_ELSE)) {
        statement(parser);
    }
    patchJump(parser, elseJump);
}

static void whileStatement(Parser* parser) {
    uint32_t loopStart = currentChunk(parser)->count;
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int32_t exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);
    statement(parser);
    emitLoop(parser, loopStart);

    patchJump(parser, exitJump);
    emitByte(parser, OP_POP);
}

static void expressionStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(parser, OP_POP);
}

static void forStatement(Parser* parser) {
    beginScope(parser);
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(parser, TOKEN_SEMICOLON)) {
        // no initialiser
    } else if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    } else {
        expressionStatement(parser);
    }

    uint32_t loopStart = currentChunk(parser)->count;
    int32_t exitJump = -1;
    if (!match(parser, TOKEN_SEMICOLON)) {
        expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
        emitByte(parser, OP_POP);
    }

    if (!match(parser, TOKEN_RIGHT_PAREN)) {
        int32_t bodyJump = emitJump(parser, OP_JUMP);
        uint32_t incrementStart = currentChunk(parser)->count;
        expression(parser);
        emitByte(parser, OP_POP);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(parser, loopStart);
        loopStart = incrementStart;
        patchJump(parser, bodyJump);
    }

    statement(parser);
    emitLoop(parser, loopStart);
    if (exitJump != -1) {
        patchJump(parser, exitJump);
        emitByte(parser, OP_POP);
    }
    endScope(parser);
}

static void returnStatement(Parser* parser) {
    if (parser->compiler->type == TYPE_SCRIPT) {
        error(parser, "Can't return from top level code.");
    }
    // continue parsing in case of errors so the compiler isn't left in an invalid state
    if (match(parser, TOKEN_SEMICOLON)) {
        emitReturn(parser);
    } else {
        if (parser->compiler->type == TYPE_INITIALISER) {
            error(parser, "Can't return value from an initialiser.");
        }
        expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(parser, OP_RETURN);
    }
}

static void statement(Parser* parser) {
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    } else if (match(parser, TOKEN_IF)) {
        ifStatement(parser);
    } else if (match(parser, TOKEN_WHILE)) {
        whileStatement(parser);
    } else if (match(parser, TOKEN_FOR)) {
        forStatement(parser);
    } else if (match(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    } else if (match(parser, TOKEN_RETURN)) {
        returnStatement(parser);
    } else {
        expressionStatement(parser);
    }
}

static void number(Parser* parser, UNUSED bool canAssign) {
    Value value = NUMBER_VAL(strtod(parser->previous.start, NULL));
    emitConstant(parser, value);
}

static void string(Parser* parser, UNUSED bool canAssign) {
    Value value = OBJ_VAL(
            copyString(parser->vm, parser->compiler, parser->previous.start + 1, parser->previous.length - 2));
    emitConstant(parser, value);
}

static void grouping(Parser* parser, UNUSED bool canAssign) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static uint8_t argumentList(Parser* parser) {
    uint8_t argumentCount = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (++argumentCount >= 255) {
                error(parser, "Can't have more than 255 arguments.");
            }
            expression(parser);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

    return argumentCount;
}

static void call(Parser* parser, UNUSED bool canAssign) {
    uint8_t argumentCount = argumentList(parser);
    emitBytes(parser, OP_CALL, argumentCount);
}

static void unary(Parser* parser, UNUSED bool canAssign) {
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

static void binary(Parser* parser, UNUSED bool canAssign) {
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

static void literal(Parser* parser, UNUSED bool canAssign) {
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
    for (int32_t i = (int32_t) compiler->localArray.count - 1; i >= 0; i--) {
        Local* local = &compiler->localArray.locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(parser, "Can't read local variable in its own initialiser.");
            }
            return i;
        }
    }

    return -1;
}

static uint32_t addUpvalue(Parser* parser, Compiler* compiler, uint8_t index, bool isLocal) {
    uint32_t upvalueCount = compiler->function->upvalueCount;

    for (uint32_t i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = compiler->upvalues + i;
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error(parser, "Too many closure variables in function.");
        return 0;
    }
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int32_t resolveUpvalue(Parser* parser, Compiler* compiler, Token* name) {
    if (!compiler->enclosing) return -1;

    int32_t local = resolveLocal(parser, compiler->enclosing, name);
    if (local != -1) {
        assert(local <= UINT8_MAX || !"Too many locals");
        compiler->enclosing->localArray.locals[local].isCaptured = true;
        return (int32_t) addUpvalue(parser, compiler, (uint8_t) local, true);
    }
    int32_t upvalue = resolveUpvalue(parser, compiler->enclosing, name);
    if (upvalue != -1) {
        return (int32_t) addUpvalue(parser, compiler, (uint8_t) upvalue, false);
    }

    return -1;
}

static void namedVariable(Parser* parser, Token name, UNUSED bool canAssign) {
    OpCode getOp, setOp, getOpLong, setOpLong;

    int32_t argument = resolveLocal(parser, parser->compiler, &name);
    if (argument != -1) {
        getOp = OP_GET_LOCAL;
        getOpLong = OP_GET_LOCAL_LONG;
        setOp = OP_SET_LOCAL;
        setOpLong = OP_SET_LOCAL_LONG;
    } else if ((argument = resolveUpvalue(parser, parser->compiler, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        getOpLong = OP_GET_UPVALUE_LONG;
        setOp = OP_SET_UPVALUE;
        setOpLong = OP_SET_UPVALUE_LONG;
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

static void variable(Parser* parser, UNUSED bool canAssign) {
    namedVariable(parser, parser->previous, canAssign);
}

static void and(Parser* parser, UNUSED bool canAssign) {
    int32_t endJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);
    parsePrecedence(parser, PREC_AND);
    patchJump(parser, endJump);
}

static void or(Parser* parser, UNUSED bool canAssign) {
    int32_t elseJump = emitJump(parser, OP_JUMP_IF_FALSE);
    int32_t endJump = emitJump(parser, OP_JUMP);

    patchJump(parser, elseJump);
    emitByte(parser, OP_POP);

    parsePrecedence(parser, PREC_OR);
    patchJump(parser, endJump);
}

static void dot(Parser* parser, bool canAssign) {
    consume(parser, TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(parser, &parser->previous);

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitBytes(parser, OP_SET_PROPERTY, name);
    } else if (match(parser, TOKEN_LEFT_PAREN)) {
        // optimise for immediate method calls i.e. bytecode can be substantially simplified for accessing a method property and invoking it immediately, rather than assigning the property to a variable then invoking that
        uint8_t argumentCount = argumentList(parser);
        emitBytes(parser, OP_INVOKE, name);
        emitByte(parser, argumentCount);
    } else {
        emitBytes(parser, OP_GET_PROPERTY, name);
    }
}

static void this(Parser* parser, UNUSED bool canAssign) {
    if (!parser->currentClass) {
        error(parser, "Can't use 'this' outside a class.");
        return;
    }
    variable(parser, false);
}

static void super(Parser* parser, UNUSED bool canAssign) {
    if (!parser->currentClass) {
        error(parser, "Can't use 'super' outside of a class.");
    } else if (!parser->currentClass->hasSuperclass) {
        error(parser, "Can't use 'super' in a class with no superclass.");
    }

    consume(parser, TOKEN_DOT, "Expect '.' after 'super'.");
    consume(parser, TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(parser, &parser->previous);

    namedVariable(parser, syntheticToken("this"), false);
    if (match(parser, TOKEN_LEFT_PAREN)) {
        uint8_t argumentCount = argumentList(parser);
        namedVariable(parser, syntheticToken("super"), false);
        emitBytes(parser, OP_SUPER_INVOKE, name);
        emitByte(parser, argumentCount);
    } else {
        namedVariable(parser, syntheticToken("super"), false);
        emitBytes(parser, OP_GET_SUPER, name);
    }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, dot, PREC_CALL},
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
        [TOKEN_AND] = {NULL, and, PREC_NONE},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, or, PREC_NONE},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {super, NULL, PREC_NONE},
        [TOKEN_THIS] = {this, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static ObjFunction* endCompiler(Parser* parser) {
    emitReturn(parser);
    ObjFunction* function = parser->compiler->function;

    freeLocalArray(parser->vm, &parser->compiler->localArray);
#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) {
        disassembleChunk(currentChunk(parser), function->name ? function->name->chars : "<script>");
    }
#endif

    parser->compiler = parser->compiler->enclosing;
    return function;
}

static void initCompiler(Parser* parser, Compiler* compiler, FunctionType type) {
    compiler->enclosing = parser->compiler;
    initLocalArray(&compiler->localArray);
    compiler->scopeDepth = 0;
    compiler->type = type;
    compiler->function = NULL;
    compiler->function = newFunction(parser->vm, parser->compiler);

    parser->compiler = compiler;
    if (type != TYPE_SCRIPT) {
        compiler->function->name = copyString(parser->vm, parser->compiler, parser->previous.start,
                                              parser->previous.length);
    }

    Local local = {
            .depth = 0,
            .name = {
                    .start = "",
                    .length = 0,
            },
            .isCaptured = false,
    };
    if (type != TYPE_FUNCTION) {
        local.name.start = "this";
        local.name.length = 4;
    }
    writeLocal(parser->vm, compiler, &compiler->localArray, local);
}

ObjFunction* compile(VM* vm, const char* source) {
    Scanner scanner;
    initScanner(&scanner, source);

    Parser parser = {
            .scanner = &scanner,
            .hadError = false,
            .panicMode = false,
            .vm = vm,
            .compiler = NULL,
            .currentClass = NULL
    };

    Compiler compiler;
    initCompiler(&parser, &compiler, TYPE_SCRIPT);

    advance(&parser);
    while (!match(&parser, TOKEN_EOF)) declaration(&parser);
    ObjFunction* function = endCompiler(&parser);

    return parser.hadError ? NULL : function;
}

void markCompilerRoots(VM* vm, Compiler* compiler) {
    while (compiler) {
        markObject(vm, (Obj*) compiler->function);
        compiler = compiler->enclosing;
    }
}
