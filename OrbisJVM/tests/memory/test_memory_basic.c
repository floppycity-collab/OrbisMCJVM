/**
 * @file test_memory_basic.c
 * @brief Basic memory allocation tests
 */

#include <orbisjvm/memory.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_malloc_free(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing malloc/free...");

    void* ptr = orbisjvm_malloc(1024, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "malloc should succeed");

    memset(ptr, 0xAA, 1024);

    orbisjvm_free(ptr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "malloc/free test passed");
    return 0;
}

int test_calloc(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing calloc...");

    void* ptr = orbisjvm_calloc(100, 32, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "calloc should succeed");

    /* Verify zero-initialized */
    uint8_t* bytes = (uint8_t*)ptr;
    for (int i = 0; i < 3200; i++) {
        TEST_ASSERT(bytes[i] == 0, "calloc should zero-initialize");
    }

    orbisjvm_free(ptr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "calloc test passed");
    return 0;
}

int test_realloc(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing realloc...");

    void* ptr = orbisjvm_malloc(512, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "initial malloc should succeed");

    memset(ptr, 0x55, 512);

    /* Expand */
    ptr = orbisjvm_realloc(ptr, 1024, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "realloc expand should succeed");

    /* Verify original data preserved */
    uint8_t* bytes = (uint8_t*)ptr;
    for (int i = 0; i < 512; i++) {
        TEST_ASSERT(bytes[i] == 0x55, "realloc should preserve data");
    }

    /* Shrink */
    ptr = orbisjvm_realloc(ptr, 256, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "realloc shrink should succeed");

    orbisjvm_free(ptr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "realloc test passed");
    return 0;
}

int test_aligned_alloc(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing aligned_alloc...");

    for (size_t align = 16; align <= 4096; align *= 2) {
        void* ptr = orbisjvm_aligned_alloc(align, 1024, ORBISJVM_MEM_NONE);
        TEST_ASSERT(ptr != NULL, "aligned_alloc should succeed");

        uintptr_t addr = (uintptr_t)ptr;
        TEST_ASSERT((addr % align) == 0, "pointer should be aligned");

        orbisjvm_free(ptr);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "aligned_alloc test passed");
    return 0;
}

int test_zero_size(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing zero-size allocations...");

    void* ptr = orbisjvm_malloc(0, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr == NULL, "malloc(0) should return NULL");

    ptr = orbisjvm_calloc(0, 100, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr == NULL, "calloc(0, n) should return NULL");

    ptr = orbisjvm_calloc(100, 0, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr == NULL, "calloc(n, 0) should return NULL");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "zero-size test passed");
    return 0;
}

int test_large_allocation(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing large allocation (16 MB)...");

    void* ptr = orbisjvm_malloc(16 * 1024 * 1024, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "16 MB allocation should succeed");

    memset(ptr, 0xFF, 16 * 1024 * 1024);

    orbisjvm_free(ptr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "large allocation test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_malloc_free() < 0) failed++;
    if (test_calloc() < 0) failed++;
    if (test_realloc() < 0) failed++;
    if (test_aligned_alloc() < 0) failed++;
    if (test_zero_size() < 0) failed++;
    if (test_large_allocation() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All memory basic tests passed!");
    return 0;
}