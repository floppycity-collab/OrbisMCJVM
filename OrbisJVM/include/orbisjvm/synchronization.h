/**
 * @file synchronization.h
 * @brief OrbisJVM Synchronization Primitives
 *
 * Provides additional synchronization primitives beyond basic mutex/condvar:
 * semaphores, read-write locks, barriers, spinlocks, atomic operations.
 */

#ifndef ORBISJVM_SYNCHRONIZATION_H
#define ORBISJVM_SYNCHRONIZATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Semaphore handle
 */
typedef struct orbisjvm_sem* orbisjvm_sem_t;

/**
 * @brief Create a semaphore
 *
 * @param[out] sem Output semaphore handle
 * @param initial_value Initial semaphore value
 * @param max_value Maximum semaphore value
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_create(orbisjvm_sem_t* sem, uint32_t initial_value, uint32_t max_value);

/**
 * @brief Destroy a semaphore
 *
 * @param sem Semaphore to destroy
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_destroy(orbisjvm_sem_t sem);

/**
 * @brief Wait on semaphore (decrement)
 *
 * @param sem Semaphore
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_wait(orbisjvm_sem_t sem);

/**
 * @brief Try wait on semaphore (non-blocking)
 *
 * @param sem Semaphore
 * @return 0 on success, EAGAIN if would block, negative error code on failure
 */
int orbisjvm_sem_trywait(orbisjvm_sem_t sem);

/**
 * @brief Timed wait on semaphore
 *
 * @param sem Semaphore
 * @param timeout_ns Timeout in nanoseconds (absolute, monotonic clock)
 * @return 0 on success, ETIMEDOUT on timeout, negative error code on failure
 */
int orbisjvm_sem_timedwait(orbisjvm_sem_t sem, uint64_t timeout_ns);

/**
 * @brief Post to semaphore (increment)
 *
 * @param sem Semaphore
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_post(orbisjvm_sem_t sem);

/**
 * @brief Get semaphore value
 *
 * @param sem Semaphore
 * @param[out] value Current value
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_getvalue(orbisjvm_sem_t sem, uint32_t* value);

/**
 * @brief Read-write lock handle
 */
typedef struct orbisjvm_rwlock* orbisjvm_rwlock_t;

/**
 * @brief Read-write lock attributes
 */
typedef struct {
    int writer_priority;  ///< 1 = writers preferred, 0 = readers preferred
} orbisjvm_rwlock_attr_t;

/**
 * @brief Initialize RW lock attributes
 *
 * @param[out] attr Attributes to initialize
 */
void orbisjvm_rwlock_attr_init(orbisjvm_rwlock_attr_t* attr);

/**
 * @brief Destroy RW lock attributes
 *
 * @param attr Attributes to destroy
 */
void orbisjvm_rwlock_attr_destroy(orbisjvm_rwlock_attr_t* attr);

/**
 * @brief Set writer priority
 *
 * @param attr Attributes
 * @param writer_priority 1 for writer priority, 0 for reader priority
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_attr_set_writer_priority(orbisjvm_rwlock_attr_t* attr, int writer_priority);

/**
 * @brief Create a read-write lock
 *
 * @param[out] rwlock Output RW lock handle
 * @param attr Attributes (or NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_create(orbisjvm_rwlock_t* rwlock, const orbisjvm_rwlock_attr_t* attr);

/**
 * @brief Destroy a read-write lock
 *
 * @param rwlock RW lock to destroy
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_destroy(orbisjvm_rwlock_t rwlock);

/**
 * @brief Acquire read lock
 *
 * @param rwlock RW lock
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_rdlock(orbisjvm_rwlock_t rwlock);

/**
 * @brief Try to acquire read lock
 *
 * @param rwlock RW lock
 * @return 0 on success, EBUSY if locked, negative error code on failure
 */
int orbisjvm_rwlock_tryrdlock(orbisjvm_rwlock_t rwlock);

/**
 * @brief Acquire write lock
 *
 * @param rwlock RW lock
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_wrlock(orbisjvm_rwlock_t rwlock);

/**
 * @brief Try to acquire write lock
 *
 * @param rwlock RW lock
 * @return 0 on success, EBUSY if locked, negative error code on failure
 */
int orbisjvm_rwlock_trywrlock(orbisjvm_rwlock_t rwlock);

/**
 * @brief Release read or write lock
 *
 * @param rwlock RW lock
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_unlock(orbisjvm_rwlock_t rwlock);

/**
 * @brief Barrier handle
 */
typedef struct orbisjvm_barrier* orbisjvm_barrier_t;

/**
 * @brief Create a barrier
 *
 * @param[out] barrier Output barrier handle
 * @param count Number of threads to wait for
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_barrier_create(orbisjvm_barrier_t* barrier, uint32_t count);

/**
 * @brief Destroy a barrier
 *
 * @param barrier Barrier to destroy
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_barrier_destroy(orbisjvm_barrier_t barrier);

/**
 * @brief Wait at barrier
 *
 * @param barrier Barrier
 * @return 0 on success, negative error code on failure, 1 for the thread that releases the barrier
 */
