# OrbisJVM - Minecraft Java Edition Server Support

## Overview

This document tracks Minecraft server compatibility with OrbisJVM, including version requirements, server software support, and testing status.

**Target**: Run Minecraft Java Edition dedicated servers natively on PS4 hardware via OrbisJVM.

---

## Minecraft Version Requirements

| Minecraft Version | Java Version | Release Date | Protocol Version | Status |
|-------------------|--------------|--------------|------------------|--------|
| 1.12.2 | Java 8 | 2017-09-18 | 340 | Legacy |
| 1.13.2 | Java 8 | 2018-10-22 | 404 | Legacy |
| 1.14.4 | Java 8 | 2019-07-19 | 498 | Legacy |
| 1.15.2 | Java 8 | 2020-01-15 | 578 | Legacy |
| 1.16.5 | Java 8 | 2021-01-15 | 754 | Legacy |
| 1.17.1 | Java 16 | 2021-07-06 | 756 | ⚠️ Java 16 required |
| 1.18.2 | Java 17 | 2022-02-28 | 757 | ✅ Target (LTS) |
| 1.19.4 | Java 17 | 2023-03-14 | 761 | ✅ Supported |
| 1.20.4 | Java 17 | 2023-12-07 | 765 | ✅ Supported |
| 1.21.x | Java 21 | 2024+ | 766+ | 🎯 Future Target |

**Recommendation**: Target **Java 17** (OpenJDK 17 LTS) as primary, supports 1.18.2 through 1.20.4. Java 21 for 1.21+.

---

## Server Software Compatibility

### Vanilla Server

| Version | Jar Size | Memory (Min) | Memory (Rec) | CPU Cores | Status |
|---------|----------|--------------|--------------|-----------|--------|
| 1.18.2 | ~50 MB | 1 GB | 2-4 GB | 2+ | 🎯 Primary Target |
| 1.19.4 | ~55 MB | 1 GB | 2-4 GB | 2+ | ✅ Supported |
| 1.20.4 | ~60 MB | 1 GB | 2-4 GB | 2+ | ✅ Supported |

**Vanilla Requirements**:
- Single-threaded main loop (tick processing)
- Network I/O on separate threads (Netty)
- File I/O for world saving (region files, level.dat)
- Logging (Log4j2)

### Paper (High-Performance Fork)

| Version | Based On | Key Optimizations | Status |
|---------|----------|-------------------|--------|
| 1.18.2 | Vanilla 1.18.2 | Async chunk loading, improved entity tracking, timings | 🎯 Primary Target |
| 1.19.4 | Vanilla 1.19.4 | Further optimizations, Folia support (multi-threaded) | ✅ Supported |
| 1.20.4 | Vanilla 1.20.4 | Current stable branch | ✅ Supported |

**Paper Advantages for PS4**:
- Better tick rate under load
- Async world saving
- Configurable performance options
- Plugin API compatibility (Bukkit/Spigot)
- **Folia**: Multi-threaded regions (experimental, good for 8-core PS4)

### Fabric (Modding Platform)

| Version | Loader | Mod Support | Status |
|---------|--------|-------------|--------|
| 1.18.2 | Fabric Loader 0.14+ | Client-side mods, some server mods | ⚠️ Limited Testing |
| 1.19.4 | Fabric Loader 0.15+ | Growing ecosystem | ⚠️ Limited Testing |
| 1.20.4 | Fabric Loader 0.16+ | Current | ⚠️ Limited Testing |

**Fabric Considerations**:
- Requires Fabric Loader (additional JAR)
- Mixin transformer (runtime bytecode manipulation)
- Some mods require client - not applicable for dedicated server

### Forge (Modding Platform)

| Version | Status | Notes |
|---------|--------|-------|
| 1.18.2 | ❌ Not Recommended | Heavy, client-focused, complex coremods |
| 1.19.2+ | ❌ Not Recommended | Major API changes, unstable |

**Forge Not Recommended**: Heavy runtime overhead, primarily for client modding.

### Other Server Software

| Software | Based On | Status | Notes |
|----------|----------|--------|-------|
| **Purpur** | Paper | ✅ Supported | Further optimizations, Pufferfish patches |
| **Airplane** | Paper | ⚠️ Experimental | Highly optimized, less tested |
| **Tuinity** | Paper | ⚠️ Archived | Merged into Paper |
| **Mohist** | Forge+Paper | ❌ Not Recommended | Hybrid, complex |
| **CatServer** | Forge+Spigot | ❌ Not Recommended | Hybrid, complex |

