# OrbisJVM Compatibility Matrix

## Overview

This document tracks the compatibility of OrbisJVM APIs with POSIX, Linux, and Orbis OS native APIs.

## API Compatibility Status

### Memory Management

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_malloc()` | `malloc()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_free()` | `free()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_calloc()` | `calloc()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_realloc()` | `realloc()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_aligned_alloc()` | `aligned_alloc()` / `posix_memalign()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_page_alloc()` | `mmap()` with MAP_ANONYMOUS | ✅ Full | ✅ Custom | ✅ Implemented |
| `orbisjvm_page_free()` | `munmap()` | ✅ Full | ✅ Custom | ✅ Implemented |
| `orbisjvm_mprotect()` | `mprotect()` | ✅ Full | ⚠️ Limited | ✅ Implemented |
| `orbisjvm_mmap()` | `mmap()` | ✅ Full | ⚠️ Partial | ✅ Implemented |
| `orbisjvm_munmap()` | `munmap()` | ✅ Full | ⚠️ Partial | ✅ Implemented |
| `orbisjvm_get_page_size()` | `sysconf(_SC_PAGESIZE)` | ✅ Full | ✅ Via muslc | ✅ Implemented |

**Notes:**
- Orbis OS has limited virtual memory management
- `mprotect`/`mmap` may have restrictions on executable pages
- Large page support (2MB) needs investigation

### Threading

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_thread_create()` | `pthread_create()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_thread_join()` | `pthread_join()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_thread_detach()` | `pthread_detach()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_thread_self()` | `pthread_self()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_thread_equal()` | `pthread_equal()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_thread_exit()` | `pthread_exit()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_mutex_create()` | `pthread_mutex_init()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_mutex_lock()` | `pthread_mutex_lock()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_mutex_trylock()` | `pthread_mutex_trylock()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_mutex_unlock()` | `pthread_mutex_unlock()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_cond_create()` | `pthread_cond_init()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_cond_wait()` | `pthread_cond_wait()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_cond_timedwait()` | `pthread_cond_timedwait()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_cond_signal()` | `pthread_cond_signal()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_cond_broadcast()` | `pthread_cond_broadcast()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_tls_create()` | `pthread_key_create()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_tls_get/set()` | `pthread_get/setspecific()` | ✅ Full | ✅ Via muslc | ✅ Implemented |

**Notes:**
- Recursive mutexes supported via `PTHREAD_MUTEX_RECURSIVE`
- Thread priorities: Orbis OS uses different range (-20 to 19 mapped)
- Thread naming: `pthread_setname_np()` available on PS4

### Filesystem

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_file_open()` | `open()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_close()` | `close()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_read()` | `read()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_write()` | `write()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_pread()` | `pread()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_pwrite()` | `pwrite()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_seek()` | `lseek()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_sync()` | `fsync()` / `fdatasync()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_truncate()` | `ftruncate()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_stat()` | `fstat()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_dir_create()` | `mkdir()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_dir_open()` | `opendir()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_dir_read()` | `readdir()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_dir_close()` | `closedir()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_remove()` | `unlink()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_file_rename()` | `rename()` | ✅ Full | ✅ Via FIOS2 | ✅ Implemented |
| `orbisjvm_path_realpath()` | `realpath()` | ✅ Full | ✅ Via muslc | ✅ Implemented |

**Notes:**
- FIOS2 is the native PS4 filesystem API
- Path format: `/user/home/<user_id>/...` for user data
- No symlink support in FIOS2 (read-only filesystem mostly)
- Case-sensitive filesystem

### Networking

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_socket()` | `socket()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_close()` | `close()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_bind()` | `bind()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_listen()` | `listen()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_accept()` | `accept()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_connect()` | `connect()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_send()` | `send()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_recv()` | `recv()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_sendto()` | `sendto()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_recvfrom()` | `recvfrom()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_sendmsg()` | `sendmsg()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_recvmsg()` | `recvmsg()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_shutdown()` | `shutdown()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_setsockopt()` | `setsockopt()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_getsockopt()` | `getsockopt()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_getsockname()` | `getsockname()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_getpeername()` | `getpeername()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_inet_pton()` | `inet_pton()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_inet_ntop()` | `inet_ntop()` | ✅ Full | ✅ sceNet | ✅ Implemented |
| `orbisjvm_resolve()` | `getaddrinfo()` | ✅ Full | ✅ sceNetResolver | ✅ Implemented |
| `orbisjvm_poll()` | `poll()` | ✅ Full | ⚠️ sceNetEpoll | ✅ Implemented |

