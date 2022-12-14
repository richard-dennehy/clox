#include "test_suite.h"
#include "vm.c"

#define INTERPRET(source) assert(interpret(&freeList, &vm, source) == INTERPRET_OK)
#define STACK_HEAD vm.stack.values[0]

int testVmStack() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 16 * 1024);
    VM vm;
    initVM(&vm);

    Value v0 = NUMBER_VAL(1.0);
    push(&freeList, &vm, v0);
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 1.0);

    Value v1 = NUMBER_VAL(2.0);
    Value v2 = NUMBER_VAL(4.0);
    push(&freeList, &vm, v1);
    push(&freeList, &vm, v2);

    checkFloatsEqual(AS_NUMBER(pop(&vm)), 4.0);
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 2.0);

    for (int i = 0; i < 257; i++) {
        push(&freeList, &vm, NUMBER_VAL(i));
    }
    checkFloatsEqual(AS_NUMBER(pop(&vm)), 256);

    freeVM(&freeList, &vm);
    freeMemory(&freeList);
    return err_code;
}

int testVmArithmetic() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&vm);

    // NOTE: these tests will probably stop working once the temporary return instruction stops being added to all chunks - will need to adjust the indexes
#define RUN_TEST(source, expected) do { \
    INTERPRET(source);  \
    Value result = vm.stack.values[0];                                    \
    checkIntsEqual(result.type, VAL_NUMBER);                                    \
    checkFloatsEqual(result.as.number, expected); \
} while(0)

    RUN_TEST("-2", -2.0);
    RUN_TEST("3 * 4", 12.0);
    RUN_TEST("5 + 6", 11.0);
    RUN_TEST("7 - 8", -1.0);
    RUN_TEST("9 / 10", 0.9);
    RUN_TEST("(-1 + 2) * 3 - -4", 7);

#undef RUN_TEST
    freeVM(&freeList, &vm);
    freeMemory(&freeList);

    return err_code;
}

int testNil() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&vm);

    INTERPRET("nil");
    checkIntsEqual(vm.stack.values[0].type, VAL_NIL);

    INTERPRET("nil == nil");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("nil != nil");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!nil");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    freeVM(&freeList, &vm);
    freeMemory(&freeList);
    return err_code;
}

int testBools() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    VM vm;
    initMemory(&freeList, 16 * 1024);
    initVM(&vm);

    INTERPRET("true");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("false");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!true");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    INTERPRET("!false");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("true == true");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("false == false");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, true);

    INTERPRET("true == false");
    checkIntsEqual(STACK_HEAD.type, VAL_BOOL);
    checkIntsEqual(STACK_HEAD.as.boolean, false);

    freeVM(&freeList, &vm);
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
    initVM(&vm);

    RUN_TEST("1 > 0", true);
    RUN_TEST("1 >= 0", true);
    RUN_TEST("1 == 0", false);
    RUN_TEST("1 <= 0", false);
    RUN_TEST("1 < 0", false);

    RUN_TEST("1 > 1", false);
    RUN_TEST("1 >= 1", true);
    RUN_TEST("1 == 1", true);
    RUN_TEST("1 <= 1", true);
    RUN_TEST("1 < 1", false);

    RUN_TEST("0 > 1", false);
    RUN_TEST("0 >= 1", false);
    RUN_TEST("0 == 1", false);
    RUN_TEST("0 <= 1", true);
    RUN_TEST("0 < 1", true);

    freeVM(&freeList, &vm);
    freeMemory(&freeList);
    return err_code;
#undef RUN_TEST
}

int main() {
    return testVmStack() | testVmArithmetic() | testNil() | testBools() | testComparisons();
}
