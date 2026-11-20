# OrbisJVM - JVM Porting Strategy

## Executive Summary

This document outlines the strategy for porting a JVM (OpenJDK HotSpot) to run on Orbis OS using the OrbisJVM compatibility layer.

**Recommendation**: Port OpenJDK 17 (LTS) as primary target, with Java 21 as secondary target. HotSpot is the recommended JVM due to its performance, maturity, and existing x86-64 support.

---

## 1. JVM Components Analysis

### Required Components

| Component | Description | Porting Effort | Priority |
|-----------|-------------|----------------|----------|
| **HotSpot VM** | Core VM (interpreter, JIT, GC, threading) | High | Critical |
| **java.base** | Core modules (lang, io, nio, util, etc.) | Low (pure Java) | Critical |
| **java.net** | Networking APIs | Medium (native) | Critical |
| **java.nio** | NIO channels, selectors | Medium (native) | Critical |
| **java.security** | Crypto, SSL/TLS | Medium (native) | Critical |
| **jdk.unsupported** | Internal APIs | Low | High |
| **JNI** | Java Native Interface | Medium | Critical |
| **JVMTI** | Tool Interface | Low | Medium |
| **JDK Tools** | javac, jar, jlink, etc. | Low (host-only) | Low |

### HotSpot Architecture Layers

```
┌─────────────────────────────────────────┐
│         Java Application                │
├─────────────────────────────────────────┤
│         Java Class Libraries            │
│  (java.base, java.net, java.security)   │
├─────────────────────────────────────────┤
│           HotSpot VM                    │
│  ┌──────────┬──────────┬──────────┐     │
│  │Interpreter│  C1 JIT  │  C2 JIT  │     │
│  ├──────────┼──────────┼──────────┤     │
│  │    GC    │ Threading│   JNI    │     │
│  ├──────────┼──────────┼──────────┤     │
│  │  OS Port │  (os::)  │          │     │
│  └──────────┴──────────┴──────────┘     │
├─────────────────────────────────────────┤
│        OrbisJVM Compatibility Layer     │
├─────────────────────────────────────────┤
│            Orbis OS (PS4)               │
└─────────────────────────────────────────┘
```

---

## 2. Component Reusability Assessment

### Reusable (Minimal Changes)

| Component | Reason |
|-----------|--------|
| Java class libraries (java.base, etc.) | Pure Java bytecode, runs on any compliant JVM |
| Java compiler (javac) | Runs on host, produces class files |
| Class file format | Standard, platform-independent |
| JFR (Flight Recorder) | Mostly Java-side |
| JMX | Mostly Java-side |

### Needs Adaptation (Platform-Specific)

| Component | Required Changes |
|-----------|------------------|
| **os::malloc/free** | Map to `orbisjvm_malloc/free` |
| **os::map_memory** | Map to `orbisjvm_mmap` (no MAP_SHARED for files) |
| **os::protect_memory** | Map to `orbisjvm_mprotect` |
| **os::commit_memory** | Map to `orbisjvm_commit_address_space` |
| **Thread creation** | Map to `orbisjvm_thread_create` |
| **Mutex/CondVar** | Map to `orbisjvm_mutex/cond` |
| **TLS** | Map to `orbisjvm_tls_*` |
| **File I/O** | Map to `orbisjvm_file_*` |
| **Socket I/O** | Map to `orbisjvm_socket_*` |
| **Time** | Map to `orbisjvm_clock_gettime` |
| **Random** | Map to `orbisjvm_secure_random` |
| **Dynamic loading** | PRX modules instead of dlopen |
| **Signal handling** | Limited signal set |
| **Process info** | Map to `orbisjvm_getpid` etc. |

### New Implementation Required

| Component | Reason |
|-----------|--------|
| **Executable memory allocation** | JIT code cache needs RWX pages |
| **Large page support** | For GC heap optimization |
| **CPU feature detection** | Jaguar-specific features (AVX, FMA, AES-NI) |
| **Stack overflow handling** | Guard pages via mprotect |
| **Async I/O integration** | sceNetEpoll for NIO selectors |
| **Crash handler** | Signal handler for HS_ERROR logs |

---

## 3. Platform Abstraction Layer (os::)

### Key Files to Modify in HotSpot

