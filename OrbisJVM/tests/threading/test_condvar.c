/**
 * @file test_condvar.c
 * @brief Condition variable tests
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
static orbisjvm_cond_t g_cond;
static atomic_int g_ready = 0;
static atomic_int g_signaled = 0;

void* condvar_waiter(void* arg) {
    (void)arg;

    orbisjvm_mutex_lock(g_mutex);

    while (!atomic_load(&g_ready)) {
        int ret = orbisjvm_cond_wait(g_cond, g_mutex);
        TEST_ASSERT(ret == 0, "cond_wait should succeed");
    }

    atomic_fetch_add(&g_signaled, 1);
    orbisjvm_mutex_unlock(g_mutex);

    return NULL;
}

int test_condvar_signal(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing condition variable signal...");

    atomic_store(&g_ready, 0);
    atomic_store(&g_signaled, 0);

    orbisjvm_thread_t threads[3];
    for (int i = 0; i < 3; i++) {
        int ret = orbisjvm_thread_create(&threads[i], NULL, condvar_waiter, NULL);
        TEST_ASSERT(ret == 0, "thread_create should succeed");
    }

    /* Give threads time to start waiting */
    orbisjvm_thread_sleep_ms(10);

    orbisjvm_mutex_lock(g_mutex);
    atomic_store(&g_ready, 1);
    orbisjvm_cond_signal(g_cond);
    orbisjvm_mutex_unlock(g_mutex);

    /* Wait for all threads */
    for (int i = 0; i < 3; i++) {
        void* retval;
        int ret = orbisjvm_thread_join(threads[i], &retval);
        TEST_ASSERT(ret == 0, "thread_join should succeed");
    }

    TEST_ASSERT(atomic_load(&g_signaled) == 1, "only one thread should be signaled");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "condvar signal test passed");
    return 0;
}

int test_condvar_broadcast(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing condition variable broadcast...");

    atomic_store(&g_ready, 0);
    atomic_store(&g_signaled, 0);

    orbisjvm_thread_t threads[3];
    for (int i = 0; i < 3; i++) {
        int ret = orbisjvm_thread_create(&threads[i], NULL, condvar_waiter, NULL);
        TEST_ASSERT(ret == 0, "thread_create should succeed");
    }

    orbisjvm_thread_sleep_ms(10);

    orbisjvm_mutex_lock(g_mutex);
    atomic_store(&g_ready, 1);
    orbisjvm_cond_broadcast(g_cond);
    orbisjvm_mutex_unlock(g_mutex);

    for (int i = 0; i < 3; i++) {
        void* retval;
        int ret = orbisjvm_thread_join(threads[i], &retval);
        TEST_ASSERT(ret == 0, "thread_join should succeed");
    }

    TEST_ASSERT(atomic_load(&g_signaled) == 3, "all three threads should be signaled");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "condvar broadcast test passed");
    return 0;
}

int test_condvar_timedwait(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing condition variable timedwait...");

    atomic_store(&g_ready, 0);

    orbisjvm_mutex_lock(g_mutex);

    uint64_t timeout = orbisjvm_get_monotonic_ns() + 100000000ULL; /* 100ms */
    int ret = orbisjvm_cond_timedwait(g_cond, g_mutex, timeout);

    orbisjvm_mutex_unlock(g_mutex);

    TEST_ASSERT(ret == -ETIMEDOUT, "timedwait should timeout");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "condvar timedwait test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int ret = orbisjvm_mutex_create(&g_mutex, NULL);
    TEST_ASSERT(ret == 0, "mutex creation should succeed");

    ret = orbisjvm_cond_create(&g_cond, NULL);
    TEST_ASSERT(ret == 0, "condvar creation should succeed");

    int failed = 0;

    if (test_condvar_signal() < 0) failed++;
    if (test_condvar_broadcast() < 0) failed++;
    if (test_condvar_timedwait() < 0) failed++;

    orbisjvm_cond_destroy(g_cond);
    orbisjvm_mutex_destroy(g_mutex);
    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All condition variable tests passed!");
    return 0;
}