/**
 * @file test_thread_basic.c
 * @brief Basic threading tests
 */

#include <orbisjvm/threading.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <stdatomic.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

static atomic_int g_thread_counter = 0;
static orbisjvm_mutex_t g_counter_mutex;

void* thread_increment(void* arg) {
    int iterations = *(int*)arg;

    for (int i = 0; i < iterations; i++) {
        orbisjvm_mutex_lock(g_counter_mutex);
        atomic_fetch_add(&g_thread_counter, 1);
        orbisjvm_mutex_unlock(g_counter_mutex);
    }

    return NULL;
}

int test_thread_create_join(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing thread create/join...");

    orbisjvm_thread_t thread;
    int ret = orbisjvm_thread_create(&thread, NULL, thread_increment, &(int){1000});
    TEST_ASSERT(ret == 0, "thread_create should succeed");

    void* retval;
    ret = orbisjvm_thread_join(thread, &retval);
    TEST_ASSERT(ret == 0, "thread_join should succeed");
    TEST_ASSERT(retval == NULL, "thread return value should be NULL");

    TEST_ASSERT(atomic_load(&g_thread_counter) == 1000, "counter should be 1000");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "thread create/join test passed");
    return 0;
}

int test_multiple_threads(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing multiple threads...");

    const int num_threads = 4;
    const int iterations = 2500;
    orbisjvm_thread_t threads[num_threads];

    atomic_store(&g_thread_counter, 0);

    for (int i = 0; i < num_threads; i++) {
        int ret = orbisjvm_thread_create(&threads[i], NULL, thread_increment, &iterations);
        TEST_ASSERT(ret == 0, "thread_create should succeed");
    }

    for (int i = 0; i < num_threads; i++) {
        void* retval;
        int ret = orbisjvm_thread_join(threads[i], &retval);
        TEST_ASSERT(ret == 0, "thread_join should succeed");
    }

    TEST_ASSERT(atomic_load(&g_thread_counter) == num_threads * iterations,
                "counter should match expected total");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "multiple threads test passed");
    return 0;
}

int test_thread_attributes(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing thread attributes...");

    orbisjvm_thread_attr_t attr;
    orbisjvm_thread_attr_init(&attr);

    orbisjvm_thread_attr_set_stack_size(&attr, 256 * 1024);
    orbisjvm_thread_attr_set_priority(&attr, 5);
    orbisjvm_thread_attr_set_name(&attr, "test-thread");

    size_t stack_size;
    orbisjvm_thread_attr_get_stack_size(&attr, &stack_size);
    TEST_ASSERT(stack_size == 256 * 1024, "stack size should be set");

    int priority;
    orbisjvm_thread_attr_get_priority(&attr, &priority);
    TEST_ASSERT(priority == 5, "priority should be set");

    orbisjvm_thread_attr_destroy(&attr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "thread attributes test passed");
    return 0;
}

int test_thread_self_equal(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing thread_self and thread_equal...");

    orbisjvm_thread_t self = orbisjvm_thread_self();
    TEST_ASSERT(self != NULL, "thread_self should return valid handle");

    int equal = orbisjvm_thread_equal(self, self);
    TEST_ASSERT(equal == 1, "thread should equal itself");

    orbisjvm_thread_t self2 = orbisjvm_thread_self();
    equal = orbisjvm_thread_equal(self, self2);
    TEST_ASSERT(equal == 1, "thread_self should be consistent");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "thread_self/equal test passed");
    return 0;
}

int test_thread_sleep(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing thread sleep...");

    uint64_t start = orbisjvm_get_monotonic_ms();
    orbisjvm_thread_sleep_ms(50);
    uint64_t end = orbisjvm_get_monotonic_ms();

    uint64_t elapsed = end - start;
    TEST_ASSERT(elapsed >= 40, "should sleep at least 40ms");
    TEST_ASSERT(elapsed <= 200, "should not sleep too long");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "thread sleep test passed (elapsed: %llu ms)", elapsed);
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_thread_create_join() < 0) failed++;
    if (test_multiple_threads() < 0) failed++;
    if (test_thread_attributes() < 0) failed++;
    if (test_thread_self_equal() < 0) failed++;
    if (test_thread_sleep() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All threading basic tests passed!");
    return 0;
}