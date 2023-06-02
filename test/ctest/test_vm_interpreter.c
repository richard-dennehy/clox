#include "test_suite.h"
#include "vm.c"

#define INTERPRET(source) assert(interpret(&vm, source) == INTERPRET_OK)
// 0 is script function object
static char printLog[32][64];
static int printed = 0;

int fakePrintf(const char* format, ...) {
    assert(printed < 32 || !"Too many things printed");
    va_list args;
    va_start(args, format);
    int result = vsnprintf(printLog[printed++], 64, format, args);
    va_end(args);
    return result;
}

void resetPrintLog(void) {
    printed = 0;
}

int testVmStack(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 32 * 1024);
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

int testVmArithmetic(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

#define RUN_TEST(source, expected) do { \
    int prevPrinted = printed;          \
    char expectedPrint[9] = "";                                    \
    snprintf(expectedPrint, 9, "%g", expected);                                    \
    INTERPRET("print " source);  \
    checkIntsEqual(printed, prevPrinted + 1);                                    \
    checkStringsEqual(printLog[prevPrinted], expectedPrint); \
} while(0)

    RUN_TEST("-2;", -2.0);
    RUN_TEST("3 * 4;", 12.0);
    RUN_TEST("5 + 6;", 11.0);
    RUN_TEST("7 - 8;", -1.0);
    RUN_TEST("9 / 10;", 0.9);
    RUN_TEST("(-1 + 2) * 3 - -4;", 7.0);

#undef RUN_TEST
    freeVM(&vm);
    freeMemory(&freeList);

    return err_code;
}

int testNil(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    INTERPRET("print nil;");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "nil");

    INTERPRET("print nil == nil;");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "true");

    INTERPRET("print nil != nil;");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "false");

    INTERPRET("print !nil;");
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "true");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testBools(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    INTERPRET("print true;");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "true");

    INTERPRET("print false;");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "false");

    INTERPRET("print !true;");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "false");

    INTERPRET("print !false;");
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "true");

    INTERPRET("print true == true;");
    checkIntsEqual(printed, 5);
    checkStringsEqual(printLog[4], "true");

    INTERPRET("print false == false;");
    checkIntsEqual(printed, 6);
    checkStringsEqual(printLog[5], "true");

    INTERPRET("print true == false;");
    checkIntsEqual(printed, 7);
    checkStringsEqual(printLog[6], "false");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testComparisons(void) {
#define RUN_TEST(source, expected) do { \
    int prevPrinted = printed;                                    \
    INTERPRET("print " source);  \
    checkIntsEqual(printed, prevPrinted + 1);                                    \
    checkStringsEqual(printLog[prevPrinted], expected); \
} while(0)

    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    RUN_TEST("1 > 0;", "true");
    RUN_TEST("1 >= 0;", "true");
    RUN_TEST("1 == 0;", "false");
    RUN_TEST("1 <= 0;", "false");
    RUN_TEST("1 < 0;", "false");

    RUN_TEST("1 > 1;", "false");
    RUN_TEST("1 >= 1;", "true");
    RUN_TEST("1 == 1;", "true");
    RUN_TEST("1 <= 1;", "true");
    RUN_TEST("1 < 1;", "false");

    RUN_TEST("0 > 1;", "false");
    RUN_TEST("0 >= 1;", "false");
    RUN_TEST("0 == 1;", "false");
    RUN_TEST("0 <= 1;", "true");
    RUN_TEST("0 < 1;", "true");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
#undef RUN_TEST
}

