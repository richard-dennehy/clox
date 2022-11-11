#include "memory.h"
#include "test_suite.h"

int testAllocation() {
    int err_code = TEST_SUCCEEDED;
    FreeList freeList;
    // zero case: create allocator too small to actually store anything
    initMemory(&freeList, sizeof(Block));
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 0);
    freeMemory(&freeList);

    // create allocator with some actual capacity
    initMemory(&freeList, 1024 * 1024);
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block));

    // test smallest non-zero allocation
    size_t start = (size_t) freeList.first;
    void* ptr = newReallocate(&freeList, NULL, 0, 1);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1);

    // test larger allocation from same list
    ptr = newReallocate(&freeList, NULL, 0, 1024);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start + 1);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1025);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1025);

    // trying too allocate too much
    ptr = newReallocate(&freeList, NULL, 0, 1024 * 1024);
    checkPtrsEqual(ptr, NULL);
    assertNotNull(freeList.first);
    checkLongsEqual((size_t) freeList.first, start + 1025);
    checkLongsEqual(freeList.first->blockSize, 1024 * 1024 - sizeof(Block) - 1025);

    // exhausting the allocator capacity
    ptr = newReallocate(&freeList, NULL, 0, 1024 * 1024 - sizeof(Block) - 1025);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, (void*) start + 1025);
    checkPtrsEqual(freeList.first, NULL);

    // attempting to allocate from empty allocator
    ptr = newReallocate(&freeList, NULL, 0, 1);
    checkPtrsEqual(ptr, NULL);

    freeMemory(&freeList);

    return err_code;
}

int testReallocation() {
    int err_code = TEST_SUCCEEDED;

    FreeList freeList;
    initMemory(&freeList, 1024);

    // allocate new block
    void* ptr = newReallocate(&freeList, NULL, 0, 64);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_);
    assertNotNull(freeList.first);
    checkPtrsEqual(freeList.first->next, NULL);
    checkLongsEqual(freeList.first->blockSize, 1024 - 64 - sizeof(Block));

    // allocate new block and return old block to free list
    ptr = newReallocate(&freeList, ptr, 64, 128);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_ + 64);
    assertNotNull(freeList.first);
    assertNotNull(freeList.first->next);
    checkLongsEqual(freeList.first->blockSize, 1024 - 192 - sizeof(Block));
    checkPtrsEqual((void*) freeList.first->next, freeList.ptr_);
    checkPtrsEqual(freeList.first->next->next, NULL);
    checkLongsEqual(freeList.first->next->blockSize, 64);
    
    // another allocation, to consume most of the remaining capacity
    size_t allocationSize = 1024 - 224 - sizeof(Block);
    ptr = newReallocate(&freeList, NULL, 0, allocationSize);
    assertNotNull(ptr);
    checkPtrsEqual(ptr, freeList.ptr_ + 192);
    assertNotNull(freeList.first);
    assertNotNull(freeList.first->next);
    checkLongsEqual(freeList.first->blockSize, 32);
    checkPtrsEqual((void*) freeList.first->next, freeList.ptr_);

    // allocate block of same size as first block; expect first block again, not new block
    ptr = newReallocate(&freeList, NULL, 0, 64);
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