/**
 * @file memory.c
 * @brief OrbisJVM Memory Management Implementation
 *
 * Implements memory allocation using Orbis OS APIs via musl libc.
 */

#include <orbisjvm/memory.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/threading.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Platform allocation functions - these would use actual Orbis OS APIs */
/* For now, we use standard malloc/free as placeholders for the musl libc port */

/* Allocation tracking (for diagnostics) */
typedef struct alloc_tracker_entry {
    void* ptr;
    size_t size;
    orbisjvm_mem_flags_t flags;
    const char* file;
    int line;
    const char* func;
    uint64_t timestamp;
    struct alloc_tracker_entry* next;
} alloc_tracker_entry_t;

static orbisjvm_mutex_t g_tracker_mutex;
static alloc_tracker_entry_t* g_alloc_head = NULL;
static size_t g_current_allocations = 0;
static size_t g_total_allocated = 0;
static size_t g_peak_allocated = 0;
static uint64_t g_total_allocs = 0;
static uint64_t g_total_frees = 0;
static int g_tracking_enabled = 0;
static orbisjvm_tls_key_t g_tracker_tls_key;

/**
 * @brief Initialize memory subsystem
 */
int orbisjvm_memory_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_MEMORY, "Initializing memory subsystem");

    int ret = orbisjvm_mutex_create(&g_tracker_mutex, NULL);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Failed to create tracker mutex");
        return ret;
    }

    ret = orbisjvm_tls_create(&g_tracker_tls_key, NULL);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Failed to create TLS key");
        return ret;
    }

    return 0;
}

/**
 * @brief Shutdown memory subsystem
 */
void orbisjvm_memory_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_MEMORY, "Shutting down memory subsystem");

    /* Report any leaks */
    size_t leaks = orbisjvm_check_memory_leaks();
    if (leaks > 0) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_MEMORY, "Detected %zu memory leaks at shutdown", leaks);
    }

    orbisjvm_tls_delete(g_tracker_tls_key);
    orbisjvm_mutex_destroy(g_tracker_mutex);
}

/**
 * @brief Add allocation to tracker
 */
static void tracker_add(void* ptr, size_t size, orbisjvm_mem_flags_t flags,
                        const char* file, int line, const char* func) {
    if (!g_tracking_enabled) return;

    alloc_tracker_entry_t* entry = malloc(sizeof(alloc_tracker_entry_t));
    if (!entry) return;

    entry->ptr = ptr;
    entry->size = size;
    entry->flags = flags;
    entry->file = file;
    entry->line = line;
    entry->func = func;
    entry->timestamp = orbisjvm_get_monotonic_ns();

    orbisjvm_mutex_lock(g_tracker_mutex);
    entry->next = g_alloc_head;
    g_alloc_head = entry;
    g_current_allocations++;
    g_total_allocated += size;
    if (g_total_allocated > g_peak_allocated) {
        g_peak_allocated = g_total_allocated;
    }
    g_total_allocs++;
    orbisjvm_mutex_unlock(g_tracker_mutex);
}

/**
 * @brief Remove allocation from tracker
 */
static void tracker_remove(void* ptr) {
    if (!g_tracking_enabled) return;

    orbisjvm_mutex_lock(g_tracker_mutex);
    alloc_tracker_entry_t** pp = &g_alloc_head;
    while (*pp) {
        if ((*pp)->ptr == ptr) {
            alloc_tracker_entry_t* entry = *pp;
            *pp = entry->next;
            g_current_allocations--;
            g_total_allocated -= entry->size;
            g_total_frees++;
            free(entry);
            break;
        }
        pp = &(*pp)->next;
    }
    orbisjvm_mutex_unlock(g_tracker_mutex);
}

/**
 * @brief Allocate memory
 */
void* orbisjvm_malloc(size_t size, orbisjvm_mem_flags_t flags) {
    if (size == 0) return NULL;

    void* ptr = NULL;

    if (flags & ORBISJVM_MEM_ALIGNED) {
        /* Page-aligned allocation */
        size_t page_size = orbisjvm_get_page_size();
        size = (size + page_size - 1) & ~(page_size - 1);
        int ret = posix_memalign(&ptr, page_size, size);
        if (ret != 0) ptr = NULL;
    } else {
        ptr = malloc(size);
    }

    if (ptr && (flags & ORBISJVM_MEM_ZERO)) {
        memset(ptr, 0, size);
    }

    tracker_add(ptr, size, flags, __FILE__, __LINE__, __func__);

    if (!ptr) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Allocation failed: size=%zu flags=0x%x", size, flags);
    } else {
        orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Allocated %zu bytes at %p", size, ptr);
    }

    return ptr;
}

/**
 * @brief Allocate zero-initialized memory
 */
void* orbisjvm_calloc(size_t count, size_t size, orbisjvm_mem_flags_t flags) {
    if (count == 0 || size == 0) return NULL;

    /* Check for overflow */
    if (count > SIZE_MAX / size) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "calloc overflow: count=%zu size=%zu", count, size);
        return NULL;
    }

    size_t total = count * size;
    flags |= ORBISJVM_MEM_ZERO;
    return orbisjvm_malloc(total, flags);
}

