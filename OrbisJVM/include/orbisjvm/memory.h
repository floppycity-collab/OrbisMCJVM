/**
 * @file memory.h
 * @brief OrbisJVM Memory Management API
 *
 * Provides memory allocation, deallocation, and management functions
 * compatible with Orbis OS using the OpenOrbis toolchain.
 */

#ifndef ORBISJVM_MEMORY_H
#define ORBISJVM_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory allocation flags
 */
typedef enum {
    ORBISJVM_MEM_NONE       = 0,
    ORBISJVM_MEM_ZERO       = (1 << 0),  ///< Zero-initialize allocated memory
    ORBISJVM_MEM_ALIGNED    = (1 << 1),  ///< Align to page boundary
    ORBISJVM_MEM_EXECUTABLE = (1 << 2),  ///< Allocate executable memory (for JIT)
    ORBISJVM_MEM_LARGE_PAGE = (1 << 3),  ///< Use large pages if available
    ORBISJVM_MEM_NO_CACHE   = (1 << 4),  ///< Uncached memory (for device memory)
    ORBISJVM_MEM_PINNED     = (1 << 5),  ///< Pinned memory (cannot be swapped)
} orbisjvm_mem_flags_t;

/**
 * @brief Memory protection flags (for mprotect equivalent)
 */
typedef enum {
    ORBISJVM_PROT_NONE  = 0,
    ORBISJVM_PROT_READ  = (1 << 0),
    ORBISJVM_PROT_WRITE = (1 << 1),
    ORBISJVM_PROT_EXEC  = (1 << 2),
} orbisjvm_prot_t;

/**
 * @brief Memory mapping flags
 */
typedef enum {
    ORBISJVM_MAP_NONE      = 0,
    ORBISJVM_MAP_SHARED    = (1 << 0),  ///< Share changes with other mappings
    ORBISJVM_MAP_PRIVATE   = (1 << 1),  ///< Private copy-on-write mapping
    ORBISJVM_MAP_FIXED     = (1 << 2),  ///< Interpret addr as exact address
    ORBISJVM_MAP_ANONYMOUS = (1 << 3),  ///< Anonymous mapping (no file)
    ORBISJVM_MAP_STACK     = (1 << 4),  ///< Stack allocation hint
} orbisjvm_map_flags_t;

/**
 * @brief Allocate memory
 *
 * @param size Size in bytes to allocate
 * @param flags Allocation flags (orbisjvm_mem_flags_t)
 * @return Pointer to allocated memory, or NULL on failure
 */
void* orbisjvm_malloc(size_t size, orbisjvm_mem_flags_t flags);

/**
 * @brief Allocate zero-initialized memory
 *
 * @param count Number of elements
 * @param size Size of each element
 * @param flags Allocation flags
 * @return Pointer to allocated memory, or NULL on failure
 */
void* orbisjvm_calloc(size_t count, size_t size, orbisjvm_mem_flags_t flags);

/**
 * @brief Reallocate memory
 *
 * @param ptr Pointer to previously allocated memory
 * @param size New size in bytes
 * @param flags Allocation flags
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* orbisjvm_realloc(void* ptr, size_t size, orbisjvm_mem_flags_t flags);

/**
 * @brief Free allocated memory
 *
 * @param ptr Pointer to memory allocated by orbisjvm_malloc/calloc/realloc
 */
void orbisjvm_free(void* ptr);

/**
 * @brief Allocate aligned memory
 *
 * @param alignment Alignment requirement (must be power of 2)
 * @param size Size in bytes to allocate
 * @param flags Allocation flags
 * @return Pointer to aligned memory, or NULL on failure
 */
void* orbisjvm_aligned_alloc(size_t alignment, size_t size, orbisjvm_mem_flags_t flags);

/**
 * @brief Allocate memory pages
 *
 * Direct page allocation for large buffers and JIT code.
 *
 * @param num_pages Number of pages to allocate
 * @param flags Allocation flags
 * @return Pointer to allocated pages, or NULL on failure
 */
void* orbisjvm_page_alloc(size_t num_pages, orbisjvm_mem_flags_t flags);

