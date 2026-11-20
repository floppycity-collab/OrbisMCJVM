/**
 * @file synchronization.c
 * @brief OrbisJVM Synchronization Primitives Implementation
 *
 * Implements semaphores, read-write locks, barriers, spinlocks, and atomic operations.
 */

#include <orbisjvm/synchronization.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>

#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

/* Semaphore structure */
struct orbisjvm_sem {
    sem_t sem;
    uint32_t max_value;
};

/* Read-write lock structure */
struct orbisjvm_rwlock {
    pthread_rwlock_t rwlock;
    int writer_priority;
};

/* Barrier structure */
struct orbisjvm_barrier {
    pthread_barrier_t barrier;
    uint32_t count;
};

/**
 * @brief Initialize synchronization subsystem
 */
int orbisjvm_synchronization_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing synchronization subsystem");
    return 0;
}

/**
 * @brief Shutdown synchronization subsystem
 */
void orbisjvm_synchronization_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down synchronization subsystem");
}

/* Semaphore functions */

int orbisjvm_sem_create(orbisjvm_sem_t* sem, uint32_t initial_value, uint32_t max_value) {
    if (!sem) return -1;

    orbisjvm_sem_t s = orbisjvm_malloc(sizeof(struct orbisjvm_sem), ORBISJVM_MEM_ZERO);
    if (!s) return -ENOMEM;

    s->max_value = max_value;

    int ret = sem_init(&s->sem, 0, initial_value);
    if (ret != 0) {
        orbisjvm_free(s);
        return -errno;
    }

    *sem = s;
    return 0;
}

int orbisjvm_sem_destroy(orbisjvm_sem_t sem) {
    if (!sem) return -1;

    int ret = sem_destroy(&sem->sem);
    if (ret != 0) {
        return -errno;
    }

    orbisjvm_free(sem);
    return 0;
}

int orbisjvm_sem_wait(orbisjvm_sem_t sem) {
    if (!sem) return -1;

    int ret = sem_wait(&sem->sem);
    return ret == 0 ? 0 : -errno;
}

int orbisjvm_sem_trywait(orbisjvm_sem_t sem) {
    if (!sem) return -1;

    int ret = sem_trywait(&sem->sem);
    if (ret == 0) return 0;
    if (errno == EAGAIN) return -EAGAIN;
    return -errno;
}

int orbisjvm_sem_timedwait(orbisjvm_sem_t sem, uint64_t timeout_ns) {
    if (!sem) return -1;

    struct timespec ts;
    ts.tv_sec = timeout_ns / 1000000000;
    ts.tv_nsec = timeout_ns % 1000000000;

    int ret = sem_timedwait(&sem->sem, &ts);
    if (ret == 0) return 0;
    if (errno == ETIMEDOUT) return -ETIMEDOUT;
    return -errno;
}

int orbisjvm_sem_post(orbisjvm_sem_t sem) {
    if (!sem) return -1;

    int ret = sem_post(&sem->sem);
    return ret == 0 ? 0 : -errno;
}

int orbisjvm_sem_getvalue(orbisjvm_sem_t sem, uint32_t* value) {
    if (!sem || !value) return -1;

    int val;
    int ret = sem_getvalue(&sem->sem, &val);
    if (ret != 0) return -errno;

    *value = (uint32_t)val;
    return 0;
}

/* Read-write lock functions */

void orbisjvm_rwlock_attr_init(orbisjvm_rwlock_attr_t* attr) {
    if (!attr) return;
    attr->writer_priority = 0;
}

void orbisjvm_rwlock_attr_destroy(orbisjvm_rwlock_attr_t* attr) {
    (void)attr;
}

int orbisjvm_rwlock_attr_set_writer_priority(orbisjvm_rwlock_attr_t* attr, int writer_priority) {
    if (!attr) return -1;
    attr->writer_priority = writer_priority;
    return 0;
}

