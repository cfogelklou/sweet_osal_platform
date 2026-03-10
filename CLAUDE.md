# SOP (Sweet OS Platform) - AI Developer Guide

## Project Overview

**SOP (Sweet OS Platform)** is a lightweight, cross-platform OS abstraction layer with **20+ years of embedded heritage**. It provides:

- **OS Abstraction (OSAL)**: Threading, mutexes, timing, memory pools
- **Task Scheduler**: Run-to-completion state machine scheduler
- **Utilities**: Linked lists, byte queues, logging, CRC, UUID, file operations
- **BufIO**: Buffered I/O with FIFO queues
- **Networking**: Socket abstraction (mini_socket)
- **Crypto**: mbedTLS and libsodium integration
- **Cross-platform support**: Windows, Linux, macOS, Android, iOS, Free-RTOS, TI-RTOS, and WebAssembly

**License**: MIT License (Copyright 2020 Chris Fogelklou)

**Repository**: https://github.com/cfogelklou/sweet_osal_platform

---

## Directory Structure

```
sop/
├── sop_src/                    # Main SOP source code
│   ├── osal/                   # OS Abstraction Layer
│   │   ├── osal.h              # Main OSAL interface
│   │   ├── osal_mutex.*        # Mutex/lock primitives
│   │   ├── osal_thread.*       # Threading support
│   │   ├── osal_timer.*        # Timing/sleep functions
│   │   └── osal_memory.*       # Memory allocation
│   ├── utils/                  # Utility functions
│   │   ├── platform_log.h      # Logging framework
│   │   ├── byteq.*             # Byte queue implementation
│   │   ├── linked_list.*       # Linked list utilities
│   │   ├── crc.*               # CRC calculations
│   │   ├── uuid.*              # UUID generation
│   │   └── file_*.*            # File operations
│   ├── task_sched/             # Task scheduler
│   │   └── task_sched.*        # Run-to-completion state machine
│   ├── buf_io/                 # Buffered I/O
│   │   └── buf_io.*            # FIFO-queued buffered I/O
│   ├── mini_socket/            # Socket abstraction
│   │   └── mini_socket.*       # Cross-platform sockets
│   ├── simple_plot/            # Plotting utilities
│   │   └── simple_plot.*       # Data visualization
│   ├── mbedtls/                # Crypto integration
│   │   └── myconfig.h          # mbedTLS configuration
│   ├── libsodium/              # Sodium crypto wrapper
│   ├── LibraryFiles.cmake      # Central source file list
│   └── tests/                  # SOP-specific tests
├── test/                       # Integration tests
│   └── *.cpp                   # Main test files (sweet_osal_test)
├── ext/                        # External dependencies (git submodules)
│   ├── googletest/             # Google Test/Mock framework
│   ├── mbedtls/                # mbedTLS crypto library
│   ├── libsodium/              # Sodium cryptography
│   └── json/                   # nlohmann/json
├── build/                      # CMake build output
├── CMakeLists.txt              # Main build configuration
├── CLAUDE.md                   # This file
└── README.md                   # Project overview
```

---

## Quick Start: Building and Testing

### Native Build (macOS/Linux)

```bash
# Configure and build
cmake -B build
cmake --build build -j8

# Or using make directly
mkdir -p build && cd build
cmake ..
make -j8

# Run all tests
cd build
ctest -j8

# Run specific test
./sweet_osal_test
./osal_test
./utils_test
```

### Windows Build

```bash
mkdir build && cd build
cmake ..
msbuild sweet_osal_platform.sln /property:Configuration=Debug -maxcpucount:4
```

---

## Main Components

### 1. OSAL (OS Abstraction Layer)

**Location**: `sop_src/osal/`

**Purpose**: Cross-platform primitives for threading, synchronization, timing, and memory.

**Key Files**:
- `osal.h` - Main OSAL interface
- `osal_mutex.*` - Mutex/lock primitives
- `osal_thread.*` - Threading support
- `osal_timer.*` - Timing/sleep functions

