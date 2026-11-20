/**
 * @file process.c
 * @brief OrbisJVM Process Management Implementation
 *
 * Implements process creation, execution, and management functions.
 * Note: Full process creation may be limited on Orbis OS.
 */

#include <orbisjvm/process.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/threading.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>

/* Process structure */
struct orbisjvm_process {
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
    int detached;
};

/**
 * @brief Initialize process subsystem
 */
int orbisjvm_process_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing process subsystem");
    return 0;
}

/**
 * @brief Shutdown process subsystem
 */
void orbisjvm_process_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down process subsystem");
}

/* Process attribute functions */

void orbisjvm_proc_attr_init(orbisjvm_proc_attr_t* attr) {
    if (!attr) return;
    attr->flags = ORBISJVM_PROC_NONE;
    attr->working_dir = NULL;
    attr->envp = NULL;
    attr->stdin_fd = -1;
    attr->stdout_fd = -1;
    attr->stderr_fd = -1;
    attr->stack_size = 0;
    attr->priority = 0;
}

void orbisjvm_proc_attr_destroy(orbisjvm_proc_attr_t* attr) {
    (void)attr;
}

/* Process functions */

int orbisjvm_proc_create(orbisjvm_process_t* process,
                         const char* path,
                         char* const argv[],
                         const orbisjvm_proc_attr_t* attr) {
    if (!process || !path || !argv) return -1;

    pid_t pid = fork();
    if (pid < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "fork() failed: errno=%d", errno);
        return -errno;
    }

    if (pid == 0) {
        /* Child process */

        /* Set up working directory */
        if (attr && attr->working_dir) {
            chdir(attr->working_dir);
        }

        /* Set up file descriptors */
        if (attr) {
            if (attr->stdin_fd >= 0) {
                dup2(attr->stdin_fd, STDIN_FILENO);
            }
            if (attr->stdout_fd >= 0) {
                dup2(attr->stdout_fd, STDOUT_FILENO);
            }
            if (attr->stderr_fd >= 0) {
                dup2(attr->stderr_fd, STDERR_FILENO);
            }
        }

        /* Set up environment */
        if (attr && attr->envp) {
            extern char** environ;
            environ = attr->envp;
        }

        /* Execute */
        execv(path, argv);

        /* If we get here, exec failed */
        _exit(127);
    }

    /* Parent process */
    orbisjvm_process_t p = orbisjvm_malloc(sizeof(struct orbisjvm_process), ORBISJVM_MEM_ZERO);
    if (!p) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -ENOMEM;
    }

    p->pid = pid;
    p->detached = attr && (attr->flags & ORBISJVM_PROC_DETACHED);

    *process = p;
    orbisjvm_log_trace(ORBISJVM_LOG_CAT_GENERAL, "Created process: pid=%d", pid);
    return 0;
}

int orbisjvm_proc_wait(orbisjvm_process_t process, int* exit_code, int options) {
    if (!process) return -1;

    int status;
    int wopts = 0;
    if (options & ORBISJVM_PROC_WNOHANG) wopts |= WNOHANG;

    pid_t ret = waitpid(process->pid, &status, wopts);
    if (ret < 0) {
        return -errno;
    }

    if (ret == 0) return 0;  /* WNOHANG and process still running */

    if (exit_code) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = -1;
        }
    }

    return 0;
}

int orbisjvm_proc_trywait(orbisjvm_process_t process, int* exit_code) {
    return orbisjvm_proc_wait(process, exit_code, ORBISJVM_PROC_WNOHANG);
}

int orbisjvm_proc_get_exit_code(orbisjvm_process_t process, int* exit_code) {
    if (!process || !exit_code) return -1;

    int status;
    pid_t ret = waitpid(process->pid, &status, WNOHANG);
    if (ret < 0) return -errno;
    if (ret == 0) return -1;  /* Still running */

    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        *exit_code = 128 + WTERMSIG(status);
    } else {
        *exit_code = -1;
    }

    return 0;
}

uint32_t orbisjvm_proc_get_pid(orbisjvm_process_t process) {
    if (!process) return 0;
    return process->pid;
}

int orbisjvm_proc_terminate(orbisjvm_process_t process, int exit_code) {
    if (!process) return -1;

    int ret = kill(process->pid, SIGTERM);
    if (ret < 0) return -errno;

    /* Wait briefly for graceful shutdown */
    int status;
    waitpid(process->pid, &status, WNOHANG);

    (void)exit_code;  /* Exit code not directly usable with SIGTERM */
    return 0;
}