/**
 * @brief Free memory pages
 *
 * @param ptr Pointer returned by orbisjvm_page_alloc
 * @param num_pages Number of pages to free
 */
void orbisjvm_page_free(void* ptr, size_t num_pages);

/**
 * @brief Change memory protection
 *
 * @param addr Base address (must be page-aligned)
 * @param size Size in bytes (must be page-aligned)
 * @param prot New protection flags
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_mprotect(void* addr, size_t size, orbisjvm_prot_t prot);

/**
 * @brief Map memory (mmap equivalent)
 *
 * @param addr Hint address (or NULL for any)
 * @param size Size in bytes
 * @param prot Protection flags
 * @param flags Mapping flags
 * @param fd File descriptor (or -1 for anonymous)
 * @param offset Offset in file
 * @return Mapped address, or MAP_FAILED on failure
 */
#define ORBISJVM_MAP_FAILED ((void*)-1)
void* orbisjvm_mmap(void* addr, size_t size, orbisjvm_prot_t prot,
                    orbisjvm_map_flags_t flags, int fd, int64_t offset);

/**
 * @brief Unmap memory (munmap equivalent)
 *
 * @param addr Base address
 * @param size Size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_munmap(void* addr, size_t size);

/**
 * @brief Get system page size
 *
 * @return Page size in bytes
 */
size_t orbisjvm_get_page_size(void);

/**
 * @brief Get memory statistics
 *
 * @param[out] total_total Total physical memory
 * @param[out] available Available physical memory
 * @param[out] used Current process virtual memory usage
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_memory_stats(uint64_t* total_total, uint64_t* available, uint64_t* used);

/**
 * @brief Memory allocation tracker entry (for diagnostics)
 */
typedef struct {
    void* ptr;
    size_t size;
    orbisjvm_mem_flags_t flags;
    const char* file;
    int line;
    const char* func;
    uint64_t timestamp;
} orbisjvm_alloc_tracker_t;

/**
 * @brief Enable/disable allocation tracking
 *
 * @param enable 1 to enable, 0 to disable
 */
void orbisjvm_set_allocation_tracking(int enable);

/**
 * @brief Get allocation tracking statistics
 *
 * @param[out] current_allocations Number of currently tracked allocations
 * @param[out] total_allocated Total bytes currently allocated
 * @param[out] peak_allocated Peak bytes allocated
 * @param[out] total_allocations Total number of allocations since tracking started
 * @param[out] total_frees Total number of frees since tracking started
 */
void orbisjvm_get_allocation_stats(size_t* current_allocations,
                                   size_t* total_allocated,
                                   size_t* peak_allocated,
                                   uint64_t* total_allocations,
                                   uint64_t* total_frees);

/**
 * @brief Dump current allocations to callback
 *
 * @param callback Function called for each allocation
 * @param user_data User data passed to callback
 */
typedef void (*orbisjvm_alloc_dump_cb_t)(const orbisjvm_alloc_tracker_t* entry, void* user_data);
void orbisjvm_dump_allocations(orbisjvm_alloc_dump_cb_t callback, void* user_data);

/**
 * @brief Check for memory leaks (allocations not freed)
 *
 * @return Number of leaked allocations
 */
size_t orbisjvm_check_memory_leaks(void);

/**
 * @brief Reserve virtual address space without committing physical memory
 *
 * @param addr Hint address (or NULL)
 * @param size Size in bytes
 * @return Reserved address, or MAP_FAILED on failure
 */
void* orbisjvm_reserve_address_space(void* addr, size_t size);

/**
 * @brief Commit reserved address space
 *
 * @param addr Address returned by orbisjvm_reserve_address_space
 * @param size Size in bytes
 * @param prot Protection flags
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_commit_address_space(void* addr, size_t size, orbisjvm_prot_t prot);

/**
 * @brief Decommit address space (release physical pages, keep reservation)
 *
 * @param addr Base address
 * @param size Size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_decommit_address_space(void* addr, size_t size);

/**
 * @brief Release reserved address space
 *
 * @param addr Address returned by orbisjvm_reserve_address_space
 * @param size Size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_release_address_space(void* addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_MEMORY_H