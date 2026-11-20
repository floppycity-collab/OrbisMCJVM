# OrbisJVM Architecture

## Overview

OrbisJVM is a layered architecture providing a POSIX-like compatibility layer on top of Orbis OS, enabling JVM execution on PS4 hardware.

```
┌─────────────────────────────────────────────────────────────┐
│                    Java Applications                         │
│  (Minecraft Server, Java Apps, etc.)                        │
├─────────────────────────────────────────────────────────────┤
│                        JVM                                   │
│  (OpenJDK/HotSpot - to be integrated)                       │
├─────────────────────────────────────────────────────────────┤
│                      JNI Bridge                              │
│  (orbisjvm/jni.h - compatibility layer)                     │
├─────────────────────────────────────────────────────────────┤
│                   OrbisJVM Runtime                           │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐   │
│  │ Memory   │ Threading│ Filesystem│ Network  │  Time    │   │
│  ├──────────┼──────────┼──────────┼──────────┼──────────┤   │
│  │ Sync     │ Environ  │ Process  │ Security │ Diagnostics│   │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   OpenOrbis System Libraries                 │
│  (orbis2, orbisnet, orbisfios2, orbisrtc, muslc, libcxx)    │
├─────────────────────────────────────────────────────────────┤
│                        Orbis OS                              │
│                  (PS4 Kernel / System)                       │
└─────────────────────────────────────────────────────────────┘
```

## Module Responsibilities

### Runtime (`src/runtime/`)
- **Purpose**: Core initialization, version management, platform capability detection
- **Key APIs**: `orbisjvm_init()`, `orbisjvm_shutdown()`, `orbisjvm_get_platform_info()`
- **Dependencies**: All other modules
- **State**: Singleton initialization tracking

### Memory (`src/memory/`)
- **Purpose**: Memory allocation, virtual memory management, heap operations
- **Key APIs**: `orbisjvm_malloc()`, `orbisjvm_mmap()`, `orbisjvm_page_alloc()`, `orbisjvm_mprotect()`
- **Features**:
  - Standard allocation (malloc/free/calloc/realloc)
  - Aligned and page-aligned allocation
  - Memory protection (mprotect equivalent)
  - Memory mapping (mmap/munmap equivalent)
  - Virtual address space reservation/commit/decommit
  - Allocation tracking for diagnostics
- **Dependencies**: Orbis OS memory syscalls, musl libc

### Threading (`src/threading/`)
- **Purpose**: Thread management and basic synchronization
- **Key APIs**: 
  - Thread: `orbisjvm_thread_create()`, `orbisjvm_thread_join()`, `orbisjvm_thread_self()`
  - Mutex: `orbisjvm_mutex_create()`, `orbisjvm_mutex_lock()`, `orbisjvm_mutex_unlock()`
  - Condition variables: `orbisjvm_cond_create()`, `orbisjvm_cond_wait()`, `orbisjvm_cond_signal()`
  - TLS: `orbisjvm_tls_create()`, `orbisjvm_tls_get()`, `orbisjvm_tls_set()`
- **Dependencies**: pthreads (via muslc), Orbis OS thread APIs

### Filesystem (`src/filesystem/`)
- **Purpose**: File and directory operations using FIOS2
- **Key APIs**:
  - Files: `orbisjvm_file_open()`, `orbisjvm_file_read()`, `orbisjvm_file_write()`, `orbisjvm_file_seek()`
  - Directories: `orbisjvm_dir_create()`, `orbisjvm_dir_open()`, `orbisjvm_dir_read()`
  - Path utilities: `orbisjvm_path_join()`, `orbisjvm_path_basename()`, `orbisjvm_get_app_data_dir()`
- **Dependencies**: FIOS2 (orbisfios2), POSIX file APIs

### Networking (`src/networking/`)
- **Purpose**: TCP/UDP socket operations and DNS resolution
- **Key APIs**:
  - Sockets: `orbisjvm_socket()`, `orbisjvm_bind()`, `orbisjvm_listen()`, `orbisjvm_accept()`, `orbisjvm_connect()`
  - I/O: `orbisjvm_send()`, `orbisjvm_recv()`, `orbisjvm_sendto()`, `orbisjvm_recvfrom()`
  - Options: `orbisjvm_setsockopt()`, `orbisjvm_getsockopt()`
  - DNS: `orbisjvm_resolve()`, `orbisjvm_resolve_multiple()`
  - Polling: `orbisjvm_poll()`
- **Dependencies**: sceNet (orbisnet), POSIX socket APIs