int orbisjvm_proc_kill(orbisjvm_process_t process) {
    if (!process) return -1;

    int ret = kill(process->pid, SIGKILL);
    if (ret < 0) return -errno;

    return 0;
}

int orbisjvm_proc_close(orbisjvm_process_t process) {
    if (!process) return -1;

    orbisjvm_free(process);
    return 0;
}

/* Execute and capture */

int orbisjvm_exec_capture(const char* cmd,
                          char* stdout_buf, size_t stdout_size,
                          char* stderr_buf, size_t stderr_size,
                          int* exit_code, int timeout_ms) {
    if (!cmd) return -1;

    /* Create pipes for stdout/stderr */
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};

    if (stdout_buf && pipe(stdout_pipe) < 0) return -errno;
    if (stderr_buf && pipe(stderr_pipe) < 0) {
        if (stdout_pipe[0] >= 0) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
        return -errno;
    }

    pid_t pid = fork();
    if (pid < 0) {
        int err = errno;
        if (stdout_pipe[0] >= 0) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
        if (stderr_pipe[0] >= 0) { close(stderr_pipe[0]); close(stderr_pipe[1]); }
        return -err;
    }

    if (pid == 0) {
        /* Child */
        if (stdout_pipe[1] >= 0) {
            dup2(stdout_pipe[1], STDOUT_FILENO);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
        }
        if (stderr_pipe[1] >= 0) {
            dup2(stderr_pipe[1], STDERR_FILENO);
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);
        }

        /* Execute via shell */
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(127);
    }

    /* Parent */
    if (stdout_pipe[1] >= 0) close(stdout_pipe[1]);
    if (stderr_pipe[1] >= 0) close(stderr_pipe[1]);

    /* Read output with timeout */
    fd_set read_fds;
    struct timeval tv;

    int max_fd = -1;
    if (stdout_pipe[0] >= 0) max_fd = stdout_pipe[0];
    if (stderr_pipe[0] >= 0 && stderr_pipe[0] > max_fd) max_fd = stderr_pipe[0];

    size_t stdout_read = 0;
    size_t stderr_read = 0;

    while ((stdout_pipe[0] >= 0 || stderr_pipe[0] >= 0) && (timeout_ms > 0 || timeout_ms == -1)) {
        FD_ZERO(&read_fds);
        if (stdout_pipe[0] >= 0) FD_SET(stdout_pipe[0], &read_fds);
        if (stderr_pipe[0] >= 0) FD_SET(stderr_pipe[0], &read_fds);

        if (timeout_ms > 0) {
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
        }

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, timeout_ms > 0 ? &tv : NULL);
        if (ret < 0) break;
        if (ret == 0) break;  /* Timeout */

        if (stdout_pipe[0] >= 0 && FD_ISSET(stdout_pipe[0], &read_fds)) {
            char buf[4096];
            ssize_t n = read(stdout_pipe[0], buf, sizeof(buf));
            if (n > 0 && stdout_buf && stdout_read + n < stdout_size) {
                memcpy(stdout_buf + stdout_read, buf, n);
                stdout_read += n;
            }
            if (n <= 0) {
                close(stdout_pipe[0]);
                stdout_pipe[0] = -1;
            }
        }

        if (stderr_pipe[0] >= 0 && FD_ISSET(stderr_pipe[0], &read_fds)) {
            char buf[4096];
            ssize_t n = read(stderr_pipe[0], buf, sizeof(buf));
            if (n > 0 && stderr_buf && stderr_read + n < stderr_size) {
                memcpy(stderr_buf + stderr_read, buf, n);
                stderr_read += n;
            }
            if (n <= 0) {
                close(stderr_pipe[0]);
                stderr_pipe[0] = -1;
            }
        }

        if (timeout_ms > 0) timeout_ms -= (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    }

    if (stdout_pipe[0] >= 0) close(stdout_pipe[0]);
    if (stderr_pipe[0] >= 0) close(stderr_pipe[0]);

    if (stdout_buf) stdout_buf[stdout_read] = '\0';
    if (stderr_buf) stderr_buf[stderr_read] = '\0';

    /* Wait for process */
    int status;
    waitpid(pid, &status, 0);

    if (exit_code) {
        if (WIFEXITED(status)) *exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
        else *exit_code = -1;
    }

    return 0;
}

int orbisjvm_exec_sync(const char* cmd, int timeout_ms) {
    int exit_code;
    return orbisjvm_exec_capture(cmd, NULL, 0, NULL, 0, &exit_code, timeout_ms);
}

/* CPU functions */

