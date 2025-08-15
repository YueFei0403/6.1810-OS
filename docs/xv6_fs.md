# MESI vs File System Write-back

## Top-down View
```
+---------------------------------------------+
| CPU Cores (L1/L2/L3 caches)                  |
|  - Hardware MESI keeps caches coherent       |
+---------------------------------------------+
              |  (MESI protocol: nanoseconds)
              v
+---------------------------------------------+
| Main Memory (RAM)                            |
|  - Shared program data                       |
|  - OS page cache lives here                  |
+---------------------------------------------+
              |  (OS manages dirty pages)
              v
+---------------------------------------------+
| OS Page Cache (in RAM)                       |
|  - File system write-back policy             |
+---------------------------------------------+
              |  (Flush to disk: milliseconds)
              v
+---------------------------------------------+
| Disk / SSD (Persistent Storage)              |
+---------------------------------------------+
```

## State Mapping Analogy

| MESI State  | FS Page State                                   |
|-------------|-------------------------------------------------|
| Modified    | Dirty (RAM ≠ Disk)                              |
| Exclusive   | Clean (only in RAM, matches Disk)               |
| Shared      | Shared/Unchanged (read-only)                    |
| Invalid     | Not present in page cache                       |

---

## File System Position in the Stack

**MESI States ≈ FS Page States**

- Modified → Dirty (RAM ≠ Disk)  
- Exclusive → Clean (only in RAM, matches Disk)  
- Shared → Shared/Unchanged (read-only)  
- Invalid → Not present in page cache  

```
[ User Programs ]          <-   open(), read(), write() calls
[ File System Code in OS ] <-   Implements file API, manages page cache
[ Virtual Memory System ]  <-   Maps RAM to files
[ Device Drivers ]         <-   Talks to disk controllers via CPU instructions
[ Hardware ]               <-   CPU, RAM, storage devices
```

*The FS logic is pure software running on the CPU.*

It uses hardware in two ways:  
1. **RAM** — stores page cache and metadata structures.  
2. **Disk controller hardware** — sends/receives blocks from persistent storage.  

---

### Simple Stack Diagram Showing Where MESI and the File System Live

```
+-----------------------------+
|   User Programs             |
|   -----------------------   |
|   open(), read(), write()   |
+-------------+---------------+
              |  System calls
+-------------v---------------+
|   File System (OS kernel)   |
|   - Implements file API     |
|   - Manages page cache in RAM|
|   - Write-back to disk      |
+-------------+---------------+
              |  Virtual memory + drivers
+-------------v---------------+
|   Device Drivers (OS kernel)|
|   - Disk controller access  |
|   - DMA setup               |
+-------------+---------------+
              |  I/O commands
+-------------v---------------+
|   Hardware                  |
|   - CPU cores + caches      |
|   - RAM (shared)            |
|   - Disk / SSD              |
+-------------+---------------+
              |  Cache coherence
+-------------v---------------+
|   MESI Protocol (hardware)  |
|   - Keeps CPU caches <-> RAM|
|     consistent              |
+-----------------------------+
```