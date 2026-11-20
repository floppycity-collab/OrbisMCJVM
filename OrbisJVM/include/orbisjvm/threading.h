/**
 * @file threading.h
 * @brief OrbisJVM Threading API
 *
 * Provides thread management, synchronization primitives, and thread-local storage
 * compatible with Orbis OS using the OpenOrbis toolchain.
 */

#ifndef ORBISJVM_THREADING_H
#define ORBISJVM_THREADING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thread handle
 */
typedef struct orbisjvm_thread* orbisjvm_thread_t;

/**
 * @brief Thread attributes
 */
typedef struct {
    size_t stack_size;        ///< Stack size in bytes (0 = default)
    int priority;             ///< Thread priority (-20 to 19, 0 = normal)
    int detached;             ///< 1 = detached, 0 = joinable
    const char* name;         ///< Thread name for debugging
    void* tls_template;       ///< Initial TLS template (optional)
    size_t tls_size;          ///< TLS size in bytes
} orbisjvm_thread_attr_t;

/**
 * @brief Thread entry function
 *
 * @param arg User argument
 * @return Thread exit value
 */
typedef void* (*orbisjvm_thread_func_t)(void* arg);

/**
 * @brief Initialize thread attributes with defaults
 *
 * @param[out] attr Thread attributes to initialize
 */
void orbisjvm_thread_attr_init(orbisjvm_thread_attr_t* attr);

/**
 * @brief Destroy thread attributes
 *
 * @param attr Thread attributes to destroy
 */
void orbisjvm_thread_attr_destroy(orbisjvm_thread_attr_t* attr);

/**
 * @brief Set stack size in thread attributes
 *
 * @param attr Thread attributes
 * @param stack_size Stack size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_set_stack_size(orbisjvm_thread_attr_t* attr, size_t stack_size);

/**
 * @brief Get stack size from thread attributes
 *
 * @param attr Thread attributes
 * @param[out] stack_size Output stack size
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_get_stack_size(const orbisjvm_thread_attr_t* attr, size_t* stack_size);

/**
 * @brief Set thread priority in thread attributes
 *
 * @param attr Thread attributes
 * @param priority Priority value (-20 to 19)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_set_priority(orbisjvm_thread_attr_t* attr, int priority);

/**
 * @brief Get thread priority from thread attributes
 *
 * @param attr Thread attributes
 * @param[out] priority Output priority
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_get_priority(const orbisjvm_thread_attr_t* attr, int* priority);

/**
 * @brief Set detached state in thread attributes
 *
 * @param attr Thread attributes
 * @param detached 1 for detached, 0 for joinable
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_set_detached(orbisjvm_thread_attr_t* attr, int detached);

/**
 * @brief Get detached state from thread attributes
 *
 * @param attr Thread attributes
 * @param[out] detached Output detached state
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_get_detached(const orbisjvm_thread_attr_t* attr, int* detached);

/**
 * @brief Set thread name in thread attributes
 *
 * @param attr Thread attributes
 * @param name Thread name (max 31 characters)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_attr_set_name(orbisjvm_thread_attr_t* attr, const char* name);

/**
 * @brief Create a new thread
 *
 * @param[out] thread Output thread handle
 * @param attr Thread attributes (or NULL for defaults)
 * @param func Thread entry function
 * @param arg Argument to pass to thread function
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_create(orbisjvm_thread_t* thread,
                           const orbisjvm_thread_attr_t* attr,
                           orbisjvm_thread_func_t func,
                           void* arg);

/**
 * @brief Get current thread handle
 *
 * @return Current thread handle
 */
orbisjvm_thread_t orbisjvm_thread_self(void);

/**
 * @brief Wait for thread to terminate
 *
 * @param thread Thread to join
 * @param[out] retval Output thread return value (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_join(orbisjvm_thread_t thread, void** retval);

/**
 * @brief Detach a thread
 *
 * @param thread Thread to detach
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_detach(orbisjvm_thread_t thread);

/**
 * @brief Exit current thread
 *
 * @param retval Thread return value
 */