### Synchronization (`src/synchronization/`)
- **Purpose**: Advanced synchronization primitives
- **Key APIs**:
  - Semaphores: `orbisjvm_sem_create()`, `orbisjvm_sem_wait()`, `orbisjvm_sem_post()`
  - RW Locks: `orbisjvm_rwlock_create()`, `orbisjvm_rwlock_rdlock()`, `orbisjvm_rwlock_wrlock()`
  - Barriers: `orbisjvm_barrier_create()`, `orbisjvm_barrier_wait()`
  - Spinlocks: `orbisjvm_spinlock_lock()`, `orbisjvm_spinlock_trylock()`
  - Atomics: `orbisjvm_atomic32_*()`, `orbisjvm_atomic64_*()`, `orbisjvm_atomic_ptr_*()`
- **Dependencies**: pthreads, C11 atomics

### Time (`src/time/`)
- **Purpose**: Time management and high-resolution timers
- **Key APIs**:
  - Clocks: `orbisjvm_clock_gettime()`, `orbisjvm_get_monotonic_ns()`, `orbisjvm_get_walltime_ns()`
  - Sleep: `orbisjvm_nanosleep()`, `orbisjvm_usleep()`, `orbisjvm_msleep()`
  - Timers: `orbisjvm_timer_create()`, `orbisjvm_timer_start()`, `orbisjvm_timer_stop()`
  - RTC: `orbisjvm_rtc_get_current()`, `orbisjvm_rtc_to_time_t()`
- **Dependencies**: sceRtc (orbisrtc), POSIX clocks

### Environment (`src/environment/`)
- **Purpose**: Environment variables, system information, configuration
- **Key APIs**:
  - Env vars: `orbisjvm_getenv()`, `orbisjvm_setenv()`, `orbisjvm_unsetenv()`
  - System info: `orbisjvm_get_system_info()`, `orbisjvm_get_cpu_info()`
  - Process info: `orbisjvm_get_process_info()`, `orbisjvm_getpid()`
  - Paths: `orbisjvm_get_app_paths()`, `orbisjvm_get_app_data_dir()`
  - Config: `orbisjvm_load_runtime_config()`, `orbisjvm_save_runtime_config()`
  - Random: `orbisjvm_getrandom()`, `orbisjvm_generate_uuid()`

### Process (`src/process/`)
- **Purpose**: Process creation and management
- **Key APIs**:
  - `orbisjvm_proc_create()`, `orbisjvm_proc_wait()`, `orbisjvm_proc_terminate()`
  - `orbisjvm_exec_capture()`, `orbisjvm_exec_sync()`
  - CPU affinity: `orbisjvm_get_cpu_affinity()`, `orbisjvm_set_cpu_affinity()`
  - Signals: `orbisjvm_signal()`, `orbisjvm_raise()`
  - Resource limits: `orbisjvm_getrlimit()`, `orbisjvm_setrlimit()`

### Security (`src/security/`)
- **Purpose**: Cryptographic operations and security utilities
- **Key APIs**:
  - Hashing: `orbisjvm_hash_create()`, `orbisjvm_hash_compute()` (MD5, SHA1, SHA2, SHA3)
  - HMAC: `orbisjvm_hmac_create()`, `orbisjvm_hmac_compute()`
  - Cipher: `orbisjvm_cipher_create()` (AES ECB/CBC/CTR/GCM, ChaCha20-Poly1305)
  - Random: `orbisjvm_secure_random()`, `orbisjvm_random_uuid()`
  - Secure memory: `orbisjvm_secure_zero()`, `orbisjvm_constant_time_eq()`
  - Path validation: `orbisjvm_path_is_safe()`, `orbisjvm_filename_valid()`

### JNI (`src/jni/`)
- **Purpose**: JNI compatibility bridge for JVM integration
- **Key APIs**:
  - Standard JNI function table (stub implementation)
  - OrbisJVM extensions: `orbisjvm_create_jvm()`, `orbisjvm_attach_thread()`, etc.
- **Status**: Stub implementation - requires actual JVM integration

### Diagnostics (`src/diagnostics/`)
- **Purpose**: Logging, profiling, crash reporting, diagnostics
- **Key APIs**:
  - Logging: `orbisjvm_log()`, `orbisjvm_diag_set_log_level()`, callbacks
  - Profiling: `orbisjvm_profile_start()`, `orbisjvm_profile_enter/exit()`
  - Crash: `orbisjvm_crash_register_handler()`, signal handlers
  - Memory: `orbisjvm_memory_snapshot()`, `orbisjvm_dump_allocations()`
  - System: `orbisjvm_generate_diagnostic_report()`

## API Design Principles

### Naming Conventions
- All public functions: `orbisjvm_<module>_<action>()`
- Types: `orbisjvm_<name>_t`
- Constants: `ORBISJVM_<MODULE>_<NAME>`
- Error codes: Negative errno values

### Error Handling
```c
// Success: 0
// Failure: negative errno (-EINVAL, -ENOMEM, -EPERM, etc.)
int ret = orbisjvm_function(...);
if (ret < 0) {
    // Handle error
    orbisjvm_log_error(ORBISJVM_LOG_CAT_MODULE, "Function failed: %d", ret);
}
```