int testStrings(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    INTERPRET("var string = \"st\" + \"ri\" + \"ng\";");
    Value string;
    ObjString* key = copyString(&vm, NULL, "string", strlen("string"));
    checkTrue(tableGet(&vm.globals, key, &string));
    checkIntsEqual(string.type, VAL_OBJ);
    checkIntsEqual(AS_OBJ(string)->type, OBJ_STRING);
    checkIntsEqual(AS_STRING(string)->length, 6);

    checkIntsEqual(interpret(&vm, "\"cannot add strings and numbers\" + 1.0;"), INTERPRET_RUNTIME_ERROR);
    checkIntsEqual(interpret(&vm, "1.0 + \"cannot add numbers and strings\";"), INTERPRET_RUNTIME_ERROR);

    // test interning/equality
    INTERPRET("print \"string\" == \"string\";");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "true");

    INTERPRET("print \"str\" + \"ing\" == \"string\";");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "true");

    INTERPRET("print \"first\" == \"second\";");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "false");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testGlobals(void) {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    Value global;
    INTERPRET("var uninitialisedGlobal;");
    ObjString* key = copyString(&vm, NULL, "uninitialisedGlobal", strlen("uninitialisedGlobal"));
    checkTrue(tableGet(&vm.globals, key, &global));
    checkIntsEqual(global.type, VAL_NIL);

    INTERPRET("uninitialisedGlobal = 5;");
    checkTrue(tableGet(&vm.globals, key, &global));
    checkIntsEqual(global.type, VAL_NUMBER);
    checkIntsEqual(AS_NUMBER(global), 5);

    INTERPRET("var initialisedGlobal = false;");
    key = copyString(&vm, NULL, "initialisedGlobal", strlen("initialisedGlobal"));
    checkTrue(tableGet(&vm.globals, key, &global));
    checkIntsEqual(global.type, VAL_BOOL);
    checkIntsEqual(AS_BOOL(global), false);

    char* chapterExample = "var breakfast = \"beignets\";\n"
                           "var beverage = \"cafe au lait\";\n"
                           "breakfast = \"beignets with \" + beverage;\n"
                           "\n"
                           "print breakfast;";
    INTERPRET(chapterExample);
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "beignets with cafe au lait");

    // check wide instructions

    char source[1850] = "";
    for (int i = 0; i < 129; i++) {
        char line[17];
        sprintf(line, "var g%d = %d;\n", i, i);
        strcat(source, line);
    }
    strcat(source, "print g128;");

    INTERPRET(source);
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "128");

    INTERPRET("print g0;");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "0");

    INTERPRET("print g1;");
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "1");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testLocals(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    // basic local declaration
    INTERPRET("{ var a = 10; print a; }");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "10");

    // falls out of scope after block closed
    checkIntsEqual(interpret(&vm, "a;"), INTERPRET_RUNTIME_ERROR);

    // local referencing global
    INTERPRET("var global = 5; { var local = global + 10; print local; }");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "15");

    // reassigning local
    INTERPRET("{ var a = 10; a = 20; print a; }");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "20");

    INTERPRET("var a = 1; { var a = 2; { var a = 3; print a; } print a; } print a;");
    checkIntsEqual(printed, 6);
    checkStringsEqual(printLog[3], "3");
    checkStringsEqual(printLog[4], "2");
    checkStringsEqual(printLog[5], "1");

    // can't refer to uninitialised variable in its initialiser
    checkIntsEqual(interpret(&vm, "{ var a = a; }"), INTERPRET_COMPILE_ERROR);

    // declaring a lot of locals
    char source[4179] = "{\n";
    for (int i = 0; i < 257; i++) {
        char line[18];
        sprintf(line, "\tvar l%d = %d;\n", i, i);
        strcat(source, line);
    }
    strcat(source, "\tprint l1 + l128 + l256; \n}\n");

    INTERPRET(source);
    checkIntsEqual(printed, 7);
    checkStringsEqual(printLog[6], "385");

    // can only assign to valid assigment target
    checkIntsEqual(interpret(&vm, "var a = 0; var b = 1; var c = 2; var d = 3; a + b = c + d"),
                   INTERPRET_COMPILE_ERROR);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testControlFlow(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    INTERPRET("if (true) { print \"simple if\"; }");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "simple if");

    INTERPRET("if (false) { print \"don't execute this\"; }");
    checkIntsEqual(printed, 1);

    INTERPRET("if (true) { print \"if branch is taken\"; } else { print \"else branch is not\"; }");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "if branch is taken");

    INTERPRET("if (false) { print \"if branch is not taken\"; } else { print \"else branch is\"; }");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "else branch is");

    INTERPRET("var done = false; while (!done) { print \"simple while\"; done = true; } ");
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "simple while");

    INTERPRET("while (false) { print \"this shouldn't happen\"; }");
    checkIntsEqual(printed, 4);

    INTERPRET("var i = 0; while (i < 3) { print i; i = i + 1; }");
    checkIntsEqual(printed, 7);
    checkStringsEqual(printLog[4], "0");
    checkStringsEqual(printLog[5], "1");
    checkStringsEqual(printLog[6], "2");

    // TODO test infinite for loops i.e. `for (;;)` once there's a way to break out
    INTERPRET(
            "var done = false; for (; !done;) { print \"for initialiser and increment are optional\"; done = true; }");
    checkIntsEqual(printed, 8);
    checkStringsEqual(printLog[7], "for initialiser and increment are optional");

    INTERPRET("var i = 0; for (; i < 3; i = i + 1) { print \"for initialiser is optional\"; }");
    checkIntsEqual(printed, 11);
    checkStringsEqual(printLog[8], "for initialiser is optional");
    checkStringsEqual(printLog[9], "for initialiser is optional");
    checkStringsEqual(printLog[10], "for initialiser is optional");

    INTERPRET(
            "var i = 100; for (i = 0; i < 3; i = i + 1) { print \"for initialiser doesn't need to be a var declaration\"; }");
    checkIntsEqual(printed, 14);
    checkStringsEqual(printLog[11], "for initialiser doesn't need to be a var declaration");

    INTERPRET("for (var i = 0; i < 3; i = i + 1) { print i; }");
    checkIntsEqual(printed, 17);
    checkStringsEqual(printLog[14], "0");
    checkStringsEqual(printLog[15], "1");
    checkStringsEqual(printLog[16], "2");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testFunctions(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    INTERPRET("fun empty() { print \"not executed\"; }");
    checkIntsEqual(printed, 0);

    INTERPRET("fun stuff() {} print stuff;");
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "<fn stuff>");

    INTERPRET("fun function() { print \"you called?\"; } function();");
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "you called?");

    INTERPRET("fun returnStuff() { return \"stuff\"; } print returnStuff();");
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "stuff");

    INTERPRET("fun add(a, b) { return a + b; } print add(2, 3);");
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "5");

    // TODO test error messages
    checkIntsEqual(interpret(&vm, "fun oops(a, b) {} oops();"), INTERPRET_RUNTIME_ERROR);
    checkIntsEqual(interpret(&vm, "fun oops(a, b) {} oops(1);"), INTERPRET_RUNTIME_ERROR);
    checkIntsEqual(interpret(&vm, "fun oops(a, b) {} oops(1, 2, 3);"), INTERPRET_RUNTIME_ERROR);

    checkIntsEqual(interpret(&vm, "var notFunction; notFunction();"), INTERPRET_RUNTIME_ERROR);

    INTERPRET("fun addition(a, b) { return a + b; } var add = addition; print add(5, 7);");
    checkIntsEqual(printed, 5);
    checkStringsEqual(printLog[4], "12");

    INTERPRET(
            "fun weirdAdd(a, b) { if (b <= 0) { return a; } else { return weirdAdd(a + 1, b - 1); } } print weirdAdd(5, 10);");
    checkIntsEqual(printed, 6);
    checkStringsEqual(printLog[5], "15");

    checkIntsEqual(interpret(&vm, "return \"oh no, top level return\";"), INTERPRET_COMPILE_ERROR);

    INTERPRET("var now = clock();");
    Value time;
    ObjString* key = copyString(&vm, NULL, "now", strlen("now"));
    checkTrue(tableGet(&vm.globals, key, &time));
    checkIntsEqual(time.type, VAL_NUMBER);
    // not checking value as it's dependent on how fast the test runs

    INTERPRET("print clock;");
    checkIntsEqual(printed, 7);
    checkStringsEqual(printLog[6], "<native fn>");

    checkIntsEqual(interpret(&vm, "clock(\"doesn't take arguments\");"), INTERPRET_RUNTIME_ERROR);

    // check it actually works
    INTERPRET("print sqrt(4);");
    checkIntsEqual(printed, 8);
    checkStringsEqual(printLog[7], "2");

    checkIntsEqual(interpret(&vm, "sqrt(\"four\");"), INTERPRET_RUNTIME_ERROR);

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testClosures(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    const char* simpleClosure =
            "fun outer() {\n"
            "  var x = \"outside\";\n"
            "  fun inner() {\n"
            "    print x;\n"
            "  }\n"
            "  inner();\n"
            "}\n"
            "outer();";
    INTERPRET(simpleClosure);
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "outside");

    const char* escapesFromStack =
            "fun outer() {\n"
            "  var x = \"outside\";\n"
            "  fun inner() {\n"
            "    print x;\n"
            "  }\n"
            "\n"
            "  return inner;\n"
            "}\n"
            "\n"
            "var closure = outer();\n"
            "closure();";

    INTERPRET(escapesFromStack);
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "outside");

    const char* closuresCaptureVariables =
            "var globalSet;\n"
            "var globalGet;\n"
            "\n"
            "fun main() {\n"
            "  var a = \"initial\";\n"
            "\n"
            "  fun set() { a = \"updated\"; }\n"
            "  fun get() { print a; }\n"
            "\n"
            "  globalSet = set;\n"
            "  globalGet = get;\n"
            "}\n"
            "\n"
            "main();\n"
            "globalSet();\n"
            "globalGet();";

    INTERPRET(closuresCaptureVariables);
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "updated");

    const char* capturingMultipleVariables =
            "var globalOne;\n"
            "var globalTwo;\n"
            "\n"
            "fun main() {\n"
            "  {\n"
            "    var a = \"one\";\n"
            "    fun one() {\n"
            "      print a;\n"
            "    }\n"
            "    globalOne = one;\n"
            "  }\n"
            "\n"
            "  {\n"
            "    var a = \"two\";\n"
            "    fun two() {\n"
            "      print a;\n"
            "    }\n"
            "    globalTwo = two;\n"
            "  }\n"
            "}\n"
            "\n"
            "main();\n"
            "globalOne();\n"
            "globalTwo();";

    INTERPRET(capturingMultipleVariables);
    checkIntsEqual(printed, 5);
    checkStringsEqual(printLog[3], "one");
    checkStringsEqual(printLog[4], "two");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int testClasses(void) {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    VM vm;
    initMemory(&freeList, 256 * 1024);
    initVM(&freeList, &vm);
    resetPrintLog();
    vm.print = fakePrintf;

    const char* emptyClass =
            "class Brioche {}"
            "print Brioche;";
    INTERPRET(emptyClass);
    checkIntsEqual(printed, 1);
    checkStringsEqual(printLog[0], "Brioche");

    const char* invokeConstructor =
            "class Brioche {}"
            "print Brioche();";
    INTERPRET(invokeConstructor);
    checkIntsEqual(printed, 2);
    checkStringsEqual(printLog[1], "Brioche instance");

    const char* getSetProperties =
            "class Pair {}\n"
            "\n"
            "var pair = Pair();\n"
            "pair.first = 1;\n"
            "pair.second = 2;\n"
            "print pair.first + pair.second;";

    INTERPRET(getSetProperties);
    checkIntsEqual(printed, 3);
    checkStringsEqual(printLog[2], "3");

    const char* simpleClassMethod =
            "class Scone {"
            "  topping(first, second) {"
            "    print \"scone with \" + first + \" and \" + second;"
            "  }"
            "}"
            ""
            "var scone = Scone();"
            "scone.topping(\"berries\", \"cream\");";

    INTERPRET(simpleClassMethod);
    checkIntsEqual(printed, 4);
    checkStringsEqual(printLog[3], "scone with berries and cream");

    const char* classInitialiser =
            "class CoffeeMaker {\n"
            "  init(coffee) {\n"
            "    this.coffee = coffee;\n"
            "  }\n"
            "\n"
            "  brew() {\n"
            "    print \"Enjoy your cup of \" + this.coffee;\n"
            "\n"
            "    // No reusing the grounds!\n"
            "    this.coffee = nil;\n"
            "  }\n"
            "}\n"
            "\n"
            "var maker = CoffeeMaker(\"coffee and chicory\");\n"
            "maker.brew();";

    INTERPRET(classInitialiser);
    checkIntsEqual(printed, 5);
    checkStringsEqual(printLog[4], "Enjoy your cup of coffee and chicory");

    freeVM(&vm);
    freeMemory(&freeList);
    return err_code;
}

int main(void) {
    return testGlobals() | testLocals() | testControlFlow() | testVmStack() | testVmArithmetic() | testNil() |
           testBools() | testComparisons() | testStrings() | testFunctions() | testClosures() | testClasses();
}