/**
 * @brief Reallocate memory
 */
void* orbisjvm_realloc(void* ptr, size_t size, orbisjvm_mem_flags_t flags) {
    if (!ptr) return orbisjvm_malloc(size, flags);
    if (size == 0) {
        orbisjvm_free(ptr);
        return NULL;
    }

    tracker_remove(ptr);
    void* new_ptr = realloc(ptr, size);

    if (new_ptr && (flags & ORBISJVM_MEM_ZERO)) {
        /* Zero the new portion if expanded */
        /* Note: we don't know old size, so this is approximate */
    }

    tracker_add(new_ptr, size, flags, __FILE__, __LINE__, __func__);

    if (!new_ptr) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Reallocation failed: size=%zu flags=0x%x", size, flags);
    } else {
        orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Reallocated to %zu bytes at %p", size, new_ptr);
    }

    return new_ptr;
}

/**
 * @brief Free allocated memory
 */
void orbisjvm_free(void* ptr) {
    if (!ptr) return;

    tracker_remove(ptr);
    free(ptr);
    orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Freed memory at %p", ptr);
}

/**
 * @brief Allocate aligned memory
 */
void* orbisjvm_aligned_alloc(size_t alignment, size_t size, orbisjvm_mem_flags_t flags) {
    if (size == 0) return NULL;
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Invalid alignment: %zu", alignment);
        return NULL;
    }

    void* ptr = NULL;
    int ret = posix_memalign(&ptr, alignment, size);
    if (ret != 0) ptr = NULL;

    if (ptr && (flags & ORBISJVM_MEM_ZERO)) {
        memset(ptr, 0, size);
    }

    tracker_add(ptr, size, flags, __FILE__, __LINE__, __func__);

    return ptr;
}

/**
 * @brief Allocate memory pages
 */
void* orbisjvm_page_alloc(size_t num_pages, orbisjvm_mem_flags_t flags) {
    if (num_pages == 0) return NULL;

    size_t page_size = orbisjvm_get_page_size();
    size_t size = num_pages * page_size;

    /* Use aligned allocation for page allocation */
    flags |= ORBISJVM_MEM_ALIGNED;
    void* ptr = orbisjvm_malloc(size, flags);

    orbisjvm_log_debug(ORBISJVM_LOG_CAT_MEMORY, "Allocated %zu pages (%zu bytes) at %p",
                       num_pages, size, ptr);

    return ptr;
}

/**
 * @brief Free memory pages
 */
void orbisjvm_page_free(void* ptr, size_t num_pages) {
    if (!ptr) return;

    size_t page_size = orbisjvm_get_page_size();
    size_t size = num_pages * page_size;

    orbisjvm_log_debug(ORBISJVM_LOG_CAT_MEMORY, "Freeing %zu pages (%zu bytes) at %p",
                       num_pages, size, ptr);

    orbisjvm_free(ptr);
}

/**
 * @brief Change memory protection
 */
int orbisjvm_mprotect(void* addr, size_t size, orbisjvm_prot_t prot) {
    if (!addr || size == 0) return -1;

    int mprot = 0;
    if (prot & ORBISJVM_PROT_READ) mprot |= PROT_READ;
    if (prot & ORBISJVM_PROT_WRITE) mprot |= PROT_WRITE;
    if (prot & ORBISJVM_PROT_EXEC) mprot |= PROT_EXEC;

    /* Align to page boundary */
    size_t page_size = orbisjvm_get_page_size();
    uintptr_t addr_int = (uintptr_t)addr;
    uintptr_t aligned_addr = addr_int & ~(page_size - 1);
    size_t aligned_size = size + (addr_int - aligned_addr);
    aligned_size = (aligned_size + page_size - 1) & ~(page_size - 1);

    int ret = mprotect((void*)aligned_addr, aligned_size, mprot);
    if (ret != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "mprotect failed: addr=%p size=%zu prot=%d errno=%d",
                           addr, size, prot, errno);
        return -errno;
    }

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Changed protection: addr=%p size=%zu prot=%d",
                       addr, size, prot);
    return 0;
}

/**
 * @brief Map memory (mmap equivalent)
 */
