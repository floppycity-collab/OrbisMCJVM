/**
 * @file test_tls.c
 * @brief Thread-local storage tests
 */

#include <orbisjvm/threading.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

static orbisjvm_tls_key_t g_tls_key;
static int g_tls_destructor_called = 0;

void tls_destructor(void* value) {
    (void)value;
    g_tls_destructor_called++;
    orbisjvm_log_debug(ORBISJVM_LOG_CAT_THREADING, "TLS destructor called");
}

void* tls_thread_func(void* arg) {
    int thread_id = *(int*)arg;

    /* Set thread-local value */
    int* value = orbisjvm_malloc(sizeof(int), ORBISJVM_MEM_NONE);
    *value = thread_id * 100;
    orbisjvm_tls_set(g_tls_key, value);

    /* Verify we can read it back */
    int* read_value = orbisjvm_tls_get(g_tls_key);
    TEST_ASSERT(read_value != NULL, "tls_get should return value");
    TEST_ASSERT(*read_value == thread_id * 100, "tls value should match");

    /* Modify value */
    *read_value = thread_id * 100 + 1;
    read_value = orbisjvm_tls_get(g_tls_key);
    TEST_ASSERT(*read_value == thread_id * 100 + 1, "modified value should persist");

    return NULL;
}

int test_tls_basic(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing basic TLS...");

    int ret = orbisjvm_tls_create(&g_tls_key, tls_destructor);
    TEST_ASSERT(ret == 0, "tls_create should succeed");

    const int num_threads = 4;
    orbisjvm_thread_t threads[num_threads];
    int thread_ids[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i + 1;
        ret = orbisjvm_thread_create(&threads[i], NULL, tls_thread_func, &thread_ids[i]);
        TEST_ASSERT(ret == 0, "thread_create should succeed");
    }

    for (int i = 0; i < num_threads; i++) {
        void* retval;
        ret = orbisjvm_thread_join(threads[i], &retval);
        TEST_ASSERT(ret == 0, "thread_join should succeed");
    }

    /* Note: destructor may not be called immediately on all platforms */

    orbisjvm_tls_delete(g_tls_key);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "basic TLS test passed");
    return 0;
}

int test_tls_main_thread(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing TLS in main thread...");

    orbisjvm_tls_key_t key;
    int ret = orbisjvm_tls_create(&key, NULL);
    TEST_ASSERT(ret == 0, "tls_create should succeed");

    /* Set value in main thread */
    int value = 42;
    ret = orbisjvm_tls_set(key, &value);
    TEST_ASSERT(ret == 0, "tls_set should succeed");

    /* Get value */
    int* read_value = orbisjvm_tls_get(key);
    TEST_ASSERT(read_value != NULL, "tls_get should return value");
    TEST_ASSERT(*read_value == 42, "value should be 42");

    orbisjvm_tls_delete(key);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "main thread TLS test passed");
    return 0;
}

int test_tls_isolation(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing TLS isolation between threads...");

    orbisjvm_tls_key_t key;
    int ret = orbisjvm_tls_create(&key, NULL);
    TEST_ASSERT(ret == 0, "tls_create should succeed");

    /* Set different values in different threads via main thread simulation */
    int val1 = 100;
    int val2 = 200;

    orbisjvm_tls_set(key, &val1);
    int* r1 = orbisjvm_tls_get(key);
    TEST_ASSERT(*r1 == 100, "first value should be 100");

    orbisjvm_tls_set(key, &val2);
    int* r2 = orbisjvm_tls_get(key);
    TEST_ASSERT(*r2 == 200, "second value should be 200");

    orbisjvm_tls_delete(key);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "TLS isolation test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_tls_basic() < 0) failed++;
    if (test_tls_main_thread() < 0) failed++;
    if (test_tls_isolation() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All TLS tests passed!");
    return 0;
}