```
hotspot/src/os/bsd/orbis/          ← NEW DIRECTORY
├── orbis_os.h                     ← Platform definitions
├── orbis_malloc.cpp               ← Memory allocation
├── orbis_memory.cpp               ← Virtual memory
├── orbis_thread.cpp               ← Threading
├── orbis_sync.cpp                 ← Synchronization
├── orbis_file.cpp                 ← File I/O
├── orbis_net.cpp                  ← Networking
├── orbis_time.cpp                 ← Time
├── orbis_dynlib.cpp               ← Dynamic loading (PRX)
├── orbis_signal.cpp               ← Signal handling
├── orbis_misc.cpp                 ← Miscellaneous
└── orbis_globals.cpp              ← Global state
```

### Build System Integration

```cmake
# In hotspot/cmake/hotspot.cmake
if(ORBIS)
    set(OS_NAME "orbis")
    set(OS_ARCH "x86_64")
    add_subdirectory(src/os/bsd/orbis)
    
    # Compiler flags
    target_compile_options(hotspot PRIVATE
        -DORBIS -D__ORBIS__ -D__PS4__
        -target x86_64-scei-ps4
    )
    
    # Link against OrbisJVM
    target_link_libraries(hotspot PRIVATE orbisjvm)
endif()
```

---

## 4. Technical Blockers & Solutions

### Blocker 1: Executable Memory for JIT

**Problem**: HotSpot JIT generates machine code at runtime requiring RWX pages.

**Orbis OS Constraint**: Restrictions on PROT_EXEC allocations.

**Solutions**:
1. Use `orbisjvm_mmap()` with `ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE | ORBISJVM_PROT_EXEC` and `ORBISJVM_MAP_ANONYMOUS`
2. Allocate RW, write code, then `orbisjvm_mprotect()` to RX
3. Reserve large executable region at startup
4. Use `ORBISJVM_MEM_EXECUTABLE` flag in `orbisjvm_malloc()`

**Code Cache Strategy**:
```cpp
// In orbis_memory.cpp
static void* g_code_cache_base = nullptr;
static size_t g_code_cache_size = 256 * 1024 * 1024; // 256 MB

void* os::allocate_code_memory(size_t size) {
    if (!g_code_cache_base) {
        g_code_cache_base = orbisjvm_mmap(nullptr, g_code_cache_size,
            ORBISJVM_PROT_READ | ORBISJVM_PROT_WRITE | ORBISJVM_PROT_EXEC,
            ORBISJVM_MAP_PRIVATE | ORBISJVM_MAP_ANONYMOUS, -1, 0);
    }
    // Bump pointer allocation within reserved region
}
```

### Blocker 2: Thread Stack Management

**Problem**: Java threads need configurable stack sizes with guard pages.

**Solution**:
```cpp
// In orbis_thread.cpp
int os::create_thread(Thread* thread, void* (*entry)(void*), void* arg) {
    orbisjvm_thread_attr_t attr;
    orbisjvm_thread_attr_init(&attr);
    orbisjvm_thread_attr_set_stack_size(&attr, thread->stack_size());
    orbisjvm_thread_attr_set_name(&attr, thread->name());
    
    orbisjvm_thread_t handle;
    int ret = orbisjvm_thread_create(&handle, &attr, entry, arg);
    return ret == 0 ? 0 : -1;
}
```

### Blocker 3: Dynamic Library Loading

**Problem**: HotSpot uses `dlopen`/`dlsym` for JNI libraries and `-agentlib`.

**Orbis OS**: Uses PRX modules, not ELF shared libraries.

**Solutions**:
1. **Static linking**: Link all native libraries into main executable
2. **PRX loading**: Implement `os::dll_load` using `sceKernelLoadModule`
3. **JNI registration**: Pre-register all native methods at startup

```cpp
// In orbis_dynlib.cpp
void* os::dll_load(const char* path, int flags) {
    // Convert to PRX if needed
    // Use sceKernelLoadModule for .sprx files
    // Return module handle
}

void* os::dll_lookup(void* handle, const char* name) {
    // Use sceKernelDlsym or equivalent
}
```

### Blocker 4: Large Page Support

**Problem**: GC performance benefits from 2MB large pages.

**Orbis OS**: Large page support needs investigation.

**Solution**:
```cpp
// In orbis_memory.cpp
bool os::use_large_pages(size_t size) {
    // Check if Orbis OS supports 2MB pages
    // Use ORBISJVM_MEM_LARGE_PAGE flag
    return orbisjvm_page_alloc(size / LARGE_PAGE_SIZE, ORBISJVM_MEM_LARGE_PAGE);
}
```