**Key API Functions**:
```c
// Threading
osal_thread_t* osal_thread_create(void (*func)(void*), void* arg);
void osal_thread_join(osal_thread_t* thread);

// Mutexes
OSALMutexPtrT OSALCreateMutex(void);
void OSALDeleteMutex(OSALMutexPtrT* ppMutex);
bool OSALLockMutex(OSALMutexPtrT pMutex, uint32_t timeoutMs);
bool OSALUnlockMutex(OSALMutexPtrT pMutex);

// Timing
void OSALSleep(uint32_t milliseconds);
uint32_t OSALGetMS(void);  // Millisecond timer

// Memory
void* OSALMALLOC(size_t size);
void OSALFREE(void* ptr);

// Tasks
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void* pParam, OSALPrioT prio, const OSALTaskStructT* pPlatform);
```

### 2. Utils

**Location**: `sop_src/utils/`

**Purpose**: General-purpose utilities for logging, data structures, and file operations.

**Key Components**:
- **Logging**: `platform_log()` with severity levels
- **Byte Queue**: `byteq_t` circular buffer
- **Linked Lists**: Single and double-linked lists
- **CRC**: CRC-16 and CRC-32
- **UUID**: RFC 4122 UUID generation
- **File Operations**: Cross-platform file I/O

**Key Example**:
```cpp
#include "utils/platform_log.h"
#include "utils/byteq.h"

// Logging
LOG_Log("Processing %d items", count);

// Byte queue
ByteQ_t queue;
ByteQCreate(&queue, buffer, sizeof(buffer));
ByteQCommitWrite(&queue, len);
unsigned int read = ByteQRead(&queue, output, sizeof(output));
```

### 3. Task Scheduler

**Location**: `sop_src/task_sched/`

**Purpose**: Run-to-completion state machine scheduler for event-driven systems.

**Key Concept**: Tasks run in phases; each task completes before the next starts—no preemption within a phase.

### 4. BufIO

**Location**: `sop_src/buf_io/`

**Purpose**: Buffered I/O with FIFO queues for reliable data streaming.

### 5. MiniSocket

**Location**: `sop_src/mini_socket/`

**Purpose**: Cross-platform socket abstraction for TCP/UDP networking.

### 6. Crypto Integration

**mbedtls** (`sop_src/mbedtls/`): mbedTLS integration for TLS/SSL
**libsodium** (`sop_src/libsodium/`): Sodium cryptography wrapper

---

## Testing

### Available Tests

| Test | Location | Description |
|------|----------|-------------|
| `sweet_osal_test` | `test/` | Comprehensive OS abstraction tests |
| `osal_test` | `sop_src/osal/test/` | OSAL-specific tests |
| `utils_test` | `sop_src/utils/tests/` | Utility function tests |
| `test_simple_plot` | `sop_src/simple_plot/tests/` | Plotting tests |
| `transport_test` | `test/` | Transport layer tests |

### Running Tests

```bash
cd build

# Run all tests
ctest -j8

# Run specific test
./sweet_osal_test

# Run with verbose output
ctest -V -R osal_test

# Coverage report (non-CI)
make ctest_coverage
```

---

## Build System Details

### CMake Configuration

**Main CMakeLists.txt**:
- Project: `sweet_osal_platform`
- Minimum CMake: 3.5
- C++ Standard: C++11
- Enables testing and code coverage

**Key Files**:
- `sop_src/LibraryFiles.cmake` - Central SOP source file list
- `test/CMakeLists.txt` - Test executable configuration

### External Dependencies (bundled in `ext/`)

1. **Google Test** - C++ testing framework
2. **Google Mock** - C++ mocking framework
3. **mbedtls** - Crypto/TLS library
4. **libsodium** - Sodium cryptography
5. **nlohmann/json** - JSON parsing

---

## Common Workflows

### Fixing Linker Errors

If you see "undefined symbols" errors when building, check that required source modules are included in the test's CMakeLists.txt SOURCE_FILES list.

### Adding New Tests

