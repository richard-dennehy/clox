#include <string.h>
#include "test_suite.h"
#include "scanner.h"

int testScanWhitespace() {
    int err_code = TEST_SUCCEEDED;

    Scanner scanner;
    initScanner(&scanner, "");

    for (int i = 0; i < 3; i++) {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_EOF);
        checkPtrsEqual(scanned.start, scanner.start);
        checkIntsEqual(scanned.line, 1);
        checkIntsEqual(scanned.length, 0);
    }

    char *whitespace = "            \t\r";
    initScanner(&scanner, whitespace);
    for (int i = 0; i < 3; i++) {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_EOF);
        checkPtrsEqual(scanned.start, whitespace + strlen(whitespace));
        checkIntsEqual(scanned.line, 1);
        checkIntsEqual(scanned.length, 0);
    }

    return err_code;
}

int testScanNumbers() {
    int err_code = TEST_SUCCEEDED;

    Scanner scanner;
    char *digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    for (int i = 0; i < 10; i++) {
        initScanner(&scanner, digits[i]);
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 1);
        checkPtrsEqual(scanned.start, digits[i]);
    }

    {
        initScanner(&scanner, "1234567890");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 10);
    }

    {
        initScanner(&scanner, "12.34");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 5);
    }

    {
        initScanner(&scanner, "123notanumber");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 3);
    }

    {
        char *numbers = "123 456";
        initScanner(&scanner, numbers);
        Token first = scanToken(&scanner);
        checkIntsEqual(first.type, TOKEN_NUMBER);
        checkIntsEqual(first.length, 3);
        checkPtrsEqual(first.start, numbers);

        Token second = scanToken(&scanner);
        checkIntsEqual(second.type, TOKEN_NUMBER);
        checkIntsEqual(second.length, 3);
        checkPtrsEqual(second.start, numbers + 4);
    }

    {
        initScanner(&scanner, "123.notanumber");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 3);
    }

    {
        initScanner(&scanner, "123.456.789");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_NUMBER);
        checkIntsEqual(scanned.length, 7);
    }

    return err_code;
}

int testScanStrings() {
    int err_code = TEST_SUCCEEDED;
    Scanner scanner;

    {
        initScanner(&scanner, "\"string\"");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_STRING);
        checkIntsEqual(scanned.length, 8);
    }

    {
        initScanner(&scanner, "\"multiline\nstring\"");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_STRING);
        checkIntsEqual(scanned.length, 18);
    }

    {
        initScanner(&scanner, "\"Unterminated");
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_ERROR);
        checkIntsEqual(strcmp(scanned.start, "Unterminated string."), 0);
    }

    return err_code;
}

int testScanSymbols() {
    int err_code = TEST_SUCCEEDED;

    Scanner scanner;
    char* symbols = "() {} , . - + ; / * ! != = == > >= < <=";
    initScanner(&scanner, symbols);

#define TEST_NEXT_TOKEN(ty, offset, len) do { \
    Token scanned = scanToken(&scanner);           \
    checkIntsEqual(scanned.type, ty);            \
    checkPtrsEqual(scanned.start, symbols + offset); \
    checkIntsEqual(scanned.length, len);        \
} while(0)

    TEST_NEXT_TOKEN(TOKEN_LEFT_PAREN, 0, 1);
    TEST_NEXT_TOKEN(TOKEN_RIGHT_PAREN, 1, 1);
    TEST_NEXT_TOKEN(TOKEN_LEFT_BRACE, 3, 1);
    TEST_NEXT_TOKEN(TOKEN_RIGHT_BRACE, 4, 1);
    TEST_NEXT_TOKEN(TOKEN_COMMA, 6, 1);
    TEST_NEXT_TOKEN(TOKEN_DOT, 8, 1);
    TEST_NEXT_TOKEN(TOKEN_MINUS, 10, 1);
    TEST_NEXT_TOKEN(TOKEN_PLUS, 12, 1);
    TEST_NEXT_TOKEN(TOKEN_SEMICOLON, 14, 1);
    TEST_NEXT_TOKEN(TOKEN_SLASH, 16, 1);
    TEST_NEXT_TOKEN(TOKEN_ASTERISK, 18, 1);
    TEST_NEXT_TOKEN(TOKEN_NOT, 20, 1);
    TEST_NEXT_TOKEN(TOKEN_NOT_EQUAL, 22, 2);
    TEST_NEXT_TOKEN(TOKEN_EQUAL, 25, 1);
    TEST_NEXT_TOKEN(TOKEN_DOUBLE_EQUAL, 27, 2);
    TEST_NEXT_TOKEN(TOKEN_GREATER_THAN, 30, 1);
    TEST_NEXT_TOKEN(TOKEN_GREATER_THAN_EQUAL, 32, 2);
    TEST_NEXT_TOKEN(TOKEN_LESS_THAN, 35, 1);
    TEST_NEXT_TOKEN(TOKEN_LESS_THAN_EQUAL, 37, 2);