int orbisjvm_rwlock_create(orbisjvm_rwlock_t* rwlock, const orbisjvm_rwlock_attr_t* attr) {
    if (!rwlock) return -1;

    orbisjvm_rwlock_t rw = orbisjvm_malloc(sizeof(struct orbisjvm_rwlock), ORBISJVM_MEM_ZERO);
    if (!rw) return -ENOMEM;

    pthread_rwlockattr_t pthread_attr;
    pthread_rwlockattr_init(&pthread_attr);

    if (attr && attr->writer_priority) {
        pthread_rwlockattr_setkind_np(&pthread_attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    } else {
        pthread_rwlockattr_setkind_np(&pthread_attr, PTHREAD_RWLOCK_PREFER_READER_NP);
    }
    rw->writer_priority = attr ? attr->writer_priority : 0;

    int ret = pthread_rwlock_init(&rw->rwlock, &pthread_attr);
    pthread_rwlockattr_destroy(&pthread_attr);

    if (ret != 0) {
        orbisjvm_free(rw);
        return -ret;
    }

    *rwlock = rw;
    return 0;
}

int orbisjvm_rwlock_destroy(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;

    int ret = pthread_rwlock_destroy(&rwlock->rwlock);
    if (ret != 0) {
        return -ret;
    }

    orbisjvm_free(rwlock);
    return 0;
}

int orbisjvm_rwlock_rdlock(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;
    int ret = pthread_rwlock_rdlock(&rwlock->rwlock);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_rwlock_tryrdlock(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;
    int ret = pthread_rwlock_tryrdlock(&rwlock->rwlock);
    if (ret == EBUSY) return -EBUSY;
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_rwlock_wrlock(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;
    int ret = pthread_rwlock_wrlock(&rwlock->rwlock);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_rwlock_trywrlock(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;
    int ret = pthread_rwlock_trywrlock(&rwlock->rwlock);
    if (ret == EBUSY) return -EBUSY;
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_rwlock_unlock(orbisjvm_rwlock_t rwlock) {
    if (!rwlock) return -1;
    int ret = pthread_rwlock_unlock(&rwlock->rwlock);
    return ret == 0 ? 0 : -ret;
}

/* Barrier functions */

int orbisjvm_barrier_create(orbisjvm_barrier_t* barrier, uint32_t count) {
    if (!barrier || count == 0) return -1;

    orbisjvm_barrier_t b = orbisjvm_malloc(sizeof(struct orbisjvm_barrier), ORBISJVM_MEM_ZERO);
    if (!b) return -ENOMEM;

    b->count = count;

    int ret = pthread_barrier_init(&b->barrier, NULL, count);
    if (ret != 0) {
        orbisjvm_free(b);
        return -ret;
    }

    *barrier = b;
    return 0;
}

int orbisjvm_barrier_destroy(orbisjvm_barrier_t barrier) {
    if (!barrier) return -1;

    int ret = pthread_barrier_destroy(&barrier->barrier);
    if (ret != 0) {
        return -ret;
    }

    orbisjvm_free(barrier);
    return 0;
}

int orbisjvm_barrier_wait(orbisjvm_barrier_t barrier) {
    if (!barrier) return -1;

    int ret = pthread_barrier_wait(&barrier->barrier);
    if (ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD) {
        return ret == PTHREAD_BARRIER_SERIAL_THREAD ? 1 : 0;
    }
    return -ret;
}

/* Spinlock functions */

void orbisjvm_spinlock_lock(orbisjvm_spinlock_t* lock) {
    if (!lock) return;

    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        /* Spin */
        __builtin_ia32_pause();
    }
}

int orbisjvm_spinlock_trylock(orbisjvm_spinlock_t* lock) {
    if (!lock) return 0;
    return !__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE);
}

void orbisjvm_spinlock_unlock(orbisjvm_spinlock_t* lock) {
    if (!lock) return;
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
}

/* Statistics functions */

int orbisjvm_rwlock_get_stats(orbisjvm_rwlock_t rwlock, orbisjvm_rwlock_stats_t* stats) {
    (void)rwlock;
    (void)stats;
    /* Not implemented - would need custom wrapper */
    return -1;
}

int orbisjvm_sem_get_stats(orbisjvm_sem_t sem, orbisjvm_sem_stats_t* stats) {
    (void)sem;
    (void)stats;
    /* Not implemented - would need custom wrapper */
    return -1;
}