#include "memory.h"
#include "test_suite.h"

int testMemoryAllocator() {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    // zero case: create allocator too small to actually store anything
    initMemory(&freeList, sizeof(Block));
    assertNotNull(freeList.first);
    checkEqual(freeList.first->next, NULL);
    checkEqual(freeList.first->blockSize, 0);
    freeMemory(&freeList);

    // create allocator with some actual capacity
    initMemory(&freeList, 1024 * 1024);
    assertNotNull(freeList.first);
    checkEqual(freeList.first->next, NULL);
    checkEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block));

    // test smallest non-zero allocation
    size_t start = (size_t) freeList.first;
    void* ptr = newReallocate(&freeList, NULL, 0, 1);
    assertNotNull(ptr);
    checkEqual(ptr, (void*) start);
    assertNotNull(freeList.first);
    checkEqual((size_t) freeList.first, start + 1);
    checkEqual(freeList.first->next, NULL);
    checkEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1);

    // test larger allocation from same list
    ptr = newReallocate(&freeList, NULL, 0, 1024);
    assertNotNull(ptr);
    checkEqual(ptr, (void*) start + 1);
    assertNotNull(freeList.first);
    checkEqual((size_t) freeList.first, start + 1025);
    checkEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1025);

    // trying too allocate too much
    ptr = newReallocate(&freeList, NULL, 0, 1024 * 1024);
    checkEqual(ptr, NULL);
    assertNotNull(freeList.first);
    checkEqual((size_t) freeList.first, start + 1025);
    checkEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1025);

    // exhausting the allocator capacity
    ptr = newReallocate(&freeList, NULL, 0, 1024 * 1024 - sizeof(Block) - 1025);
    assertNotNull(ptr);
    checkEqual(ptr, (void*) start + 1025);
    checkEqual(freeList.first, NULL);

    // attempting to allocate from empty allocator
    ptr = newReallocate(&freeList, NULL, 0, 1);
    checkEqual(ptr, NULL);

    freeMemory(&freeList);

    return err_code;
}

int main() {
    return testMemoryAllocator();
}