/**
 * @file threading.c
 * @brief OrbisJVM Threading Implementation
 *
 * Implements threading using Orbis OS pthread APIs.
 */

#include <orbisjvm/threading.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/time.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Thread structure */
struct orbisjvm_thread {
    pthread_t pthread;
    orbisjvm_thread_func_t func;
    void* arg;
    void* retval;
    int detached;
    char name[32];
    uint64_t id;
};

/* Mutex structure */
struct orbisjvm_mutex {
    pthread_mutex_t mutex;
    int recursive;
};

/* Condition variable structure */
struct orbisjvm_cond {
    pthread_cond_t cond;
};

/* Statistics */
static uint64_t g_threads_created = 0;
static uint64_t g_threads_destroyed = 0;
static uint64_t g_mutexes_created = 0;
static uint64_t g_mutexes_destroyed = 0;
static uint64_t g_condvars_created = 0;
static uint64_t g_condvars_destroyed = 0;
static uint32_t g_current_thread_count = 0;
static uint32_t g_peak_thread_count = 0;
static orbisjvm_mutex_t g_stats_mutex;

/* Thread entry wrapper */
static void* thread_entry_wrapper(void* arg) {
    orbisjvm_thread_t thread = (orbisjvm_thread_t)arg;

    /* Set thread name if supported */
    if (thread->name[0]) {
        pthread_setname_np(pthread_self(), thread->name);
    }

    /* Execute user function */
    thread->retval = thread->func(thread->arg);

    /* Update stats */
    orbisjvm_mutex_lock(g_stats_mutex);
    g_current_thread_count--;
    g_threads_destroyed++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    /* If detached, clean up */
    if (thread->detached) {
        orbisjvm_free(thread);
    }

    return thread->retval;
}

/**
 * @brief Initialize threading subsystem
 */
int orbisjvm_threading_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_THREADING, "Initializing threading subsystem");

    int ret = orbisjvm_mutex_create(&g_stats_mutex, NULL);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_THREADING, "Failed to create stats mutex");
        return ret;
    }

    return 0;
}

/**
 * @brief Shutdown threading subsystem
 */
void orbisjvm_threading_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_THREADING, "Shutting down threading subsystem");
    orbisjvm_mutex_destroy(g_stats_mutex);
}

/* Thread attribute functions */

void orbisjvm_thread_attr_init(orbisjvm_thread_attr_t* attr) {
    if (!attr) return;
    attr->stack_size = 0;
    attr->priority = 0;
    attr->detached = 0;
    attr->name = NULL;
    attr->tls_template = NULL;
    attr->tls_size = 0;
}

void orbisjvm_thread_attr_destroy(orbisjvm_thread_attr_t* attr) {
    (void)attr;
}

int orbisjvm_thread_attr_set_stack_size(orbisjvm_thread_attr_t* attr, size_t stack_size) {
    if (!attr) return -1;
    attr->stack_size = stack_size;
    return 0;
}

int orbisjvm_thread_attr_get_stack_size(const orbisjvm_thread_attr_t* attr, size_t* stack_size) {
    if (!attr || !stack_size) return -1;
    *stack_size = attr->stack_size;
    return 0;
}

int orbisjvm_thread_attr_set_priority(orbisjvm_thread_attr_t* attr, int priority) {
    if (!attr) return -1;
    attr->priority = priority;
    return 0;
}

int orbisjvm_thread_attr_get_priority(const orbisjvm_thread_attr_t* attr, int* priority) {
    if (!attr || !priority) return -1;
    *priority = attr->priority;
    return 0;
}

int orbisjvm_thread_attr_set_detached(orbisjvm_thread_attr_t* attr, int detached) {
    if (!attr) return -1;
    attr->detached = detached;
    return 0;
}

int orbisjvm_thread_attr_get_detached(const orbisjvm_thread_attr_t* attr, int* detached) {
    if (!attr || !detached) return -1;
    *detached = attr->detached;
    return 0;
}