---

## Technical Requirements Analysis

### CPU Requirements

| Component | Vanilla | Paper | Notes |
|-----------|---------|-------|-------|
| **Main Thread** | 1 core | 1-2 cores | Tick processing, entity AI |
| **Netty IO** | 2-3 threads | 2-4 threads | Network I/O |
| **World Gen** | 1 thread | Async (Paper) | Chunk generation |
| **Garbage Collection** | 1-2 threads | 1-2 threads | G1GC default |
| **Total Recommended** | 2-4 cores | 4-6 cores | PS4 has 8 cores |

**PS4 Advantage**: 8 Jaguar cores at 1.6 GHz - sufficient for 20 TPS with 10-20 players.

### Memory Requirements

| Component | Vanilla (4GB) | Paper (4GB) | Notes |
|-----------|---------------|-------------|-------|
| **Heap** | 2-3 GB | 2-3 GB | -Xmx3G |
| **Metaspace** | 256 MB | 256 MB | Class metadata |
| **Code Cache** | 256 MB | 256 MB | JIT |
| **Direct Memory** | 256 MB | 512 MB | NIO buffers |
| **Thread Stacks** | ~100 MB | ~150 MB | 1MB per thread |
| **OS/Overhead** | ~500 MB | ~500 MB | Orbis OS + native |
| **Total** | **~3.5 GB** | **~4 GB** | Within 6GB available |

**Recommended Flags**:
```bash
java -Xmx3G -Xms1G \
  -XX:+UseG1GC \
  -XX:MaxGCPauseMillis=50 \
  -XX:+ParallelRefProcEnabled \
  -XX:+UnlockExperimentalVMOptions \
  -XX:+AlwaysPreTouch \
  -jar paper.jar nogui
```

### Network Requirements

| Requirement | Vanilla | Paper | Notes |
|-------------|---------|-------|-------|
| **Default Port** | 25565 | 25565 | TCP |
| **Query Port** | 25565 | 25565 | UDP (optional) |
| **Max Players** | 20 | 50+ | Configurable |
| **View Distance** | 10 chunks | 10 chunks | Affects bandwidth |
| **Simulation Distance** | 10 chunks | 10 chunks | Entity processing |
| **Network Compression** | 256 bytes | 256 bytes | Threshold |

**PS4 Network**: Gigabit Ethernet - sufficient for 20+ players.

### Storage Requirements

| Component | Size | Growth | Notes |
|-----------|------|--------|-------|
| **World (Overworld)** | 500 MB - 5 GB | +100 MB/hour | Region files |
| **World (Nether)** | 100 MB - 1 GB | +50 MB/hour | |
| **World (End)** | 50 MB - 500 MB | +10 MB/hour | |
| **Playerdata** | 1 MB/player | | |
| **Logs** | 10-100 MB | | Rotate daily |
| **Backups** | World size × count | | Compressed |

**PS4 Storage**: Internal HDD (500GB/1TB) or external USB - sufficient.

---

## Java Feature Usage in Minecraft

### Java 8 Features (1.12-1.16)
- Lambdas, Streams, Optional
- Default interface methods
- Date/Time API (java.time)
- Nashorn JS (removed in 15)

### Java 16+ Features (1.17+)
- Records (data classes)
- Pattern matching (instanceof)
- Sealed classes
- Text blocks
- Vector API (incubator)

### Java 17 Features (1.18+)
- Sealed classes (standard)
- Pattern matching for switch (preview)
- Foreign Function & Memory API (incubator) - **Critical for JNI**

### Java 21 Features (1.21+)
- Virtual threads (Project Loom) - **Game changer for servers**
- Structured concurrency
- Pattern matching for switch (standard)
- String templates (preview)

---

## OrbisJVM Implementation Priorities

### Phase 1: Vanilla 1.18.2 (Minimum Viable)

