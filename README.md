# Sweet OS Platform (SOP)

[![CI or PR](https://github.com/cfogelklou/sweet_osal_platform/actions/workflows/ci_pr.yml/badge.svg?branch=master)](https://github.com/cfogelklou/sweet_osal_platform/actions/workflows/ci_pr.yml)

<a href="https://scan.coverity.com/projects/cfogelklou-sweet_osal_platform">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/22874/badge.svg"/>
</a>

An easily portable abstraction layer for Windows, Linux, macOS, Free-RTOS, Android, iOS, and WebAssembly platforms containing OS abstractions and utilities that have been proven useful over my career.

**Repository**: https://github.com/cfogelklou/sweet_osal_platform

**License**: MIT License (Copyright 2020 Chris Fogelklou)

---

## Overview

Sweet OS Platform (SOP) is a lightweight, cross-platform OS abstraction layer with **20+ years of embedded heritage**. Many of these source files originated in embedded projects two decades ago, so they are lightweight and portable. The code balances "modern C++" with lightweight C-like heritage—**correctness trumps modernity**.

### Design Philosophy

- **Portability First**: Works on Windows, Linux, macOS, Android, iOS, Free-RTOS, TI-RTOS, and WebAssembly
- **Lightweight**: Minimal overhead, suitable for embedded systems
- **Proven**: Battle-tested across decades of commercial projects
- **Incremental Modernization**: Gradually refactoring while maintaining reliability

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

### Emscripten Build (WebAssembly)

```bash
# See emscripten_build.sh in parent SAU project
# SOP is included as part of SAU's WebAssembly build
```

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
├── CLAUDE.md                   # AI developer instructions
└── README.md                   # This file
```

---

## Components

### OSAL (OS Abstraction Layer)

**Location**: `sop_src/osal/`

**Purpose**: Provides cross-platform primitives for:
- **Threading**: `OSALTaskPtrT`, task creation and management
- **Mutexes**: `OSALMutexPtrT`, recursive and non-recursive locks
- **Timing**: `OSALSleep()`, `OSALGetMS()` for millisecond timing
- **Memory**: `OSALMALLOC()`, `OSALFREE()` for portable allocation

**Key APIs** (see `sop_src/osal/osal.h` for full list and exact signatures):
```c
// Initialization
void OSALInit(void);

// Mutexes
OSALMutexPtrT OSALCreateMutex(void);
void OSALDeleteMutex(OSALMutexPtrT* ppMutex);
bool OSALLockMutex(OSALMutexPtrT pMutex, uint32_t timeoutMs);
bool OSALUnlockMutex(OSALMutexPtrT pMutex);

// Timing
void OSALSleep(uint32_t milliseconds);
uint32_t OSALGetMS(void);

// Tasks
OSALTaskPtrT OSALTaskCreate(OSALTaskFuncPtrT pTaskFunc, void* pParam, OSALPrioT prio, const OSALTaskStructT* pPlatform);
```

### Utils

**Location**: `sop_src/utils/`

**Purpose**: General-purpose utilities:
- **Logging**: `platform_log()` with severity levels (PL_MEDIUM, PL_HIGH, etc.)
- **Byte Queue**: `byteq_t` - lightweight circular buffer for byte streams
- **Linked Lists**: Single and double-linked list implementations
- **CRC**: CRC-16 and CRC-32 calculations
- **UUID**: RFC 4122 UUID generation
- **File Operations**: Cross-platform file I/O helpers

**Key Example**:
```cpp
#include "utils/platform_log.h"
#include "utils/byteq.h"

platform_log(PL_MEDIUM, "Processing %d items", count);

byteq_t queue;
byteq_init(&queue, buffer, sizeof(buffer));
byteq_write(&queue, data, len);
```

### Task Scheduler

**Location**: `sop_src/task_sched/`

**Purpose**: Run-to-completion state machine scheduler for event-driven systems.

**Key Concept**: Tasks run in phases; each task completes before the next starts—no preemption within a phase.

### BufIO (Buffered I/O)

**Location**: `sop_src/buf_io/`

**Purpose**: Buffered I/O with FIFO queues for reliable data streaming.

### MiniSocket

**Location**: `sop_src/mini_socket/`

**Purpose**: Cross-platform socket abstraction for TCP/UDP networking.

### SimplePlot

**Location**: `sop_src/simple_plot/`

**Purpose**: Lightweight data visualization utilities.

### Crypto Integration

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

### Emscripten / WebAssembly
- Build as part of parent SAU project
- Memory growth enabled for dynamic allocation

---

## Coding Guidelines

See **[C_CODING_GUIDELINES.md](C_CODING_GUIDELINES.md)** for comprehensive C/C++ coding conventions:

- Naming conventions (classes, functions, variables, constants)
- Code formatting (clang-format)
- Memory safety best practices
- Include ordering and header guards
- Comment style and documentation
- C++11/14 features
- Constant management

**Quick Summary**:
- **C++ Standard**: C++11 (C++14 features acceptable)
- **Naming**: Use existing patterns (`osal_` prefix for C API, `task_` for scheduler)
- **Error Handling**: Check return values, use `platform_log` for logging
- **Testing**: Add tests for new functionality
- **Documentation**: Update README.md and CLAUDE.md for significant changes
- **Cross-Platform**: Use OSAL for platform-specific operations

---

## Development Workflow

### Keeping AI Instructions Synchronized

When changes are made to `CLAUDE.md`, run the sync script:

```bash
./scripts/copy-ai-instructions.sh
```

This copies `CLAUDE.md` to:
- `GEMINI.md` - For Google Gemini
- `.github/copilot-instructions.md` - For GitHub Copilot

### Formatting Code

```bash
# Format all source files (after adding format target to CMake)
make format

# Check format without modifying (CI)
make format-check

# Format single file
clang-format -i path/to/file.cpp
```

---

## Related Projects

SOP is used as a dependency by:

- **SAU (Sweet Audio Utils)**: https://github.com/cfogelklou/sweet_audio_utils
  - Audio processing library and strobe tuner
  - Uses SOP for OS abstraction across platforms

---

## Contributing

When contributing to SOP:

1. Follow the coding guidelines in `C_CODING_GUIDELINES.md`
2. Add tests for new functionality
3. Ensure all tests pass (`ctest`)
4. Format code with `clang-format`
5. Update documentation as needed
6. Keep the embedded heritage in mind—portability over modernity

---

## License

MIT License

Copyright 2020 Chris Fogelklou

---

**Last Updated**: 2025-03-10