#undef TEST_NEXT_TOKEN

    return err_code;
}

int testScanIdentifiers() {
    int err_code = TEST_SUCCEEDED;

    Scanner scanner;
    char* identifiers = "and class else false for fun if nil or print return super this true var while";
    initScanner(&scanner, identifiers);

#define TEST_NEXT_TOKEN(ty, offset, len) do { \
    Token scanned = scanToken(&scanner);         \
    checkIntsEqual(scanned.type, ty);            \
    checkPtrsEqual(scanned.start, identifiers + offset); \
    checkIntsEqual(scanned.length, len);         \
} while (0)

    TEST_NEXT_TOKEN(TOKEN_AND, 0, 3);
    TEST_NEXT_TOKEN(TOKEN_CLASS, 4, 5);
    TEST_NEXT_TOKEN(TOKEN_ELSE, 10, 4);
    TEST_NEXT_TOKEN(TOKEN_FALSE, 15, 5);
    TEST_NEXT_TOKEN(TOKEN_FOR, 21, 3);
    TEST_NEXT_TOKEN(TOKEN_FUN, 25, 3);
    TEST_NEXT_TOKEN(TOKEN_IF, 29, 2);
    TEST_NEXT_TOKEN(TOKEN_NIL, 32, 3);
    TEST_NEXT_TOKEN(TOKEN_OR, 36, 2);
    TEST_NEXT_TOKEN(TOKEN_PRINT, 39, 5);
    TEST_NEXT_TOKEN(TOKEN_RETURN, 45, 6);
    TEST_NEXT_TOKEN(TOKEN_SUPER, 52, 5);
    TEST_NEXT_TOKEN(TOKEN_THIS, 58, 4);
    TEST_NEXT_TOKEN(TOKEN_TRUE, 63, 4);
    TEST_NEXT_TOKEN(TOKEN_VAR, 68, 3);
    TEST_NEXT_TOKEN(TOKEN_WHILE, 72, 5);

#undef TEST_NEXT_TOKEN

    // case sensitivity
    char* upperCaseKeyword = "CLASS";
    initScanner(&scanner, upperCaseKeyword);
    {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_IDENTIFIER);
        checkPtrsEqual(scanned.start, upperCaseKeyword);
        checkIntsEqual(scanned.length, 5);
    }

    // similar names
    char* almostKeyword = "superb";
    initScanner(&scanner, almostKeyword);
    {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_IDENTIFIER);
        checkPtrsEqual(scanned.start, almostKeyword);
        checkIntsEqual(scanned.length, 6);
    }

    // underscores
    char* underscoreIdents = "_123 _class very_long_identifier_with_underscores";
    initScanner(&scanner, underscoreIdents);
    {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_IDENTIFIER);
        checkPtrsEqual(scanned.start, underscoreIdents);
        checkIntsEqual(scanned.length, 4);
    }

    {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_IDENTIFIER);
        checkPtrsEqual(scanned.start, underscoreIdents + 5);
        checkIntsEqual(scanned.length, 6);
    }

    {
        Token scanned = scanToken(&scanner);
        checkIntsEqual(scanned.type, TOKEN_IDENTIFIER);
        checkPtrsEqual(scanned.start, underscoreIdents + 12);
        checkIntsEqual(scanned.length, 37);
    }

    return err_code;
}

int main() {
    return testScanWhitespace() | testScanNumbers() | testScanStrings() | testScanSymbols() | testScanIdentifiers();
}
