/**
 * @file test_memory_page_alloc.c
 * @brief Page allocation tests
 */

#include <orbisjvm/memory.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_page_alloc_free(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing page_alloc/page_free...");

    size_t page_size = orbisjvm_get_page_size();
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Page size: %zu bytes", page_size);

    void* ptr = orbisjvm_page_alloc(1, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "page_alloc(1) should succeed");

    uintptr_t addr = (uintptr_t)ptr;
    TEST_ASSERT((addr % page_size) == 0, "page_alloc should return page-aligned address");

    memset(ptr, 0xAB, page_size);

    orbisjvm_page_free(ptr, 1);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "page_alloc/page_free test passed");
    return 0;
}

int test_multiple_pages(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing multiple page allocation...");

    void* ptr = orbisjvm_page_alloc(10, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "page_alloc(10) should succeed");

    size_t page_size = orbisjvm_get_page_size();
    memset(ptr, 0xCD, 10 * page_size);

    orbisjvm_page_free(ptr, 10);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "multiple pages test passed");
    return 0;
}

int test_page_aligned_alloc(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing page-aligned malloc...");

    void* ptr = orbisjvm_malloc(4096, ORBISJVM_MEM_ALIGNED);
    TEST_ASSERT(ptr != NULL, "aligned malloc should succeed");

    uintptr_t addr = (uintptr_t)ptr;
    size_t page_size = orbisjvm_get_page_size();
    TEST_ASSERT((addr % page_size) == 0, "MEM_ALIGNED should return page-aligned address");

    orbisjvm_free(ptr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "page-aligned malloc test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_page_alloc_free() < 0) failed++;
    if (test_multiple_pages() < 0) failed++;
    if (test_page_aligned_alloc() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All page allocation tests passed!");
    return 0;
}