int orbisjvm_thread_attr_set_name(orbisjvm_thread_attr_t* attr, const char* name) {
    if (!attr) return -1;
    attr->name = name;
    return 0;
}

/* Thread functions */

int orbisjvm_thread_create(orbisjvm_thread_t* thread,
                           const orbisjvm_thread_attr_t* attr,
                           orbisjvm_thread_func_t func,
                           void* arg) {
    if (!thread || !func) return -1;

    orbisjvm_thread_t t = orbisjvm_malloc(sizeof(struct orbisjvm_thread), ORBISJVM_MEM_ZERO);
    if (!t) return -1;

    t->func = func;
    t->arg = arg;
    t->retval = NULL;
    t->detached = 0;

    if (attr) {
        t->detached = attr->detached;
        if (attr->name) {
            strncpy(t->name, attr->name, sizeof(t->name) - 1);
        }
    }

    pthread_attr_t pthread_attr;
    pthread_attr_init(&pthread_attr);

    if (attr && attr->stack_size > 0) {
        pthread_attr_setstacksize(&pthread_attr, attr->stack_size);
    }

    if (attr && attr->detached) {
        pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
    }

    int ret = pthread_create(&t->pthread, &pthread_attr, thread_entry_wrapper, t);
    pthread_attr_destroy(&pthread_attr);

    if (ret != 0) {
        orbisjvm_free(t);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_THREADING, "pthread_create failed: %d", ret);
        return -ret;
    }

    /* Update stats */
    orbisjvm_mutex_lock(g_stats_mutex);
    g_threads_created++;
    g_current_thread_count++;
    if (g_current_thread_count > g_peak_thread_count) {
        g_peak_thread_count = g_current_thread_count;
    }
    orbisjvm_mutex_unlock(g_stats_mutex);

    *thread = t;
    orbisjvm_log_trace(ORBISJVM_LOG_CAT_THREADING, "Created thread %p (pthread=%lu)", t, (unsigned long)t->pthread);
    return 0;
}

orbisjvm_thread_t orbisjvm_thread_self(void) {
    /* Return a pseudo-handle for current thread */
    /* In a real implementation, we'd use TLS to store the thread handle */
    static __thread orbisjvm_thread_t self_thread = NULL;
    if (!self_thread) {
        self_thread = orbisjvm_malloc(sizeof(struct orbisjvm_thread), ORBISJVM_MEM_ZERO);
        self_thread->pthread = pthread_self();
        self_thread->id = (uint64_t)(uintptr_t)pthread_self();
    }
    return self_thread;
}

int orbisjvm_thread_join(orbisjvm_thread_t thread, void** retval) {
    if (!thread) return -1;

    int ret = pthread_join(thread->pthread, retval ? &thread->retval : NULL);
    if (ret != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_THREADING, "pthread_join failed: %d", ret);
        return -ret;
    }

    if (retval) *retval = thread->retval;

    orbisjvm_free(thread);
    return 0;
}

int orbisjvm_thread_detach(orbisjvm_thread_t thread) {
    if (!thread) return -1;

    int ret = pthread_detach(thread->pthread);
    if (ret != 0) {
        return -ret;
    }

    thread->detached = 1;
    return 0;
}

void orbisjvm_thread_exit(void* retval) {
    pthread_exit(retval);
}

int orbisjvm_thread_equal(orbisjvm_thread_t t1, orbisjvm_thread_t t2) {
    if (!t1 || !t2) return 0;
    return pthread_equal(t1->pthread, t2->pthread);
}

uint64_t orbisjvm_thread_get_id(orbisjvm_thread_t thread) {
    if (!thread) return 0;
    return (uint64_t)(uintptr_t)thread->pthread;
}

int orbisjvm_thread_get_name(orbisjvm_thread_t thread, char* buffer, size_t buflen) {
    if (!thread || !buffer || buflen == 0) return -1;

    strncpy(buffer, thread->name, buflen - 1);
    buffer[buflen - 1] = '\0';
    return 0;
}