| Feature | OrbisJVM Module | Status |
|---------|-----------------|--------|
| JVM Startup | Runtime, JNI | 🎯 Critical |
| Class Loading | JNI, Filesystem | 🎯 Critical |
| Main Thread | Threading | 🎯 Critical |
| Netty Networking | Networking (TCP) | 🎯 Critical |
| World I/O | Filesystem | 🎯 Critical |
| Logging | Diagnostics | 🎯 Critical |
| G1GC | Memory, Synchronization | 🎯 Critical |
| JIT (C1/C2) | JNI Extensions | 🔴 High |

### Phase 2: Paper 1.18.2 (Production)

| Feature | Additional Requirements | Status |
|---------|-------------------------|--------|
| Async Chunk Loading | Thread pool, CompletableFuture | 🔴 High |
| Folia (Multi-threaded) | Virtual threads (Java 21) | 🟡 Future |
| Timings/Metrics | JMX, MBeans | 🟡 Medium |
| Plugin API | Bukkit/Spigot API | 🟡 Medium |

### Phase 3: Modern Versions (1.19-1.20)

| Feature | Java Version | Status |
|---------|--------------|--------|
| Records/Pattern Matching | Java 17 | ✅ Supported |
| Foreign Function API | Java 17+ | 🔴 High (JNI replacement) |
| Virtual Threads | Java 21 | 🟡 Future |

---

## Configuration Templates

### Vanilla Server (server.properties)

```properties
# Minecraft server properties
enable-jmx-monitoring=false
rcon.port=25575
level-seed=
gamemode=survival
enable-command-block=false
enable-query=false
generator-settings={}
enforce-secure-profile=true
level-name=world
motd=OrbisMC PS4 Server
query.port=25565
pvp=true
generate-structures=true
difficulty=easy
network-compression-threshold=256
max-tick-time=60000
require-resource-pack=false
use-native-transport=true
max-players=20
online-mode=true
enable-status=true
allow-flight=false
initial-disabled-packs=
broadcast-rcon-to-ops=true
view-distance=10
server-ip=
resource-pack-prompt=
allow-nether=true
server-port=25565
enable-rcon=false
sync-chunk-writes=true
op-permission-level=4
prevent-proxy-connections=false
resource-pack=
entity-broadcast-range-percentage=100
simulation-distance=10
rcon.password=
player-idle-timeout=0
debug=false
force-gamemode=false
rate-limit=0
hardcore=false
white-list=false
broadcast-console-to-ops=true
spawn-npcs=true
spawn-animals=true
function-permission-level=2
level-type=minecraft\:normal
spawn-monsters=true
enforce-whitelist=false
spawn-protection=16
resource-pack-sha1=
max-world-size=29999984
```

### Paper Server (paper.yml - Key Settings)

```yaml
# paper.yml - Performance settings for PS4
verbose: false
config-version: 12

settings:
  # Chunk loading
  async-chunks:
    enabled: true
    threads: 2
  
  # Entity optimization
  entity-per-chunk-save-limit:
    experience_orb: 16
  
  # Network
  network-compression-threshold: 256
  use-alternative-keepalive: true
  
  # World saving
  autosave-interval: 6000
  use-faster-eigencraft-redstone: true
  
  # Timings
  timings:
    enabled: true
    verbose: false
    server-name-privacy: false
    hidden-config-entries: []
    history-length: 300
    history-interval: 300

messages:
  kick:
    authentication-servers-down: "Cannot connect to authentication servers. Try again later."
    flying-player: "Flying is not enabled on this server"
    flying-vehicle: "Flying is not enabled on this server"

world-settings:
  default:
    # Optimization
    optimize-explosions: true
    disable-chest-cat-detection: true
    disable-creeper-lingering-effect: true
    
    # Mobs
    max-auto-save-chunks-per-tick: 8
    prevent-moving-into-unloaded-chunks: true
    
    # Containers
    hopper:
      disable-move-event: true
      cooldown-when-full: true
      push-based: false
    
    # Redstone
    redstone-implementation: "ALTERNATE_CURRENT"
```

### Java Flags for PS4

