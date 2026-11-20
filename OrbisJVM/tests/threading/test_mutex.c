/**
 * @file test_mutex.c
 * @brief Mutex and synchronization tests
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

static orbisjvm_mutex_t g_mutex;
static atomic_int g_counter = 0;

void* mutex_increment(void* arg) {
    int iterations = *(int*)arg;

    for (int i = 0; i < iterations; i++) {
        orbisjvm_mutex_lock(g_mutex);
        atomic_fetch_add(&g_counter, 1);
        orbisjvm_mutex_unlock(g_mutex);
    }

    return NULL;
}

int test_mutex_basic(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing basic mutex...");

    atomic_store(&g_counter, 0);

    const int num_threads = 4;
    const int iterations = 10000;
    orbisjvm_thread_t threads[num_threads];

    for (int i = 0; i < num_threads; i++) {
        int ret = orbisjvm_thread_create(&threads[i], NULL, mutex_increment, &iterations);
        TEST_ASSERT(ret == 0, "thread_create should succeed");
    }

    for (int i = 0; i < num_threads; i++) {
        void* retval;
        int ret = orbisjvm_thread_join(threads[i], &retval);
        TEST_ASSERT(ret == 0, "thread_join should succeed");
    }

    TEST_ASSERT(atomic_load(&g_counter) == num_threads * iterations,
                "counter should match expected total (mutex protection worked)");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "basic mutex test passed");
    return 0;
}

int test_mutex_trylock(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing mutex trylock...");

    orbisjvm_mutex_lock(g_mutex);

    int ret = orbisjvm_mutex_trylock(g_mutex);
    TEST_ASSERT(ret == -EBUSY, "trylock should fail with EBUSY when locked");

    orbisjvm_mutex_unlock(g_mutex);

    ret = orbisjvm_mutex_trylock(g_mutex);
    TEST_ASSERT(ret == 0, "trylock should succeed when unlocked");

    orbisjvm_mutex_unlock(g_mutex);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "mutex trylock test passed");
    return 0;
}

int test_recursive_mutex(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing recursive mutex...");

    orbisjvm_mutex_attr_t attr;
    orbisjvm_mutex_attr_init(&attr);
    orbisjvm_mutex_attr_set_recursive(&attr, 1);

    orbisjvm_mutex_t rmutex;
    int ret = orbisjvm_mutex_create(&rmutex, &attr);
    TEST_ASSERT(ret == 0, "recursive mutex_create should succeed");

    orbisjvm_mutex_attr_destroy(&attr);

    /* Lock multiple times */
    for (int i = 0; i < 5; i++) {
        ret = orbisjvm_mutex_lock(rmutex);
        TEST_ASSERT(ret == 0, "recursive lock should succeed");
    }

    /* Unlock same number of times */
    for (int i = 0; i < 5; i++) {
        ret = orbisjvm_mutex_unlock(rmutex);
        TEST_ASSERT(ret == 0, "recursive unlock should succeed");
    }

    orbisjvm_mutex_destroy(rmutex);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "recursive mutex test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int ret = orbisjvm_mutex_create(&g_mutex, NULL);
    TEST_ASSERT(ret == 0, "global mutex creation should succeed");

    int failed = 0;

    if (test_mutex_basic() < 0) failed++;
    if (test_mutex_trylock() < 0) failed++;
    if (test_recursive_mutex() < 0) failed++;

    orbisjvm_mutex_destroy(g_mutex);
    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All mutex tests passed!");
    return 0;
}