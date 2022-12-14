#include "test_suite.h"
#include "vm.c"

int testVmStack() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 16 * 1024);
    VM vm;
    initVM(&vm);

    Value v0 = 1.0;
    push(&freeList, &vm, v0);
    checkFloatsEqual(pop(&vm), v0);

    Value v1 = 2.0;
    Value v2 = 4.0;
    push(&freeList, &vm, v1);
    push(&freeList, &vm, v2);

    checkFloatsEqual(pop(&vm), v2);
    checkFloatsEqual(pop(&vm), v1);

    for (int i = 0; i < 257; i++) {
        push(&freeList, &vm, i);
    }
    checkFloatsEqual(pop(&vm), 256);

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
    interpret(&freeList, &vm, source);  \
    checkFloatsEqual(vm.stack.values[0], expected); \
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

int main() {
    return testVmStack() | testVmArithmetic();
}
