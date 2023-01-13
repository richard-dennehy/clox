#include "test_suite.h"
#include "vm.c"

#define INTERPRET(source) assert(interpret(&vm, source) == INTERPRET_OK)
#define STACK_HEAD vm.stack.values[0]
static char printLog[64][32];
static int printed = 0;
int fakePrintf(const char* format, ...) {
    assert(printed < 64 || !"Too many things printed");
    va_list args;
    va_start(args, format);
    int result = vsnprintf(printLog[printed++], 32, format, args);
    va_end(args);
    return result;
}

void resetPrintLog() {
    printed = 0;
}

int testVmStack() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 16 * 1024);
    VM vm;
    initVM(&freeList, &vm);

    Value v0 = NUMBER_VAL(1.0);
    push(&vm, v0);
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 1.0);

    Value v1 = NUMBER_VAL(2.0);
    Value v2 = NUMBER_VAL(4.0);
    push(&vm, v1);
    push(&vm, v2);

    checkFloatsEqual(AS_NUMBER(pop(&vm)), 4.0);
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 2.0);

    for (int i = 0; i < 257; i++) {
        push(&vm, NUMBER_VAL(i));
    }
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 256);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testVmArithmetic() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);

    // NOTE: these tests will probably stop working once the temporary return instruction stops being added to all chunks - will need to adjust the indexes
#define RUN_TEST(source, expected) do { \
    INTERPRET(source);  \
    Value result = vm.stack.values[0];                                    \
    checkIntsEqual(result.type, VAL_NUMBER);                                    \
    checkFloatsEqual(result.as.number, expected); \
} while(0)

    RUN_TEST("-2;", -2.0);
    RUN_TEST("3 * 4;", 12.0);
    RUN_TEST("5 + 6;", 11.0);
    RUN_TEST("7 - 8;", -1.0);
    RUN_TEST("9 / 10;", 0.9);
    RUN_TEST("(-1 + 2) * 3 - -4;", 7);

#undef RUN_TEST
    freeVM(&vm);
    freeMemory(&freeList);

    return err_code;
}

int testNil() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);

    INTERPRET("nil;");
    checkIntsEqual(vm.stack.values[0].type, VAL_NIL);

    INTERPRET("nil == nil;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("nil != nil;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!nil;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testBools() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);

    INTERPRET("true;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("false;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!true;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!false;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("true == true;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("false == false;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("true == false;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testComparisons() {
#define RUN_TEST(source, expected) do { \
    INTERPRET(source);  \
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);                                    \
    checkIntsEqual(STACK_HEAD.as.boolean, expected); \
} while(0)

    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);

    RUN_TEST("1 > 0;", true);
    RUN_TEST("1 >= 0;", true);
    RUN_TEST("1 == 0;", false);
    RUN_TEST("1 <= 0;", false);
    RUN_TEST("1 < 0;", false);

    RUN_TEST("1 > 1;", false);
    RUN_TEST("1 >= 1;", true);
    RUN_TEST("1 == 1;", true);
    RUN_TEST("1 <= 1;", true);
    RUN_TEST("1 < 1;", false);

    RUN_TEST("0 > 1;", false);
    RUN_TEST("0 >= 1;", false);
    RUN_TEST("0 == 1;", false);
    RUN_TEST("0 <= 1;", true);
    RUN_TEST("0 < 1;", true);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
#undef RUN_TEST
}

int testStrings() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);

    INTERPRET("\"st\" + \"ri\" + \"ng\";");
    checkIntsEqual(STACK_HEAD.type, VAL_OBJ);
    checkIntsEqual(AS_OBJ(STACK_HEAD)->type, OBJ_STRING);
    checkIntsEqual(AS_STRING(STACK_HEAD)->length, 6);

    checkIntsEqual(interpret(&vm, "\"cannot add strings and numbers\" + 1.0;"), INTERPRET_RUNTIME_ERROR);
    checkIntsEqual(interpret(&vm, "1.0 + \"cannot add numbers and strings\";"), INTERPRET_RUNTIME_ERROR);

    // test interning/equality
    INTERPRET("\"string\" == \"string\";");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(STACK_HEAD), true);

    INTERPRET("\"str\" + \"ing\" == \"string\";");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(STACK_HEAD), true);

    INTERPRET("\"first\" == \"second\";");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(STACK_HEAD), false);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testGlobals() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);

    INTERPRET("var uninitialisedGlobal;");
    checkIntsEqual(STACK_HEAD.type, VAL_NIL);

    INTERPRET("uninitialisedGlobal = 5;");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 5);

    INTERPRET("var initialisedGlobal = false;");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(STACK_HEAD), false);

    char* chapterExample = "var breakfast = \"beignets\";\n"
                           "var beverage = \"cafe au lait\";\n"
                           "breakfast = \"beignets with \" + beverage;\n"
                           "\n"
                           "print breakfast;";
    INTERPRET(chapterExample);
    checkIntsEqual(STACK_HEAD.type, VAL_OBJ);
    checkIntsEqual(AS_OBJ(STACK_HEAD)->type, OBJ_STRING);

    INTERPRET("breakfast == \"beignets with cafe au lait\";");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(STACK_HEAD), true);

    // check wide instructions

    char source[1850] = "";
    for (int i = 0; i < 129; i++) {
        char line[17];
        sprintf(line, "var g%d = %d;\n", i, i);
        strcat(source, line);
    }
    strcat(source, "g128;");

    INTERPRET(source);
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 128);

    INTERPRET("g0;");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 0);

    INTERPRET("g1;");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 1);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testLocals() {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    // basic local declaration
    INTERPRET("{ var a = 10; }");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 10);

    // falls out of scope after block closed
    checkIntsEqual(interpret(&vm, "a;"), INTERPRET_RUNTIME_ERROR);

    // local referencing global
    INTERPRET("var global = 5; { var local = global + 10; }");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 15);

    // reassigning local
    INTERPRET("{ var a = 10; a = 20; }");
    checkIntsEqual(STACK_HEAD.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(STACK_HEAD), 20);

    INTERPRET("var a = 1; { var a = 2; { var a = 3; print a; } print a; } print a;");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[0], "3");
    checkStringsEqual(printLog[1], "2");
    checkStringsEqual(printLog[2], "1");

    // can't refer to uninitialised variable in its initialiser
    checkIntsEqual(interpret(&vm, "{ var a = a; }"), INTERPRET_COMPILE_ERROR);

    // declaring a lot of locals
    char source[4161] = "{\n";
    for (int i = 0; i < 256; i++) {
        char line[18];
        sprintf(line, "\tvar l%d = %d;\n", i, i);
        strcat(source, line);
    }
    strcat(source, "\tprint l1 + l128 + l255; \n}\n");

    INTERPRET(source);
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "384");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int main() {
    return testLocals() | testGlobals() | testVmStack() | testVmArithmetic() | testNil() | testBools() | testComparisons() | testStrings();
}