1. Create test directory in appropriate location
2. Add `CMakeLists.txt` following existing patterns
3. Include `LibraryFiles.cmake` from SOP
4. Add `${GTEST_SRC}` and required module sources
5. Add subdirectory to appropriate parent CMakeLists.txt

### Updating Dependencies

All external dependencies are git submodules in `ext/`. Update with:
```bash
cd ext/<dependency>
git pull origin main/branch
```

---

## Platform-Specific Notes

### macOS
- Uses Clang from Xcode toolchain
- Threads::Threads for threading support
- Debug builds include `-fno-omit-frame-pointer`

### Linux
- POSIX threads (`pthread`)
- May need `-DMBEDTLS_NO_SOCKETS` for some configs

### Windows
- MSBuild for compilation
- Requires `ws2_32` (Windows Sockets)

### Free-RTOS / TI-RTOS
- Define `FREERTOS_PORT` for FreeRTOS build
- OSAL provides appropriate implementations

### Emscripten
- Build as part of parent SAU project
- Memory growth enabled for dynamic allocation

---

## Development Guidelines

**See [C_CODING_GUIDELINES.md](C_CODING_GUIDELINES.md)** for comprehensive C/C++ coding conventions including:

- Naming conventions (classes, functions, variables, constants)
- Code formatting (clang-format)
- Memory safety best practices (smart pointers, const correctness)
- Include ordering and header guards
- Comment style and documentation
- C++11/14 features
- Constant management

**Quick Summary**:
- **C++ Standard**: C++11 (C++14 features acceptable)
- **Naming**: Use existing patterns (`osal_` prefix for C API, `task_` for scheduler)
- **Error Handling**: Check return values, use `platform_log` for logging
- **Testing**: Add tests for new functionality
- **Documentation**: Update this file when making significant changes
- **Cross-Platform**: Use OSAL for platform-specific operations

### Keeping AI Instructions Synchronized

When changes are made to `CLAUDE.md`, run the sync script to propagate changes to other AI instruction files:

```bash
./scripts/copy-ai-instructions.sh
```

This script copies `CLAUDE.md` to:
- `GEMINI.md` - For Google Gemini
- `.github/copilot-instructions.md` - For GitHub Copilot

This ensures all AI tools have consistent context when working with this project.

---

## Ralph Loop for Autonomous Implementation

The `loop.sh` script enables autonomous implementation of Claude Code planning documents.

### The loops/ Directory

Ralph uses a `loops/` working directory to manage active plans:

```
loops/
├── plan-foo.md         # Working copy with checkboxes (tracked in git)
└── plan-bar.md         # Another working plan (tracked in git)
```

**Why loops/?**
- Plans without checkboxes are auto-converted using AI
- Original plans stay untouched; working copies go in `loops/`
- Multiple plans can be tracked simultaneously
- Plans are tracked in git so progress syncs across machines

### Usage

```bash
# Ingest a plan and start working on it
./loop.sh docs/plan-to-implement-feature.md

# Continue working on active plan (finds first with incomplete tasks)
./loop.sh

# Show status of all plans in loops/
./loop.sh status

# Verbose mode (see Claude output in real-time)
./loop.sh -v docs/plan-to-implement-feature.md

# Reset a plan (delete from loops/ to re-ingest fresh)
rm loops/plan-to-implement-feature.md
./loop.sh docs/plan-to-implement-feature.md
```

### How It Works

1. Reads the plan file for incomplete tasks (`- [ ]`)
2. Picks the next task and implements it
3. Runs build and tests to verify:
   ```bash
   cd build && cmake .. && make -j8
   ctest -j8
   ```
4. Updates the plan file (`- [x]`)
5. Commits and pushes changes
6. Loops until all tasks complete

### Plan File Format

Plans are markdown files with task checkboxes:

```markdown
## Phase 1: Setup
- [ ] Task 1
- [ ] Task 2

## Completed
- [x] Finished task
```

### Requirements

- `~/.zai.json` configured with z.ai credentials
- Build directory configured (`build/`)

See `ralph.md` for full documentation.