### Thread Safety
- All APIs are thread-safe unless documented otherwise
- Internal mutexes protect shared state
- Functions marked as MT-Safe in documentation

### Memory Ownership
- Caller owns returned pointers unless documented
- `orbisjvm_malloc()` family: caller must `orbisjvm_free()`
- Output parameters: caller provides buffer, callee fills

### Versioning
- API version in `orbisjvm_version_info_t`
- Semantic versioning: MAJOR.MINOR.PATCH
- Breaking changes only in MAJOR version

## Data Flow

### Memory Allocation Flow
```
Application
    │
    ▼
orbisjvm_malloc(size, flags)
    │
    ├──► flags & ALIGNED ──► posix_memalign()
    │
    ├──► flags & PAGE_ALLOC ──► orbisjvm_page_alloc()
    │
    └──► default ──► malloc() (muslc)
    │
    ▼
Allocation Tracker (if enabled)
    │
    ▼
Return pointer
```

### Thread Creation Flow
```
Application
    │
    ▼
orbisjvm_thread_create(attr, func, arg)
    │
    ▼
Create thread struct
    │
    ▼
pthread_create(wrapper, thread_struct)
    │
    ▼
Wrapper: set name, call func(arg), cleanup
    │
    ▼
Return thread handle
```

### Network Operation Flow
```
Application
    │
    ▼
orbisjvm_socket()/connect()/send()/recv()
    │
    ▼
POSIX socket APIs (muslc)
    │
    ▼
sceNet (orbisnet) syscalls
    │
    ▼
Orbis OS Network Stack
    │
    ▼
Return result + update stats
```

## Integration Points

### JVM Integration (Future)
```
Java VM (HotSpot)
    │
    ▼
JNI Interface (standard)
    │
    ▼
OrbisJVM JNI Bridge (src/jni/)
    │
    ├──► orbisjvm_create_jvm() ──────► OrbisJVM Runtime
    ├──► orbisjvm_alloc_jvm_memory() ─► Memory Module
    ├──► orbisjvm_create_java_thread() ► Threading Module
    └──► orbisjvm_define_class() ─────► Filesystem Module
```

### OrbisMC Application (Future)
```
OrbisMC UI
    │
    ▼
Server Management API
    │
    ├──► Process Module (server start/stop)
    ├──► Networking Module (player connections)
    ├──► Filesystem Module (world management)
    ├──► Diagnostics Module (monitoring)
    └──► Environment Module (configuration)
```

## Platform-Specific Considerations

### Orbis OS Limitations
- No `fork()`/`exec()` in standard sense (limited process creation)
- No standard `mmap()` (custom implementation needed)
- Thread priorities differ from POSIX
- File permissions model differs
- No dynamic linking in standard sense (PRX modules)

### PS4 Hardware Constraints
- 8 CPU cores (Jaguar, 1.6 GHz)
- 8 GB GDDR5 unified memory
- Limited virtual address space for applications
- GPU not directly accessible from user mode
- Secure boot and memory encryption

### Memory Layout (Typical)
```
0x0000000000000000 - 0x00007FFFFFFFFFFF  User Space (128 TB)
0x0000800000000000 - 0xFFFFFFFFFFFFFFFF  Kernel Space
```

### Available Memory for Applications
- ~6 GB physical available for homebrew
- Virtual memory: 256 TB theoretical
- Page size: 4 KB (standard), 2 MB (large pages)

## Future Extensibility

### Planned Modules
- **Graphics**: GNM/GNMX integration for UI rendering
- **Audio**: AudioOut/AudioIn for sound
- **Input**: Pad/HID for controller support
- **Crypto**: Hardware-accelerated crypto (AES-NI, RDRAND)
- **JIT**: JIT compilation support for JVM

### Module Plugin System
Future versions may support dynamic module loading:
```c
// Hypothetical future API
orbisjvm_module_t* orbisjvm_module_load(const char* path);
orbisjvm_module_t* orbisjvm_module_get(const char* name);
```

## Testing Architecture

```
tests/
├── memory/
│   ├── test_alloc_basic.c
│   ├── test_page_alloc.c
│   ├── test_mmap.c
│   └── test_tracking.c
├── threading/
│   ├── test_thread_create.c
│   ├── test_mutex.c
│   ├── test_condvar.c
│   └── test_tls.c
├── filesystem/
│   ├── test_file_io.c
│   ├── test_dir_ops.c
│   └── test_path_utils.c
├── networking/
│   ├── test_tcp.c
│   ├── test_udp.c
│   ├── test_dns.c
│   └── test_poll.c
└── runtime/
    ├── test_init.c
    ├── test_capabilities.c
    └── test_config.c
```

Each test is a standalone executable linked against `orbisjvm` library.