**Notes:**
- sceNet uses different socket ID type (int32_t vs int)
- DNS resolution via `sceNetResolver*` APIs
- `sceNetEpoll` available for high-performance I/O
- TCP_NODELAY, SO_REUSEADDR, SO_KEEPALIVE supported
- IPv6: Limited support in sceNet

### Time

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_clock_gettime()` | `clock_gettime()` | ✅ Full | ✅ sceRtc | ✅ Implemented |
| `orbisjvm_gettimeofday()` | `gettimeofday()` | ✅ Full | ✅ sceRtc | ✅ Implemented |
| `orbisjvm_nanosleep()` | `nanosleep()` | ✅ Full | ✅ sceKernel | ✅ Implemented |
| `orbisjvm_usleep()` | `usleep()` | ✅ Full | ✅ sceKernel | ✅ Implemented |
| `orbisjvm_timer_create()` | `timer_create()` | ✅ Full | ⚠️ Custom | ✅ Implemented |
| `orbisjvm_rtc_get_current()` | N/A | ❌ N/A | ✅ sceRtc | ✅ Implemented |

**Notes:**
- Monotonic clock: Available via `CLOCK_MONOTONIC`
- High-resolution timers: `timer_create()` with `CLOCK_MONOTONIC`
- RTC: `sceRtcGetCurrentClockLocalTime()` for wall clock

### Synchronization

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_sem_create()` | `sem_init()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_sem_wait()` | `sem_wait()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_sem_post()` | `sem_post()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_rwlock_create()` | `pthread_rwlock_init()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_barrier_create()` | `pthread_barrier_init()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_spinlock_*()` | Custom | ✅ Via atomics | ✅ Via atomics | ✅ Implemented |
| `orbisjvm_atomic_*()` | C11 atomics | ✅ Full | ✅ Clang | ✅ Implemented |

### Environment/Process

| OrbisJVM API | POSIX Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|------------------|---------------|-----------------|--------|
| `orbisjvm_getenv()` | `getenv()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_setenv()` | `setenv()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_getpid()` | `getpid()` | ✅ Full | ✅ Via muslc | ✅ Implemented |
| `orbisjvm_fork()` | `fork()` | ✅ Full | ❌ Not available | ❌ Not implemented |
| `orbisjvm_exec()` | `execve()` | ✅ Full | ❌ Not available | ❌ Not implemented |
| `orbisjvm_waitpid()` | `waitpid()` | ✅ Full | ⚠️ Limited | ⚠️ Partial |

**Notes:**
- Process creation (`fork`/`exec`) is NOT available on Orbis OS
- `orbisjvm_proc_create()` uses `fork`/`exec` on host, but would need different approach on PS4
- Orbis OS uses application-based model, not process-based

### Security

| OrbisJVM API | OpenSSL Equivalent | Linux Support | Orbis OS Native | Status |
|--------------|-------------------|---------------|-----------------|--------|
| `orbisjvm_hash_*()` | `EVP_Digest*()` | ✅ Full | ✅ OpenSSL port | ✅ Implemented |
| `orbisjvm_hmac_*()` | `HMAC_*()` | ✅ Full | ✅ OpenSSL port | ✅ Implemented |
| `orbisjvm_cipher_*()` | `EVP_Cipher*()` | ✅ Full | ✅ OpenSSL port | ✅ Implemented |
| `orbisjvm_secure_random()` | `RAND_bytes()` | ✅ Full | ✅ OpenSSL port | ✅ Implemented |
| `orbisjvm_secure_zero()` | `OPENSSL_cleanse()` | ✅ Full | ✅ OpenSSL port | ✅ Implemented |

**Notes:**
- OpenSSL must be ported to PS4 (available in OpenOrbis ecosystem)
- Hardware AES-NI available on Jaguar CPU
- RDRAND available for entropy

## Unsupported / Limited APIs

### Not Available on Orbis OS