void orbisjvm_thread_exit(void* retval) __attribute__((noreturn));

/**
 * @brief Compare two thread handles for equality
 *
 * @param t1 First thread
 * @param t2 Second thread
 * @return 1 if equal, 0 if not equal
 */
int orbisjvm_thread_equal(orbisjvm_thread_t t1, orbisjvm_thread_t t2);

/**
 * @brief Get thread ID (for debugging)
 *
 * @param thread Thread handle
 * @return Thread ID (platform-specific)
 */
uint64_t orbisjvm_thread_get_id(orbisjvm_thread_t thread);

/**
 * @brief Get thread name
 *
 * @param thread Thread handle
 * @param[out] buffer Buffer to receive name
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_get_name(orbisjvm_thread_t thread, char* buffer, size_t buflen);

/**
 * @brief Yield current thread's timeslice
 */
void orbisjvm_thread_yield(void);

/**
 * @brief Sleep current thread for specified microseconds
 *
 * @param usec Microseconds to sleep
 */
void orbisjvm_thread_sleep_us(uint64_t usec);

/**
 * @brief Sleep current thread for specified milliseconds
 *
 * @param msec Milliseconds to sleep
 */
void orbisjvm_thread_sleep_ms(uint64_t msec);

/**
 * @brief Mutex handle
 */
typedef struct orbisjvm_mutex* orbisjvm_mutex_t;

/**
 * @brief Mutex attributes
 */
typedef struct {
    int recursive;   ///< 1 = recursive mutex, 0 = normal
    int priority_ceiling;  ///< Priority ceiling for priority inheritance
} orbisjvm_mutex_attr_t;

/**
 * @brief Initialize mutex attributes with defaults
 *
 * @param[out] attr Mutex attributes to initialize
 */
void orbisjvm_mutex_attr_init(orbisjvm_mutex_attr_t* attr);

/**
 * @brief Destroy mutex attributes
 *
 * @param attr Mutex attributes to destroy
 */
void orbisjvm_mutex_attr_destroy(orbisjvm_mutex_attr_t* attr);

/**
 * @brief Set recursive attribute
 *
 * @param attr Mutex attributes
 * @param recursive 1 for recursive, 0 for normal
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mutex_attr_set_recursive(orbisjvm_mutex_attr_t* attr, int recursive);

/**
 * @brief Create a mutex
 *
 * @param[out] mutex Output mutex handle
 * @param attr Mutex attributes (or NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mutex_create(orbisjvm_mutex_t* mutex, const orbisjvm_mutex_attr_t* attr);

/**
 * @brief Destroy a mutex
 *
 * @param mutex Mutex to destroy
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mutex_destroy(orbisjvm_mutex_t mutex);

/**
 * @brief Lock a mutex
 *
 * @param mutex Mutex to lock
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mutex_lock(orbisjvm_mutex_t mutex);

/**
 * @brief Try to lock a mutex (non-blocking)
 *
 * @param mutex Mutex to lock
 * @return 0 on success, EBUSY if locked, negative error code on failure
 */
int orbisjvm_mutex_trylock(orbisjvm_mutex_t mutex);

/**
 * @brief Unlock a mutex
 *
 * @param mutex Mutex to unlock
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mutex_unlock(orbisjvm_mutex_t mutex);

/**
 * @brief Condition variable handle
 */
typedef struct orbisjvm_cond* orbisjvm_cond_t;

/**
 * @brief Condition variable attributes
 */
typedef struct {
    int clock_type;  ///< Clock type for timed waits (0 = monotonic, 1 = realtime)
} orbisjvm_cond_attr_t;

/**
 * @brief Initialize condition variable attributes
 *
 * @param[out] attr Condition variable attributes to initialize
 */
void orbisjvm_cond_attr_init(orbisjvm_cond_attr_t* attr);

/**
 * @brief Destroy condition variable attributes
 *
 * @param attr Condition variable attributes to destroy
 */
void orbisjvm_cond_attr_destroy(orbisjvm_cond_attr_t* attr);