### Blocker 5: Signal Handling for Crash Dumps

**Problem**: HotSpot installs signal handlers for crash reporting (HS_ERROR).

**Orbis OS**: Limited signal support.

**Solution**:
```cpp
// In orbis_signal.cpp
void os::install_crash_handlers() {
    // Install handlers for SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
    // Use orbisjvm_crash_register_handler for integration
    // Generate minimal HS_ERROR compatible output
}
```

---

## 5. Recommended Implementation Approach

### Phase 1: Minimal Viable JVM (Weeks 1-4)

**Goal**: Run `java -version` and `java HelloWorld`

1. **Create Orbis OS port directory** in HotSpot
2. **Implement core os:: functions**:
   - Memory: malloc, free, map_memory, protect_memory
   - Threading: create_thread, sleep, yield
   - Synchronization: mutex, condvar
   - Time: clock_gettime, nanosleep
   - File: open, read, write, close, stat
   - Misc: getpid, getenv, abort
3. **Build HotSpot with zero JIT** (template interpreter only)
4. **Test with simple Java programs**

### Phase 2: Networking & File I/O (Weeks 5-8)

**Goal**: Run networked Java applications

1. **Implement socket I/O** in os::net
2. **Implement NIO selector** using sceNetEpoll
3. **Complete file I/O** (mmap for memory-mapped files)
4. **Test with java.net and java.nio**

### Phase 3: JIT Compilation (Weeks 9-16)

**Goal**: Enable C1/C2 JIT for performance

1. **Implement executable memory allocation** for code cache
2. **Port x86-64 C1 compiler** (template-based, easier)
3. **Port x86-64 C2 compiler** (more complex)
4. **Optimize for Jaguar CPU** (AVX, FMA, AES-NI)
5. **Test with SPECjvm2008**

### Phase 4: GC & Advanced Features (Weeks 17-24)

**Goal**: Production-ready JVM

1. **Tune GC for PS4 memory** (8GB total, ~6GB for app)
2. **Implement large page support** for heap
3. **Add JNI critical path optimizations**
4. **Implement JVMTI for profiling**
5. **SSL/TLS via OrbisJVM security**

### Phase 5: Minecraft Server Testing (Weeks 25-32)

**Goal**: Run Minecraft server

1. **Test with vanilla server JAR**
2. **Test with Paper/Fabric**
3. **Optimize for server workload**
4. **Long-running stability tests**

---

## 6. Alternative JVM Options

### Option A: OpenJDK HotSpot (Recommended)
- **Pros**: Best performance, mature, full JIT, all GCs, industry standard
- **Cons**: Large codebase, complex build, significant porting effort
- **Effort**: ~6-12 months for full port

### Option B: Eclipse OpenJ9
- **Pros**: Modular, smaller footprint, good for containers
- **Cons**: Less mature JIT, different architecture
- **Effort**: Similar to HotSpot

### Option C: GraalVM Native Image
- **Pros**: AOT compilation, no JIT needed, fast startup
- **Cons**: Limited dynamic features, reflection constraints
- **Effort**: Lower for static apps, but Minecraft uses reflection heavily

### Option D: MicroJVM (e.g., JamVM, Cacao)
- **Pros**: Small, simple, interpreter-only
- **Cons**: Slow, no JIT, limited GC
- **Effort**: Low, but unsuitable for Minecraft

### Option E: Custom Minimal JVM
- **Pros**: Designed for Orbis from scratch
- **Cons**: Enormous effort, reinventing wheel
- **Effort**: Years

**Decision**: OpenJDK HotSpot is the only viable option for running Minecraft Java Edition server with acceptable performance.

---

## 7. Build Configuration for OpenJDK

### JDK 17 Build Requirements

```bash
# Toolchain
clang 14+ / GCC 11+
lld
CMake 3.20+
Python 3.8+

# Dependencies (host)
freetype, cups, alsa, libffi

# Boot JDK
JDK 17 (for building JDK 17)
```

### Configure Options for PS4

