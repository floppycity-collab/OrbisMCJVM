/**
 * @file test_memory_tracking.c
 * @brief Memory allocation tracking tests
 */

#include <orbisjvm/memory.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

static int g_dump_count = 0;

static void dump_callback(const orbisjvm_alloc_tracker_t* entry, void* user_data) {
    (void)user_data;
    g_dump_count++;
    orbisjvm_log_debug(ORBISJVM_LOG_CAT_MEMORY, "  Allocation: %p size=%zu flags=0x%x %s:%d %s",
                       entry->ptr, entry->size, entry->flags,
                       entry->file ? entry->file : "unknown", entry->line,
                       entry->func ? entry->func : "unknown");
}

int test_tracking_enable_disable(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing tracking enable/disable...");

    orbisjvm_set_allocation_tracking(1);

    void* ptr1 = orbisjvm_malloc(100, ORBISJVM_MEM_NONE);
    void* ptr2 = orbisjvm_malloc(200, ORBISJVM_MEM_NONE);

    size_t current, total, peak;
    uint64_t allocs, frees;
    orbisjvm_get_allocation_stats(&current, &total, &peak, &allocs, &frees);

    TEST_ASSERT(current == 2, "should have 2 current allocations");
    TEST_ASSERT(total == 300, "should have 300 bytes total");
    TEST_ASSERT(allocs == 2, "should have 2 total allocations");
    TEST_ASSERT(frees == 0, "should have 0 frees");

    orbisjvm_free(ptr1);

    orbisjvm_get_allocation_stats(&current, &total, &peak, &allocs, &frees);
    TEST_ASSERT(current == 1, "should have 1 current allocation after free");
    TEST_ASSERT(total == 200, "should have 200 bytes total after free");
    TEST_ASSERT(frees == 1, "should have 1 free");

    orbisjvm_free(ptr2);

    orbisjvm_get_allocation_stats(&current, &total, &peak, &allocs, &frees);
    TEST_ASSERT(current == 0, "should have 0 current allocations");
    TEST_ASSERT(frees == 2, "should have 2 frees");

    orbisjvm_set_allocation_tracking(0);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "tracking enable/disable test passed");
    return 0;
}

int test_dump_allocations(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing allocation dump...");

    orbisjvm_set_allocation_tracking(1);

    void* ptr1 = orbisjvm_malloc(100, ORBISJVM_MEM_NONE);
    void* ptr2 = orbisjvm_malloc(200, ORBISJVM_MEM_NONE);
    void* ptr3 = orbisjvm_malloc(300, ORBISJVM_MEM_NONE);

    g_dump_count = 0;
    orbisjvm_dump_allocations(dump_callback, NULL);
    TEST_ASSERT(g_dump_count == 3, "should dump 3 allocations");

    orbisjvm_free(ptr1);
    orbisjvm_free(ptr2);
    orbisjvm_free(ptr3);

    orbisjvm_set_allocation_tracking(0);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "allocation dump test passed");
    return 0;
}

int test_memory_leak_detection(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing memory leak detection...");

    orbisjvm_set_allocation_tracking(1);

    void* ptr = orbisjvm_malloc(500, ORBISJVM_MEM_NONE);
    TEST_ASSERT(ptr != NULL, "malloc should succeed");

    size_t leaks = orbisjvm_check_memory_leaks();
    TEST_ASSERT(leaks == 1, "should detect 1 leak");

    orbisjvm_free(ptr);

    leaks = orbisjvm_check_memory_leaks();
    TEST_ASSERT(leaks == 0, "should detect 0 leaks after free");

    orbisjvm_set_allocation_tracking(0);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "memory leak detection test passed");
    return 0;
}

int test_peak_tracking(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing peak memory tracking...");

    orbisjvm_set_allocation_tracking(1);

    void* ptr1 = orbisjvm_malloc(1000, ORBISJVM_MEM_NONE);
    void* ptr2 = orbisjvm_malloc(2000, ORBISJVM_MEM_NONE);

    size_t current, total, peak;
    orbisjvm_get_allocation_stats(&current, &total, &peak, NULL, NULL);
    TEST_ASSERT(peak >= 3000, "peak should be at least 3000");

    orbisjvm_free(ptr1);
    orbisjvm_free(ptr2);

    orbisjvm_get_allocation_stats(&current, &total, &peak, NULL, NULL);
    TEST_ASSERT(peak >= 3000, "peak should remain at 3000");

    orbisjvm_set_allocation_tracking(0);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "peak tracking test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_tracking_enable_disable() < 0) failed++;
    if (test_dump_allocations() < 0) failed++;
    if (test_memory_leak_detection() < 0) failed++;
    if (test_peak_tracking() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All memory tracking tests passed!");
    return 0;
}