```bash
# Recommended Java flags for PS4 (4GB heap)
JAVA_FLAGS="
  -Xmx3G -Xms1G
  -XX:+UseG1GC
  -XX:MaxGCPauseMillis=50
  -XX:+ParallelRefProcEnabled
  -XX:+UnlockExperimentalVMOptions
  -XX:+AlwaysPreTouch
  -XX:G1HeapRegionSize=16M
  -XX:G1NewSizePercent=30
  -XX:G1MaxNewSizePercent=40
  -XX:G1HeapWastePercent=5
  -XX:G1MixedGCCountTarget=4
  -XX:InitiatingHeapOccupancyPercent=45
  -XX:G1MixedGCLiveThresholdPercent=90
  -XX:G1RSetUpdatingPauseTimePercent=5
  -XX:SurvivorRatio=32
  -XX:+PerfDisableSharedMem
  -XX:MaxTenuringThreshold=1
  -Dusing.aikars.flags=https://mcflags.emc.gs
  -Daikars.new.flags=true
"
```

---

## Testing Checklist

### Functional Testing

| Test | Vanilla | Paper | Status |
|------|---------|-------|--------|
| Server starts | ✅ | ✅ | ⏳ |
| Player joins | ✅ | ✅ | ⏳ |
| Player moves | ✅ | ✅ | ⏳ |
| Block break/place | ✅ | ✅ | ⏳ |
| Chat works | ✅ | ✅ | ⏳ |
| Commands work | ✅ | ✅ | ⏳ |
| World saves | ✅ | ✅ | ⏳ |
| Server stops gracefully | ✅ | ✅ | ⏳ |

### Performance Testing

| Metric | Target | Measurement |
|--------|--------|-------------|
| TPS (Ticks Per Second) | 20.0 | /tps command |
| Memory Usage | < 3.5 GB | JVM metrics |
| GC Pause (max) | < 100 ms | GC logs |
| CPU Usage | < 80% | top/htop |
| Network Latency | < 50 ms | ping |
| Player Capacity | 20+ | Stress test |
| Uptime | 24+ hours | Stability test |

### Stability Testing

| Test | Duration | Success Criteria |
|------|----------|------------------|
| Idle server | 24 hours | No crashes, memory stable |
| 5 players active | 4 hours | TPS > 19.5 |
| 10 players active | 2 hours | TPS > 19.0 |
| 20 players active | 1 hour | TPS > 18.0 |
| Chunk generation | 30 min | No lag spikes |
| World border | 1 hour | No crashes |
| Restart cycle | 10 cycles | Clean startup/shutdown |

---

## Known Issues & Workarounds

| Issue | Cause | Workaround |
|-------|-------|------------|
| **JIT Compilation Crashes** | Executable memory restrictions | Use `-Xint` (interpreter only) initially |
| **Netty Native Transport** | epoll not available | Use `-Dio.netty.transport.noNative=true` |
| **File Locking** | FIOS2 limitations | Disable `level-lock` in server.properties |
| **DNS Resolution** | sceNetResolver async | Increase connection timeout |
| **Large World Generation** | CPU/memory pressure | Pre-generate world, use Paper |
| **Plugin Classloading** | Custom classloaders | Test plugins individually |

---

## Future Roadmap

### Short Term (0-3 months)
- [ ] Vanilla 1.18.2 server startup
- [ ] Player connection and basic gameplay
- [ ] World save/load
- [ ] 1-hour stability test

### Medium Term (3-6 months)
- [ ] Paper 1.18.2 with async chunks
- [ ] 20 player stress test
- [ ] 24-hour uptime
- [ ] Plugin support (Essentials, WorldEdit)

### Long Term (6-12 months)
- [ ] Java 21 + Virtual Threads (Project Loom)
- [ ] Folia multi-threaded regions
- [ ] 1.20.4 / 1.21 support
- [ ] Modded server (Fabric)
- [ ] OrbisMC management UI

---

## Resources

### Server Downloads
- **Vanilla**: https://www.minecraft.net/en-us/download/server
- **Paper**: https://papermc.io/downloads
- **Purpur**: https://purpurmc.org/downloads
- **Fabric**: https://fabricmc.net/use/server/

### Documentation
- **Minecraft Wiki**: https://minecraft.fandom.com/wiki/Server
- **Paper Docs**: https://docs.papermc.io/
- **Minecraft Protocol**: https://wiki.vg/Protocol

### Performance Guides
- **Aikar's Flags**: https://mcflags.emc.gs/
- **Paper Tuning**: https://docs.papermc.io/paper/performance
- **JVM Tuning**: https://docs.oracle.com/en/java/javase/17/gctuning/

---

*Last Updated: 2026-11-20 | OrbisJVM v0.1.0*