int orbisjvm_barrier_wait(orbisjvm_barrier_t barrier);

/**
 * @brief Spinlock handle
 */
typedef struct {
    volatile uint32_t lock;
} orbisjvm_spinlock_t;

#define ORBISJVM_SPINLOCK_INIT {0}

/**
 * @brief Initialize a spinlock
 *
 * @param lock Spinlock to initialize
 */
static inline void orbisjvm_spinlock_init(orbisjvm_spinlock_t* lock) {
    lock->lock = 0;
}

/**
 * @brief Acquire spinlock
 *
 * @param lock Spinlock
 */
void orbisjvm_spinlock_lock(orbisjvm_spinlock_t* lock);

/**
 * @brief Try to acquire spinlock
 *
 * @param lock Spinlock
 * @return 1 if acquired, 0 if not
 */
int orbisjvm_spinlock_trylock(orbisjvm_spinlock_t* lock);

/**
 * @brief Release spinlock
 *
 * @param lock Spinlock
 */
void orbisjvm_spinlock_unlock(orbisjvm_spinlock_t* lock);

/**
 * @brief Atomic operations
 */

/* Atomic 32-bit integer */
typedef volatile uint32_t orbisjvm_atomic32_t;

/* Atomic 64-bit integer */
typedef volatile uint64_t orbisjvm_atomic64_t;

/* Atomic pointer */
typedef volatile void* orbisjvm_atomic_ptr_t;

/**
 * @brief Atomic load (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_load(const orbisjvm_atomic32_t* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic store (32-bit)
 */
static inline void orbisjvm_atomic32_store(orbisjvm_atomic32_t* ptr, uint32_t val) {
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic exchange (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_exchange(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic compare-and-swap (32-bit)
 */
static inline int orbisjvm_atomic32_cas(orbisjvm_atomic32_t* ptr, uint32_t* expected, uint32_t desired) {
    return __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-add (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_fetch_add(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-sub (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_fetch_sub(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-or (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_fetch_or(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-and (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_fetch_and(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-xor (32-bit)
 */
static inline uint32_t orbisjvm_atomic32_fetch_xor(orbisjvm_atomic32_t* ptr, uint32_t val) {
    return __atomic_fetch_xor(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic load (64-bit)
 */
static inline uint64_t orbisjvm_atomic64_load(const orbisjvm_atomic64_t* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic store (64-bit)
 */
static inline void orbisjvm_atomic64_store(orbisjvm_atomic64_t* ptr, uint64_t val) {
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic exchange (64-bit)
 */
static inline uint64_t orbisjvm_atomic64_exchange(orbisjvm_atomic64_t* ptr, uint64_t val) {
    return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic compare-and-swap (64-bit)
 */
static inline int orbisjvm_atomic64_cas(orbisjvm_atomic64_t* ptr, uint64_t* expected, uint64_t desired) {
    return __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic fetch-and-add (64-bit)
 */
static inline uint64_t orbisjvm_atomic64_fetch_add(orbisjvm_atomic64_t* ptr, uint64_t val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic pointer load
 */
static inline void* orbisjvm_atomic_ptr_load(const orbisjvm_atomic_ptr_t* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic pointer store
 */
static inline void orbisjvm_atomic_ptr_store(orbisjvm_atomic_ptr_t* ptr, void* val) {
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic pointer exchange
 */
static inline void* orbisjvm_atomic_ptr_exchange(orbisjvm_atomic_ptr_t* ptr, void* val) {
    return __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomic pointer compare-and-swap
 */
static inline int orbisjvm_atomic_ptr_cas(orbisjvm_atomic_ptr_t* ptr, void** expected, void* desired) {
    return __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/**
 * @brief Memory barrier (full fence)
 */
static inline void orbisjvm_memory_barrier(void) {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

/**
 * @brief Acquire memory barrier
 */
static inline void orbisjvm_memory_barrier_acquire(void) {
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

/**
 * @brief Release memory barrier
 */
static inline void orbisjvm_memory_barrier_release(void) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

/**
 * @brief Read-write lock statistics (for diagnostics)
 */
typedef struct {
    uint64_t rdlock_count;
    uint64_t wrlock_count;
    uint64_t unlock_count;
    uint64_t contention_count;
} orbisjvm_rwlock_stats_t;

/**
 * @brief Get RW lock statistics
 *
 * @param rwlock RW lock
 * @param[out] stats Statistics output
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rwlock_get_stats(orbisjvm_rwlock_t rwlock, orbisjvm_rwlock_stats_t* stats);

/**
 * @brief Semaphore statistics (for diagnostics)
 */
typedef struct {
    uint64_t wait_count;
    uint64_t post_count;
    uint64_t contention_count;
} orbisjvm_sem_stats_t;

/**
 * @brief Get semaphore statistics
 *
 * @param sem Semaphore
 * @param[out] stats Statistics output
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_sem_get_stats(orbisjvm_sem_t sem, orbisjvm_sem_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_SYNCHRONIZATION_H