void* orbisjvm_mmap(void* addr, size_t size, orbisjvm_prot_t prot,
                    orbisjvm_map_flags_t flags, int fd, int64_t offset) {
    if (size == 0) return ORBISJVM_MAP_FAILED;

    int mprot = 0;
    if (prot & ORBISJVM_PROT_READ) mprot |= PROT_READ;
    if (prot & ORBISJVM_PROT_WRITE) mprot |= PROT_WRITE;
    if (prot & ORBISJVM_PROT_EXEC) mprot |= PROT_EXEC;

    int mflags = 0;
    if (flags & ORBISJVM_MAP_SHARED) mflags |= MAP_SHARED;
    if (flags & ORBISJVM_MAP_PRIVATE) mflags |= MAP_PRIVATE;
    if (flags & ORBISJVM_MAP_FIXED) mflags |= MAP_FIXED;
    if (flags & ORBISJVM_MAP_ANONYMOUS) mflags |= MAP_ANONYMOUS;

    if (fd == -1) {
        mflags |= MAP_ANONYMOUS;
        fd = -1;
        offset = 0;
    }

    void* result = mmap(addr, size, mprot, mflags, fd, offset);
    if (result == MAP_FAILED) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "mmap failed: addr=%p size=%zu prot=%d flags=%d fd=%d offset=%lld errno=%d",
                           addr, size, prot, flags, fd, offset, errno);
        return ORBISJVM_MAP_FAILED;
    }

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Mapped memory: addr=%p size=%zu prot=%d flags=%d",
                       result, size, prot, flags);

    /* Track anonymous mappings */
    if (flags & ORBISJVM_MAP_ANONYMOUS) {
        tracker_add(result, size, ORBISJVM_MEM_ALIGNED, __FILE__, __LINE__, __func__);
    }

    return result;
}

/**
 * @brief Unmap memory (munmap equivalent)
 */
int orbisjvm_munmap(void* addr, size_t size) {
    if (!addr || size == 0) return -1;

    /* Remove from tracker if tracked */
    tracker_remove(addr);

    int ret = munmap(addr, size);
    if (ret != 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "munmap failed: addr=%p size=%zu errno=%d",
                           addr, size, errno);
        return -errno;
    }

    orbisjvm_log_trace(ORBISJVM_LOG_CAT_MEMORY, "Unmapped memory: addr=%p size=%zu", addr, size);
    return 0;
}

/**
 * @brief Get system page size
 */
size_t orbisjvm_get_page_size(void) {
    return sysconf(_SC_PAGESIZE);
}

/**
 * @brief Get memory statistics
 */
int orbisjvm_get_memory_stats(uint64_t* total_total, uint64_t* available, uint64_t* used) {
    /* On Orbis OS, we would use actual system calls */
    /* For now, return placeholder values */
    if (total_total) *total_total = 8ULL * 1024 * 1024 * 1024;
    if (available) *available = 6ULL * 1024 * 1024 * 1024;
    if (used) *used = g_total_allocated;
    return 0;
}

/**
 * @brief Enable/disable allocation tracking
 */
void orbisjvm_set_allocation_tracking(int enable) {
    g_tracking_enabled = enable ? 1 : 0;
    orbisjvm_log_info(ORBISJVM_LOG_CAT_MEMORY, "Allocation tracking %s", enable ? "enabled" : "disabled");
}

/**
 * @brief Get allocation tracking statistics
 */
void orbisjvm_get_allocation_stats(size_t* current_allocations,
                                   size_t* total_allocated,
                                   size_t* peak_allocated,
                                   uint64_t* total_allocations,
                                   uint64_t* total_frees) {
    if (current_allocations) *current_allocations = g_current_allocations;
    if (total_allocated) *total_allocated = g_total_allocated;
    if (peak_allocated) *peak_allocated = g_peak_allocated;
    if (total_allocations) *total_allocations = g_total_allocs;
    if (total_frees) *total_frees = g_total_frees;
}

/**
 * @brief Dump current allocations to callback
 */
void orbisjvm_dump_allocations(orbisjvm_alloc_dump_cb_t callback, void* user_data) {
    if (!callback) return;

    orbisjvm_mutex_lock(g_tracker_mutex);
    alloc_tracker_entry_t* entry = g_alloc_head;
    while (entry) {
        orbisjvm_alloc_tracker_t tracker = {
            .ptr = entry->ptr,
            .size = entry->size,
            .flags = entry->flags,
            .file = entry->file,
            .line = entry->line,
            .func = entry->func,
            .timestamp = entry->timestamp,
        };
        callback(&tracker, user_data);
        entry = entry->next;
    }
    orbisjvm_mutex_unlock(g_tracker_mutex);
}

/**
 * @brief Check for memory leaks
 */
size_t orbisjvm_check_memory_leaks(void) {
    orbisjvm_mutex_lock(g_tracker_mutex);
    size_t leaks = g_current_allocations;
    orbisjvm_mutex_unlock(g_tracker_mutex);
    return leaks;
}

/**
 * @brief Reserve virtual address space
 */
void* orbisjvm_reserve_address_space(void* addr, size_t size) {
    return orbisjvm_mmap(addr, size, ORBISJVM_PROT_NONE,
                         ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS, -1, 0);
}

/**
 * @brief Commit reserved address space
 */
int orbisjvm_commit_address_space(void* addr, size_t size, orbisjvm_prot_t prot) {
    return orbisjvm_mprotect(addr, size, prot);
}

/**
 * @brief Decommit address space
 */
int orbisjvm_decommit_address_space(void* addr, size_t size) {
    return orbisjvm_mprotect(addr, size, ORBISJVM_PROT_NONE);
}

/**
 * @brief Release reserved address space
 */
int orbisjvm_release_address_space(void* addr, size_t size) {
    return orbisjvm_munmap(addr, size);
}