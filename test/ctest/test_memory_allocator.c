#include "memory.h"
#include "test_suite.h"

int testAllocation() {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    // minimum sized free list (needs to be big enough to store at least one block metadata)
    initMemory(&freeList, sizeof(Block));
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 16);
    freeMemory(&freeList);

    // create allocator with some actual capacity
    initMemory(&freeList, 1024 * 1024);
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024);

    // test smallest non-zero allocation
    size_t start = (size_t) freeList.first;
    void* ptr = reallocate(&freeList, NULL, 0, 1);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - 1);

    // test larger allocation from same list
    ptr = reallocate(&freeList, NULL, 0, 1024);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start + 1);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1025);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - 1025);

    // trying too allocate too much
    ptr = reallocate(&freeList, NULL, 0, 1024 * 1024);
    checkPtrsEqual(ptr, NULL);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1025);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - 1025);

    // exhausting the allocator capacity
    ptr = reallocate(&freeList, NULL, 0, 1024 * 1024 - 1025);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start + 1025);
    checkPtrsEqual(freeList.first, NULL);

    // attempting to allocate from empty allocator
    ptr = reallocate(&freeList, NULL, 0, 1);
    checkPtrsEqual(ptr, NULL);

    freeMemory(&freeList);

    return err_code;
}

int testReallocation() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 1024);

    // allocate new block
    void* ptr = reallocate(&freeList, NULL, 0, 64);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_);
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 - 64);

    // write some data to block
    int* numbers = (int*) ptr;
    numbers[0] = 32;
    numbers[1] = 64;
    numbers[2] = 128;
    numbers[3] = 256;

    // allocate new block and return old block to free list
    ptr = reallocate(&freeList, ptr, 64, 128);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_ + 64);
    assertNotNull(freeList.first);
    assertNotNull(freeList.first->next);
    checkLongsEqual(freeList.first->blockSize, 1024 - 192);
    checkPtrsEqual((void*) freeList.first->next, freeList.ptr_);
    checkPtrsEqual(freeList.first->next->next, NULL);
    checkLongsEqual(freeList.first->next->blockSize, 64);

    // check data has been copied to new block
    numbers = (int*) ptr;
    checkIntsEqual(numbers[0], 32);
    checkIntsEqual(numbers[1], 64);
    checkIntsEqual(numbers[2], 128);
    checkIntsEqual(numbers[3], 256);

    // another allocation, to consume most of the remaining capacity
    size_t allocationSize = 1024 - 224;
    ptr = reallocate(&freeList, NULL, 0, allocationSize);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_ + 192);
    assertNotNull(freeList.first);
    assertNotNull(freeList.first->next);
    checkLongsEqual(freeList.first->blockSize, 32);
    checkPtrsEqual((void*) freeList.first->next, freeList.ptr_);

    // allocate block of same size as first block; expect first block again, not new block
    ptr = reallocate(&freeList, NULL, 0, 64);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_);
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 32);

    freeMemory(&freeList);

    return err_code;
}

int main() {
    return testAllocation() || testReallocation();
}