```bash
bash configure \
    --target=x86_64-scei-ps4 \
    --openjdk-target=x86_64-scei-ps4 \
    --with-toolchain-path=/path/to/llvm \
    --with-sysroot=$OO_PS4_TOOLCHAIN \
    --with-extra-cflags="-target x86_64-scei-ps4 -fPIC" \
    --with-extra-cxxflags="-target x86_64-scei-ps4 -fPIC" \
    --with-extra-ldflags="-target x86_64-scei-ps4 -fuse-ld=lld" \
    --disable-jfr \
    --disable-dtrace \
    --with-native-debug-symbols=none \
    --with-debug-level=release \
    --with-jvm-variants=server \
    --with-jobs=8
```

### Custom HotSpot Build

```bash
# Build only HotSpot with custom os:: port
cd hotspot
cmake -DORBIS=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/OrbisToolchain.cmake ..
make -j8
```

---

## 8. Memory Configuration for PS4

### JVM Heap Sizing

| Configuration | Value | Rationale |
|---------------|-------|-----------|
| **Max Heap (-Xmx)** | 4 GB | Leave 2GB for OS, native, code cache |
| **Initial Heap (-Xms)** | 1 GB | Reasonable startup |
| **Code Cache** | 256 MB | JIT compilation space |
| **Metaspace** | 256 MB | Class metadata |
| **Compressed Oops** | Enabled | 32-bit refs up to 32GB heap |
| **Large Pages** | Enabled | If supported |

### GC Recommendations

| Workload | Recommended GC | Flags |
|----------|----------------|-------|
| **Minecraft Server** | G1GC | `-XX:+UseG1GC -XX:MaxGCPauseMillis=50` |
| **Low latency** | ZGC | `-XX:+UseZGC` (needs large pages) |
| **Throughput** | ParallelGC | `-XX:+UseParallelGC` |

---

## 9. Testing Strategy

### Unit Tests (Per Component)
```bash
# Memory
./test_orbis_malloc
./test_orbis_mmap

# Threading
./test_orbis_thread_create
./test_orbis_mutex

# Networking
./test_orbis_tcp_client_server
./test_orbis_dns_resolve
```

### Integration Tests
```bash
# JVM startup
./java -version
./java -Xint -version

# Class loading
./java HelloWorld

# JNI
./java -Djava.library.path=. JNITest

# Networking
./java NetworkTest

# File I/O
./java FileTest
```

### Minecraft Server Tests
```bash
# Vanilla
java -Xmx4G -jar server.jar nogui

# Paper
java -Xmx4G -jar paper.jar nogui

# Performance
./run_benchmark.sh
```

---

## 10. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| JIT executable memory blocked | High | Critical | Pre-reserve RWX region at startup |
| Thread stack guard pages fail | Medium | High | Manual stack checking in interpreter |
| sceNetEpoll not suitable for NIO | Medium | High | Fallback to poll()-based selector |
| GC performance on 6GB heap | Medium | Medium | Tune G1, consider ZGC with large pages |
| Dynamic library loading fails | High | High | Static link all native libs |
| OpenJDK build system complexity | High | Medium | Use pre-built HotSpot, custom CMake |
| PS4 kernel restrictions | Unknown | Critical | Test early on real hardware |

---

## 11. Resources & References

### OpenJDK Source
- https://github.com/openjdk/jdk17u
- https://github.com/openjdk/jdk21u

### HotSpot Porting Guides
- https://wiki.openjdk.org/display/HotSpot/Porting
- https://wiki.openjdk.org/display/HotSpot/OSPort

### Orbis OS Documentation
- OpenOrbis Wiki: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/wiki
- sceNet: https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/include/orbis
- sceRtc: RTC API documentation

### Related Projects
- **FreeBSD HotSpot port**: Reference for BSD-like OS port
- **Linux/x86_64 HotSpot**: Primary reference implementation
- **AArch64 HotSpot**: Cross-architecture port example

---

## 12. Next Steps

1. **Immediate**: Set up OpenJDK 17 build environment
2. **Week 1**: Create `hotspot/src/os/bsd/orbis/` directory structure
3. **Week 2**: Implement core `os::` functions (memory, thread, sync)
4. **Week 3**: Build HotSpot with interpreter only
5. **Week 4**: Test `java -version` on host (cross-compiled)
6. **Week 5**: Test on PS4 hardware via diagnostic tool
7. **Ongoing**: Iterate based on hardware testing results

---

*This document is a living specification. Update as implementation progresses and hardware testing reveals new constraints.*