| API Category | Functions | Reason |
|--------------|-----------|--------|
| Process Creation | `fork()`, `vfork()`, `exec*()`, `posix_spawn()` | Orbis OS doesn't support traditional process model |
| Signals (Full) | `sigaction()`, `sigprocmask()`, `signalfd()` | Limited signal support |
| Dynamic Linking | `dlopen()`, `dlsym()`, `dlclose()` | PRX modules instead |
| Syslog | `syslog()`, `openlog()` | Custom logging needed |
| PTY | `posix_openpt()`, `grantpt()`, `unlockpt()` | No pseudo-terminals |
| Inotify | `inotify_init()`, `inotify_add_watch()` | No filesystem notifications |
| Epoll (native) | `epoll_create()`, `epoll_ctl()`, `epoll_wait()` | sceNetEpoll is different API |
| Shared Memory | `shm_open()`, `shm_unlink()` | Different memory sharing model |
| POSIX MQ | `mq_open()`, `mq_send()`, `mq_receive()` | Not available |

### Partially Available

| API | Limitation |
|-----|------------|
| `mmap()` | No MAP_SHARED for file-backed, limited PROT_EXEC |
| `pthread` | Thread priorities different range, no pthread_cancel |
| `getaddrinfo()` | Must use sceNetResolver instead |
| `stat()` | Some fields not meaningful (uid/gid, timestamps) |
| `chmod()` | Limited permission model |
| `utime()` | Timestamp modification limited |

## Java/JVM Compatibility

### Target JVM: OpenJDK 17/21

| Component | Status | Notes |
|-----------|--------|-------|
| HotSpot C1/C2 JIT | ⚠️ Planned | Requires x86-64 code generation |
| Interpreter | ✅ Required | Template interpreter for x86-64 |
| Garbage Collectors | ✅ Required | G1, ZGC, Shenandoah |
| Class Loading | ✅ Required | Standard classloader hierarchy |
| JNI | ✅ Required | Full JNI 1.8+ support |
| java.lang | ✅ Required | Core classes |
| java.io | ✅ Required | File I/O via OrbisJVM filesystem |
| java.net | ✅ Required | Networking via OrbisJVM networking |
| java.nio | ✅ Required | Channels, buffers, selectors |
| java.util.concurrent | ✅ Required | Threading via OrbisJVM threading |
| javax.crypto | ✅ Required | Security via OrbisJVM security |
| JMX | ⚠️ Optional | Management/monitoring |

### HotSpot Platform Port Requirements

| Component | Effort | Status |
|-----------|--------|--------|
| os::malloc/free | Low | Map to orbisjvm_malloc/free |
| os::map_memory | Medium | Map to orbisjvm_mmap |
| os::protect_memory | Medium | Map to orbisjvm_mprotect |
| Thread creation | Low | Map to orbisjvm_thread_create |
| Mutex/condvar | Low | Map to orbisjvm_mutex/cond |
| TLS | Low | Map to orbisjvm_tls |
| File I/O | Medium | Map to orbisjvm_file_* |
| Socket I/O | Medium | Map to orbisjvm_socket_* |
| Time | Low | Map to orbisjvm_time_* |
| Random | Low | Map to orbisjvm_secure_random |
| Dynamic loading | High | PRX modules, not dlopen |

## Testing Status

| Module | Host Tests | PS4 Simulator | PS4 Hardware |
|--------|------------|---------------|--------------|
| Memory | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Threading | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Filesystem | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Networking | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Time | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Synchronization | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Security | ⏳ Planned | ⏳ Planned | ⏳ Planned |
| Full Runtime | ⏳ Planned | ⏳ Planned | ⏳ Planned |

## Known Issues

1. **Memory Mapping**: PS4 has restricted executable memory allocation - JIT needs special handling
2. **Thread Stack**: Default stack size may need adjustment for Java threads
3. **File Descriptors**: Limited number available per application
4. **DNS**: sceNetResolver is asynchronous, different from blocking getaddrinfo
5. **Clock Sources**: Only monotonic and realtime clocks available
6. **Signal Handling**: Only SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS supported

## Future Compatibility Targets

- **Java 21**: Virtual threads, structured concurrency
- **GraalVM**: Native image support
- **WASM**: WebAssembly runtime integration
- **Containers**: Lightweight isolation for multiple servers