/**
 * @brief Create a condition variable
 *
 * @param[out] cond Output condition variable handle
 * @param attr Condition variable attributes (or NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cond_create(orbisjvm_cond_t* cond, const orbisjvm_cond_attr_t* attr);

/**
 * @brief Destroy a condition variable
 *
 * @param cond Condition variable to destroy
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cond_destroy(orbisjvm_cond_t cond);

/**
 * @brief Wait on a condition variable
 *
 * @param cond Condition variable
 * @param mutex Associated mutex (must be locked by caller)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cond_wait(orbisjvm_cond_t cond, orbisjvm_mutex_t mutex);

/**
 * @brief Timed wait on a condition variable
 *
 * @param cond Condition variable
 * @param mutex Associated mutex (must be locked by caller)
 * @param abstime Absolute timeout time (monotonic clock, nanoseconds)
 * @return 0 on success, ETIMEDOUT on timeout, negative error code on failure
 */
int orbisjvm_cond_timedwait(orbisjvm_cond_t cond, orbisjvm_mutex_t mutex, uint64_t abstime);

/**
 * @brief Signal one waiting thread
 *
 * @param cond Condition variable
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cond_signal(orbisjvm_cond_t cond);

/**
 * @brief Broadcast to all waiting threads
 *
 * @param cond Condition variable
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cond_broadcast(orbisjvm_cond_t cond);

/**
 * @brief Thread-local storage key
 */
typedef uint32_t orbisjvm_tls_key_t;

/**
 * @brief Create a thread-local storage key
 *
 * @param[out] key Output TLS key
 * @param destructor Destructor function called on thread exit (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_tls_create(orbisjvm_tls_key_t* key, void (*destructor)(void*));

/**
 * @brief Delete a thread-local storage key
 *
 * @param key TLS key to delete
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_tls_delete(orbisjvm_tls_key_t key);

/**
 * @brief Set thread-local value
 *
 * @param key TLS key
 * @param value Value to set
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_tls_set(orbisjvm_tls_key_t key, void* value);

/**
 * @brief Get thread-local value
 *
 * @param key TLS key
 * @return Thread-local value, or NULL if not set
 */
void* orbisjvm_tls_get(orbisjvm_tls_key_t key);

/**
 * @brief Once control for one-time initialization
 */
typedef struct {
    int done;
    orbisjvm_mutex_t mutex;
    orbisjvm_cond_t cond;
} orbisjvm_once_t;

#define ORBISJVM_ONCE_INIT {0, NULL, NULL}

/**
 * @brief Execute initialization function once
 *
 * @param once Once control structure
 * @param init Initialization function
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_once(orbisjvm_once_t* once, void (*init)(void));

/**
 * @brief Thread priority values
 */
typedef enum {
    ORBISJVM_THREAD_PRIO_LOWEST    = -20,
    ORBISJVM_THREAD_PRIO_LOW       = -10,
    ORBISJVM_THREAD_PRIO_NORMAL    = 0,
    ORBISJVM_THREAD_PRIO_HIGH      = 10,
    ORBISJVM_THREAD_PRIO_HIGHEST   = 19,
    ORBISJVM_THREAD_PRIO_CRITICAL  = 20,
    ORBISJVM_THREAD_PRIO_REALTIME  = 30,
} orbisjvm_thread_priority_t;

/**
 * @brief Set current thread priority
 *
 * @param priority New priority
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_set_priority(int priority);

/**
 * @brief Get current thread priority
 *
 * @return Current priority
 */
int orbisjvm_thread_get_priority(void);

/**
 * @brief Get thread statistics (for diagnostics)
 */
typedef struct {
    uint64_t threads_created;
    uint64_t threads_destroyed;
    uint64_t mutexes_created;
    uint64_t mutexes_destroyed;
    uint64_t condvars_created;
    uint64_t condvars_destroyed;
    uint32_t current_thread_count;
    uint32_t peak_thread_count;
} orbisjvm_thread_stats_t;

/**
 * @brief Get thread statistics
 *
 * @param[out] stats Output statistics
 */
void orbisjvm_get_thread_stats(orbisjvm_thread_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_THREADING_H