int orbisjvm_get_cpu_count(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int orbisjvm_get_cpu_affinity(uint64_t* mask, size_t mask_size) {
    if (!mask || mask_size < sizeof(cpu_set_t)) return -1;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int ret = sched_getaffinity(0, sizeof(cpuset), &cpuset);
    if (ret < 0) return -errno;

    memcpy(mask, &cpuset, sizeof(cpuset));
    return 0;
}

int orbisjvm_set_cpu_affinity(const uint64_t* mask, size_t mask_size) {
    if (!mask || mask_size < sizeof(cpu_set_t)) return -1;

    cpu_set_t cpuset;
    memcpy(&cpuset, mask, sizeof(cpuset));
    int ret = sched_setaffinity(0, sizeof(cpuset), &cpuset);
    if (ret < 0) return -errno;

    return 0;
}

/* Exit functions */

void orbisjvm_exit(int exit_code) {
    exit(exit_code);
}

void orbisjvm_abort(void) {
    abort();
}

/* Atexit */

int orbisjvm_atexit(void (*func)(void)) {
    if (!func) return -1;
    return atexit(func);
}

int orbisjvm_at_quick_exit(void (*func)(void)) {
    if (!func) return -1;
    return at_quick_exit(func);
}

void orbisjvm_quick_exit(int exit_code) {
    quick_exit(exit_code);
}

/* Signals */

orbisjvm_sighandler_t orbisjvm_signal(int signum, orbisjvm_sighandler_t handler) {
    struct sigaction sa, old;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(signum, &sa, &old) < 0) return SIG_ERR;
    return old.sa_handler;
}

int orbisjvm_raise(int signum) {
    return raise(signum);
}

/* Resource limits */

int orbisjvm_getrlimit(orbisjvm_rlimit_resource_t resource, orbisjvm_rlimit_t* limit) {
    if (!limit) return -1;

    int res;
    switch (resource) {
        case ORBISJVM_RLIMIT_CPU: res = RLIMIT_CPU; break;
        case ORBISJVM_RLIMIT_FSIZE: res = RLIMIT_FSIZE; break;
        case ORBISJVM_RLIMIT_DATA: res = RLIMIT_DATA; break;
        case ORBISJVM_RLIMIT_STACK: res = RLIMIT_STACK; break;
        case ORBISJVM_RLIMIT_CORE: res = RLIMIT_CORE; break;
        case ORBISJVM_RLIMIT_RSS: res = RLIMIT_RSS; break;
        case ORBISJVM_RLIMIT_NOFILE: res = RLIMIT_NOFILE; break;
        case ORBISJVM_RLIMIT_AS: res = RLIMIT_AS; break;
        case ORBISJVM_RLIMIT_NPROC: res = RLIMIT_NPROC; break;
        case ORBISJVM_RLIMIT_MEMLOCK: res = RLIMIT_MEMLOCK; break;
        default: return -EINVAL;
    }

    struct rlimit rl;
    int ret = getrlimit(res, &rl);
    if (ret < 0) return -errno;

    limit->rlim_cur = rl.rlim_cur;
    limit->rlim_max = rl.rlim_max;
    return 0;
}

int orbisjvm_setrlimit(orbisjvm_rlimit_resource_t resource, const orbisjvm_rlimit_t* limit) {
    if (!limit) return -1;

    int res;
    switch (resource) {
        case ORBISJVM_RLIMIT_CPU: res = RLIMIT_CPU; break;
        case ORBISJVM_RLIMIT_FSIZE: res = RLIMIT_FSIZE; break;
        case ORBISJVM_RLIMIT_DATA: res = RLIMIT_DATA; break;
        case ORBISJVM_RLIMIT_STACK: res = RLIMIT_STACK; break;
        case ORBISJVM_RLIMIT_CORE: res = RLIMIT_CORE; break;
        case ORBISJVM_RLIMIT_RSS: res = RLIMIT_RSS; break;
        case ORBISJVM_RLIMIT_NOFILE: res = RLIMIT_NOFILE; break;
        case ORBISJVM_RLIMIT_AS: res = RLIMIT_AS; break;
        case ORBISJVM_RLIMIT_NPROC: res = RLIMIT_NPROC; break;
        case ORBISJVM_RLIMIT_MEMLOCK: res = RLIMIT_MEMLOCK; break;
        default: return -EINVAL;
    }

    struct rlimit rl;
    rl.rlim_cur = limit->rlim_cur;
    rl.rlim_max = limit->rlim_max;

    int ret = setrlimit(res, &rl);
    if (ret < 0) return -errno;

    return 0;
}