void orbisjvm_thread_yield(void) {
    sched_yield();
}

void orbisjvm_thread_sleep_us(uint64_t usec) {
    struct timespec ts;
    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

void orbisjvm_thread_sleep_ms(uint64_t msec) {
    orbisjvm_thread_sleep_us(msec * 1000);
}

/* Mutex functions */

void orbisjvm_mutex_attr_init(orbisjvm_mutex_attr_t* attr) {
    if (!attr) return;
    attr->recursive = 0;
    attr->priority_ceiling = 0;
}

void orbisjvm_mutex_attr_destroy(orbisjvm_mutex_attr_t* attr) {
    (void)attr;
}

int orbisjvm_mutex_attr_set_recursive(orbisjvm_mutex_attr_t* attr, int recursive) {
    if (!attr) return -1;
    attr->recursive = recursive;
    return 0;
}

int orbisjvm_mutex_create(orbisjvm_mutex_t* mutex, const orbisjvm_mutex_attr_t* attr) {
    if (!mutex) return -1;

    orbisjvm_mutex_t m = orbisjvm_malloc(sizeof(struct orbisjvm_mutex), ORBISJVM_MEM_ZERO);
    if (!m) return -1;

    pthread_mutexattr_t pthread_attr;
    pthread_mutexattr_init(&pthread_attr);

    if (attr && attr->recursive) {
        pthread_mutexattr_settype(&pthread_attr, PTHREAD_MUTEX_RECURSIVE);
        m->recursive = 1;
    } else {
        pthread_mutexattr_settype(&pthread_attr, PTHREAD_MUTEX_NORMAL);
        m->recursive = 0;
    }

    int ret = pthread_mutex_init(&m->mutex, &pthread_attr);
    pthread_mutexattr_destroy(&pthread_attr);

    if (ret != 0) {
        orbisjvm_free(m);
        return -ret;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_mutexes_created++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    *mutex = m;
    return 0;
}

int orbisjvm_mutex_destroy(orbisjvm_mutex_t mutex) {
    if (!mutex) return -1;

    int ret = pthread_mutex_destroy(&mutex->mutex);
    if (ret != 0) {
        return -ret;
    }

    orbisjvm_free(mutex);

    orbisjvm_mutex_lock(g_stats_mutex);
    g_mutexes_destroyed++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return 0;
}

int orbisjvm_mutex_lock(orbisjvm_mutex_t mutex) {
    if (!mutex) return -1;
    int ret = pthread_mutex_lock(&mutex->mutex);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_mutex_trylock(orbisjvm_mutex_t mutex) {
    if (!mutex) return -1;
    int ret = pthread_mutex_trylock(&mutex->mutex);
    if (ret == EBUSY) return -EBUSY;
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_mutex_unlock(orbisjvm_mutex_t mutex) {
    if (!mutex) return -1;
    int ret = pthread_mutex_unlock(&mutex->mutex);
    return ret == 0 ? 0 : -ret;
}

/* Condition variable functions */

void orbisjvm_cond_attr_init(orbisjvm_cond_attr_t* attr) {
    if (!attr) return;
    attr->clock_type = 0;
}

void orbisjvm_cond_attr_destroy(orbisjvm_cond_attr_t* attr) {
    (void)attr;
}

int orbisjvm_cond_create(orbisjvm_cond_t* cond, const orbisjvm_cond_attr_t* attr) {
    if (!cond) return -1;

    orbisjvm_cond_t c = orbisjvm_malloc(sizeof(struct orbisjvm_cond), ORBISJVM_MEM_ZERO);
    if (!c) return -1;

    pthread_condattr_t pthread_attr;
    pthread_condattr_init(&pthread_attr);

    if (attr && attr->clock_type == 1) {
        pthread_condattr_setclock(&pthread_attr, CLOCK_REALTIME);
    }

    int ret = pthread_cond_init(&c->cond, &pthread_attr);
    pthread_condattr_destroy(&pthread_attr);

    if (ret != 0) {
        orbisjvm_free(c);
        return -ret;
    }

    orbisjvm_mutex_lock(g_stats_mutex);
    g_condvars_created++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    *cond = c;
    return 0;
}

int orbisjvm_cond_destroy(orbisjvm_cond_t cond) {
    if (!cond) return -1;

    int ret = pthread_cond_destroy(&cond->cond);
    if (ret != 0) {
        return -ret;
    }

    orbisjvm_free(cond);

    orbisjvm_mutex_lock(g_stats_mutex);
    g_condvars_destroyed++;
    orbisjvm_mutex_unlock(g_stats_mutex);

    return 0;
}

int orbisjvm_cond_wait(orbisjvm_cond_t cond, orbisjvm_mutex_t mutex) {
    if (!cond || !mutex) return -1;
    int ret = pthread_cond_wait(&cond->cond, &mutex->mutex);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_cond_timedwait(orbisjvm_cond_t cond, orbisjvm_mutex_t mutex, uint64_t abstime) {
    if (!cond || !mutex) return -1;

    struct timespec ts;
    ts.tv_sec = abstime / 1000000000;
    ts.tv_nsec = abstime % 1000000000;

    int ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &ts);
    if (ret == ETIMEDOUT) return -ETIMEDOUT;
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_cond_signal(orbisjvm_cond_t cond) {
    if (!cond) return -1;
    int ret = pthread_cond_signal(&cond->cond);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_cond_broadcast(orbisjvm_cond_t cond) {
    if (!cond) return -1;
    int ret = pthread_cond_broadcast(&cond->cond);
    return ret == 0 ? 0 : -ret;
}

/* Thread-local storage */

int orbisjvm_tls_create(orbisjvm_tls_key_t* key, void (*destructor)(void*)) {
    if (!key) return -1;

    pthread_key_t pkey;
    int ret = pthread_key_create(&pkey, destructor);
    if (ret != 0) return -ret;

    *key = (orbisjvm_tls_key_t)pkey;
    return 0;
}

int orbisjvm_tls_delete(orbisjvm_tls_key_t key) {
    int ret = pthread_key_delete((pthread_key_t)key);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_tls_set(orbisjvm_tls_key_t key, void* value) {
    int ret = pthread_setspecific((pthread_key_t)key, value);
    return ret == 0 ? 0 : -ret;
}

void* orbisjvm_tls_get(orbisjvm_tls_key_t key) {
    return pthread_getspecific((pthread_key_t)key);
}

/* Once control */

int orbisjvm_once(orbisjvm_once_t* once, void (*init)(void)) {
    if (!once || !init) return -1;

    static pthread_once_t pthread_once = PTHREAD_ONCE_INIT;
    /* Note: This is a simplified implementation. Real implementation would need
       to map orbisjvm_once_t to pthread_once_t properly. */
    (void)once;
    init();
    return 0;
}

/* Thread priority */

int orbisjvm_thread_set_priority(int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    int policy = SCHED_OTHER;
    int ret = pthread_setschedparam(pthread_self(), policy, &param);
    return ret == 0 ? 0 : -ret;
}

int orbisjvm_thread_get_priority(void) {
    struct sched_param param;
    int policy;
    int ret = pthread_getschedparam(pthread_self(), &policy, &param);
    return ret == 0 ? param.sched_priority : -ret;
}

/* Statistics */

void orbisjvm_get_thread_stats(orbisjvm_thread_stats_t* stats) {
    if (!stats) return;

    orbisjvm_mutex_lock(g_stats_mutex);
    stats->threads_created = g_threads_created;
    stats->threads_destroyed = g_threads_destroyed;
    stats->mutexes_created = g_mutexes_created;
    stats->mutexes_destroyed = g_mutexes_destroyed;
    stats->condvars_created = g_condvars_created;
    stats->condvars_destroyed = g_condvars_destroyed;
    stats->current_thread_count = g_current_thread_count;
    stats->peak_thread_count = g_peak_thread_count;
    orbisjvm_mutex_unlock(g_stats_mutex);
}