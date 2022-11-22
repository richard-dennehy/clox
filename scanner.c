#include <string.h>
#include "common.h"
#include "scanner.h"

void initScanner(Scanner* scanner, const char* source) {
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

static bool isAtEnd(Scanner* scanner) {
    return *scanner->current == '\0';
}

static Token makeToken(Scanner* scanner, TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (uint32_t) (scanner->current - scanner->start);
    token.line = scanner->line;

    return token;
}

static Token errorToken(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner->line;

    return token;
}

static char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static bool match(Scanner* scanner, char expected) {
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static char peek(Scanner* scanner) {
    return *scanner->current;
}

static char peekNext(Scanner* scanner) {
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static void skipWhitespace(Scanner* scanner) {
    for (;;) {
        char c = peek(scanner);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '/':
                if (peekNext(scanner) == '/') {
                    while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

Token scanToken(Scanner* scanner) {
    skipWhitespace(scanner);
    scanner->start = scanner->current;

    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    char c = advance(scanner);
    switch (c) {
        case '(':
            return makeToken(scanner, TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(scanner, TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(scanner, TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(scanner, TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(scanner, TOKEN_SEMICOLON);
        case ',':
            return makeToken(scanner, TOKEN_COMMA);
        case '.':
            return makeToken(scanner, TOKEN_DOT);
        case '-':
            return makeToken(scanner, TOKEN_MINUS);
        case '+':
            return makeToken(scanner, TOKEN_PLUS);
        case '/':
            return makeToken(scanner, TOKEN_SLASH);
        case '*':
            return makeToken(scanner, TOKEN_ASTERISK);
        case '!':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_NOT_EQUAL: TOKEN_NOT);
        case '=':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_DOUBLE_EQUAL: TOKEN_EQUAL);
        case '<':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_LESS_THAN_EQUAL: TOKEN_LESS_THAN);
        case '>':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_GREATER_THAN_EQUAL: TOKEN_GREATER_THAN);
    }

    return errorToken(scanner, "Unexpected character.");
}