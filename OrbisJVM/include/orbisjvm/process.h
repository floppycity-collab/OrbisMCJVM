/**
 * @file process.h
 * @brief OrbisJVM Process Management API
 *
 * Provides process creation, execution, and management functions.
 * Note: Full process creation may be limited on Orbis OS.
 */

#ifndef ORBISJVM_PROCESS_H
#define ORBISJVM_PROCESS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Process handle
 */
typedef struct orbisjvm_process* orbisjvm_process_t;

/**
 * @brief Process creation flags
 */
typedef enum {
    ORBISJVM_PROC_NONE       = 0,
    ORBISJVM_PROC_DETACHED   = (1 << 0),  ///< Detached process
    ORBISJVM_PROC_NEW_CONSOLE = (1 << 1), ///< New console (if supported)
    ORBISJVM_PROC_SUSPENDED   = (1 << 2), ///< Create suspended
    ORBISJVM_PROC_INHERIT_FDS = (1 << 3), ///< Inherit file descriptors
} orbisjvm_proc_flags_t;

/**
 * @brief Process attributes
 */
typedef struct {
    orbisjvm_proc_flags_t flags;
    const char* working_dir;    ///< Working directory (NULL = inherit)
    char** envp;                ///< Environment variables (NULL = inherit)
    int stdin_fd;               ///< Stdin file descriptor (-1 = inherit)
    int stdout_fd;              ///< Stdout file descriptor (-1 = inherit)
    int stderr_fd;              ///< Stderr file descriptor (-1 = inherit)
    size_t stack_size;          ///< Stack size (0 = default)
    int priority;               ///< Process priority
} orbisjvm_proc_attr_t;

/**
 * @brief Initialize process attributes with defaults
 *
 * @param[out] attr Process attributes to initialize
 */
void orbisjvm_proc_attr_init(orbisjvm_proc_attr_t* attr);

/**
 * @brief Destroy process attributes
 *
 * @param attr Process attributes to destroy
 */
void orbisjvm_proc_attr_destroy(orbisjvm_proc_attr_t* attr);

/**
 * @brief Create a new process
 *
 * @param[out] process Output process handle
 * @param path Executable path
 * @param argv Argument vector (NULL-terminated)
 * @param attr Process attributes (or NULL for defaults)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_create(orbisjvm_process_t* process,
                         const char* path,
                         char* const argv[],
                         const orbisjvm_proc_attr_t* attr);

/**
 * @brief Wait for process to exit
 *
 * @param process Process handle
 * @param[out] exit_code Exit code (or NULL)
 * @param options Wait options (0 = blocking, ORBISJVM_PROC_WNOHANG = non-blocking)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_wait(orbisjvm_process_t process, int* exit_code, int options);

#define ORBISJVM_PROC_WNOHANG 1

/**
 * @brief Try to wait for process (non-blocking)
 *
 * @param process Process handle
 * @param[out] exit_code Exit code (or NULL)
 * @return 0 if still running, 1 if exited, negative error code on failure
 */
int orbisjvm_proc_trywait(orbisjvm_process_t process, int* exit_code);

/**
 * @brief Get process exit code
 *
 * @param process Process handle
 * @param[out] exit_code Exit code
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_get_exit_code(orbisjvm_process_t process, int* exit_code);

/**
 * @brief Get process ID
 *
 * @param process Process handle
 * @return Process ID
 */
uint32_t orbisjvm_proc_get_pid(orbisjvm_process_t process);

/**
 * @brief Terminate process
 *
 * @param process Process handle
 * @param exit_code Exit code
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_terminate(orbisjvm_process_t process, int exit_code);

/**
 * @brief Kill process (force)
 *
 * @param process Process handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_kill(orbisjvm_process_t process);

/**
 * @brief Close process handle
 *
 * @param process Process handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_proc_close(orbisjvm_process_t process);

/**
 * @brief Execute command and capture output
 *
 * @param cmd Command to execute
 * @param[out] stdout_buf Stdout buffer (or NULL)
 * @param stdout_size Stdout buffer size
 * @param[out] stderr_buf Stderr buffer (or NULL)
 * @param stderr_size Stderr buffer size
 * @param[out] exit_code Exit code (or NULL)
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_exec_capture(const char* cmd,
                          char* stdout_buf, size_t stdout_size,
                          char* stderr_buf, size_t stderr_size,
                          int* exit_code, int timeout_ms);

/**
 * @brief Execute command synchronously
 *
 * @param cmd Command to execute
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return Exit code, or negative error code on failure
 */
