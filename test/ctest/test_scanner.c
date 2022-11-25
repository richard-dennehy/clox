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

    char* whitespace = "            \t\r";
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
    char* digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
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
        char* numbers = "123 456";
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

    checkPtrsEqual("Not", "Implemented");

    return err_code;
}

int testScanIdentifiers() {
    int err_code = TEST_SUCCEEDED;

    checkPtrsEqual("Not", "Implemented");

    return err_code;
}

int main() {
    return testScanWhitespace() | testScanNumbers() | testScanStrings() | testScanSymbols() | testScanIdentifiers();
}