int orbisjvm_exec_sync(const char* cmd, int timeout_ms);

/**
 * @brief Get number of available CPU cores
 *
 * @return Number of CPU cores
 */
int orbisjvm_get_cpu_count(void);

/**
 * @brief Get current process CPU affinity mask
 *
 * @param[out] mask Affinity mask (bit per CPU)
 * @param mask_size Mask size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_cpu_affinity(uint64_t* mask, size_t mask_size);

/**
 * @brief Set current process CPU affinity mask
 *
 * @param mask Affinity mask (bit per CPU)
 * @param mask_size Mask size in bytes
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_cpu_affinity(const uint64_t* mask, size_t mask_size);

/**
 * @brief Exit current process
 *
 * @param exit_code Exit code
 */
void orbisjvm_exit(int exit_code) __attribute__((noreturn));

/**
 * @brief Abort current process
 */
void orbisjvm_abort(void) __attribute__((noreturn));

/**
 * @brief Register atexit handler
 *
 * @param func Function to call at exit
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_atexit(void (*func)(void));

/**
 * @brief Register at_quick_exit handler
 *
 * @param func Function to call at quick exit
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_at_quick_exit(void (*func)(void));

/**
 * @brief Quick exit
 *
 * @param exit_code Exit code
 */
void orbisjvm_quick_exit(int exit_code) __attribute__((noreturn));

/**
 * @brief Signal handling (limited on Orbis OS)
 */
typedef void (*orbisjvm_sighandler_t)(int);

/**
 * @brief Signal handler for SIGSEGV, SIGABRT, etc.
 */
orbisjvm_sighandler_t orbisjvm_signal(int signum, orbisjvm_sighandler_t handler);

/**
 * @brief Raise signal
 *
 * @param signum Signal number
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_raise(int signum);

/**
 * @brief Signal numbers
 */
#define ORBISJVM_SIGINT   2
#define ORBISJVM_SIGQUIT  3
#define ORBISJVM_SIGILL   4
#define ORBISJVM_SIGTRAP  5
#define ORBISJVM_SIGABRT  6
#define ORBISJVM_SIGBUS   7
#define ORBISJVM_SIGFPE   8
#define ORBISJVM_SIGKILL  9
#define ORBISJVM_SIGSEGV  11
#define ORBISJVM_SIGPIPE  13
#define ORBISJVM_SIGALRM  14
#define ORBISJVM_SIGTERM  15

/**
 * @brief Resource limits
 */
typedef struct {
    uint64_t rlim_cur;  ///< Soft limit
    uint64_t rlim_max;  ///< Hard limit
} orbisjvm_rlimit_t;

/**
 * @brief Resource limit types
 */
typedef enum {
    ORBISJVM_RLIMIT_CPU     = 0,  ///< CPU time (seconds)
    ORBISJVM_RLIMIT_FSIZE   = 1,  ///< File size (bytes)
    ORBISJVM_RLIMIT_DATA    = 2,  ///< Data segment size (bytes)
    ORBISJVM_RLIMIT_STACK   = 3,  ///< Stack size (bytes)
    ORBISJVM_RLIMIT_CORE    = 4,  ///< Core file size (bytes)
    ORBISJVM_RLIMIT_RSS     = 5,  ///< Resident set size (bytes)
    ORBISJVM_RLIMIT_NOFILE  = 6,  ///< Number of open files
    ORBISJVM_RLIMIT_AS      = 7,  ///< Address space size (bytes)
    ORBISJVM_RLIMIT_NPROC   = 8,  ///< Number of processes
    ORBISJVM_RLIMIT_MEMLOCK = 9,  ///< Locked memory (bytes)
} orbisjvm_rlimit_resource_t;

/**
 * @brief Get resource limit
 *
 * @param resource Resource type
 * @param[out] limit Limit structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_getrlimit(orbisjvm_rlimit_resource_t resource, orbisjvm_rlimit_t* limit);

/**
 * @brief Set resource limit
 *
 * @param resource Resource type
 * @param limit Limit structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_setrlimit(orbisjvm_rlimit_resource_t resource, const orbisjvm_rlimit_t* limit);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_PROCESS_H