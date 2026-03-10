# SOP C/C++ Coding Guidelines

This document defines the coding standards and conventions for the SOP (Sweet OS Platform) project. All contributors—both human and AI—should follow these guidelines to maintain code consistency, quality, and maintainability.

---

## Table of Contents

1. [File Header & Copyright](#file-header--copyright)
2. [Naming Conventions](#naming-conventions)
3. [Code Formatting](#code-formatting)
4. [Memory Safety](#memory-safety)
5. [Include Ordering](#include-ordering)
6. [Comment Style](#comment-style)
7. [C++ Version & Features](#c-version--features)
8. [Constant Management](#constant-management)
9. [Platform Portability](#platform-portability)
10. [Testing Guidelines](#testing-guidelines)

---

## File Header & Copyright

All source files should begin with the standard copyright header:

```cpp
/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  @author: chris.fogelklou@gmail.com
******************************************************************************/
```

---

## Naming Conventions

### Classes and Structs
- **Style**: `PascalCase`
- **Examples**: `TaskScheduler`, `ByteQueue`, `OsThread`, `Mutex`

### Methods and Functions
- **C++ Class methods**: `camelCase`
  - **Examples**: `init()`, `process()`, `getData()`, `allocate()`
- **C API functions**: `prefix_PascalCase()`
  - **OSAL functions**: `osal_MutexCreate()`, `osal_ThreadCreate()`, `osal_SleepMs()`
  - **Task scheduler**: `task_SchedCreate()`, `task_SchedRun()`
  - **Utils**: `byteq_Init()`, `platform_Log()`

### Variables
- **Member variables**: `m_camelCase` (prefix with `m_`)
  - **Examples**: `mMutex`, `mBuffer`, `mCount`, `mInitialized`
- **Local variables**: `camelCase`
  - **Examples**: `buffer`, `count`, `threadHandle`
- **Function parameters**: `camelCase`
  - **Examples**: `inputBuffer`, `numBytes`, `timeoutMs`

### Constants and Macros
- **Constants**: `ALL_CAPS_WITH_UNDERSCORES`
  - **Examples**: `MAX_QUEUE_SIZE`, `DEFAULT_TIMEOUT_MS`, `MIN_BUFFER_SIZE`
- **Macro names**: `ALL_CAPS_WITH_UNDERSCORES`
  - **Examples**: `MIN(a,b)`, `CLAMP(x, lo, hi)`

### Types
- **Typedefs/Aliases**: `_t` suffix
  - **Examples**: `osal_thread_t`, `osal_mutex_t`, `byteq_t`, `task_sched_t`
- **Enums**: `E_PASCAL_CASE` or `kPascalCase` for values
  - **Examples**: `E_THREAD_RUNNING`, `E_MUTEX_LOCKED`, `kStateIdle`

### File Names
- **C++ headers**: `.hpp` extension (preferred for C++) or `.h`
- **C headers**: `.h` extension
- **Source files**: `.cpp` for C++, `.c` for C
- **File names**: `snake_case.cpp` or `snake_case.hpp`
  - **Examples**: `byteq.cpp`, `osal_mutex.h`, `task_sched.hpp`

---

## Code Formatting

SOP uses `clang-format` for automated code formatting. The configuration is in `.clang-format` at the project root.

### Key Formatting Rules

- **Indentation**: 2 spaces
- **Column limit**: 100 characters
- **Brace style**: Opening brace on same line (K&R/Stroustrup variant)
- **Pointer alignment**: Left-aligned (`T* ptr` not `T *ptr`)

### Formatting Commands

```bash
# Format a single file
clang-format -i myfile.cpp

# Format all source files
find sop_src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check format without modifying (CI)
clang-format --dry-run --Werror myfile.cpp
```

### Example Formatting

```cpp
// Correct: Opening brace on same line, 2-space indent
class ByteQueue {
public:
  ByteQueue() : mSize(0) {}

  void write(const uint8_t* data, size_t len) {
    if (len > 0 && data != nullptr) {
      // Write data
    }
  }

private:
  uint8_t* mBuffer;
  size_t mSize;
};

// Correct: Control structures
if (mutex != nullptr) {
  osal_mutex_lock(mutex);
} else {
  handleError();
}

// Correct: Short conditionals can be one-liner
if (ptr == nullptr) return false;
```

---

## Memory Safety

### Smart Pointers (Preferred for C++ code)

**Use `std::unique_ptr<T>` for single ownership:**

```cpp
// Preferred: Unique pointer for owned members
class TaskScheduler {
public:
  TaskScheduler()
      : mMutex(std::unique_ptr<osal_mutex_t>(
            osal_mutex_create(), osal_mutex_destroy)) {}

private:
  std::unique_ptr<osal_mutex_t, decltype(&osal_mutex_destroy)> mMutex;
};

// For arrays: use unique_ptr<T[]> or std::vector
auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[1024]);
auto data = std::vector<uint8_t>(1024);  // Even better
```

**Raw pointers for non-owning references:**

```cpp
// OK: Raw pointer as observer (does not own)
void processBuffer(const uint8_t* buffer, size_t size);
```

### Containers

```cpp
// Preferred: std::vector for dynamic arrays
std::vector<uint8_t> mBuffer;

// Use reserve() when size is known
mBuffer.reserve(expectedSize);

// Prefer standard algorithms
std::sort(mItems.begin(), mItems.end());
```

### Const Correctness

```cpp
// Function parameters: Pass by const ref for complex types
void setConfig(const Config& config);

// Member functions: const at end when they don't modify state
size_t getSize() const { return mSize; }

// Local variables: const when immutable
const size_t bufferSize = mSize;
```

### Initialization

```cpp
// Preferred: Member initializer list
class MyClass {
public:
  MyClass(size_t size) : mSize(size), mInitialized(true) {}

private:
  size_t mSize;
  bool mInitialized;
};

// Use in-class initialization for default values
class MyClass {
  int mCounter = 0;
  size_t mBufferSize = 256;
};
```

---

## Include Ordering

Organize includes in this order:

1. **Module-specific headers** (current module's headers)
2. **Project headers** (other SOP headers)
3. **Standard library headers** (with `<>`)

```cpp
// 1. Module-specific
#include "byteq.h"
#include "byteq_impl.hpp"

// 2. Project headers
#include "osal/osal.h"
#include "utils/platform_log.h"
#include "task_sched/task_sched.h"

// 3. Standard library
#include <algorithm>
#include <cstdint>
#include <vector>
```

**Note**: For SOP, prioritize OSAL and utils includes as they are the core abstractions.

---

## Comment Style

### Single-Line Comments

Use `//` for most code comments:

```cpp
// Remove DC component by subtracting the mean
float mean = calculateMean(samples, count);

// Fall through to next case (intentional)
// no break needed here
```

### Block Comments

Use `/* */` for longer explanations and file headers:

```cpp
/*
 * The byte queue implements a circular buffer for efficient byte stream
 * processing. It provides thread-safe read/write operations when used
 * with OSAL mutexes.
 */
```

### Doxygen Documentation

Use `/** */` for public API documentation:

```cpp
/**
 * @brief Create a new OSAL mutex
 * @return Pointer to the created mutex, or NULL on failure
 * @note Call osal_mutex_destroy() to free the mutex when done
 */
osal_mutex_t* osal_mutex_create(void);
```

### Comment What and Why, Not How

```cpp
// Good: Explains purpose
// Spin-wait with exponential backoff to reduce contention
while (try_lock_failed) {
  backoff *= 2;
  osal_sleep_ms(backoff);
}

// Bad: Just restates the code
// Multiply backoff by 2 and sleep
backoff *= 2;
osal_sleep_ms(backoff);
```

---

## C++ Version & Features

### Standard

- **Target**: C++11
- **Acceptable**: C++14 features where useful
- **Avoid**: C++17/20+ features (for maximum compatibility, especially embedded)

### Modern C++11 Practices

```cpp
// Use constexpr for compile-time constants
constexpr size_t MAX_QUEUE_SIZE = 256;
constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;

// Use enum class for type-safe enums (new code)
enum class ThreadState {
  kIdle,
  kRunning,
  kStopped
};

// Use override keyword when overriding virtual functions
void process() override;

// Use range-based for loops
for (const auto& item : mItems) {
  process(item);
}

// Use auto when type is obvious
auto it = mItems.begin();
auto ptr = getData();  // When return type is well-known

// Don't use auto when clarity matters
size_t size = getSize();  // Clearer than auto
```

### nullptr vs NULL

```cpp
// Preferred: nullptr (C++11)
if (ptr == nullptr) { }

// Avoid: NULL or 0
if (ptr == NULL) { }  // Legacy
if (ptr == 0) { }     // Legacy
```

---

## Constant Management

### Centralized Configuration

**TODO**: Create `sop_src/sop_config.hpp` to centralize magic numbers.

Current issue: Many hardcoded values scattered across files.

### Best Practices

```cpp
// Preferred: Named constants in a namespace
namespace sop {
  constexpr size_t DEFAULT_BUFFER_SIZE = 256;
  constexpr uint32_t DEFAULT_TIMEOUT_MS = 1000;
  constexpr int MAX_QUEUE_DEPTH = 16;
}

// Avoid: Magic numbers in code
void process() {
  if (mSize > 256) { }  // Bad: Magic number
}

// Better
void process() {
  if (mSize > sop::DEFAULT_BUFFER_SIZE) { }
}
```

---

## Platform Portability

### Custom Types

Use platform-agnostic types:

```cpp
#include <cstdint>

// Use these instead of built-in types
uint8_t   // Unsigned 8-bit
int32_t   // Signed 32-bit
uint64_t  // Unsigned 64-bit
size_t    // Size type
```

### OS Abstraction Layer (OSAL)

Use OSAL for platform-specific operations:

```cpp
#include "osal/osal.h"

// Threading
osal_thread_t* thread = osal_thread_create(workerFunc, arg);

// Mutexes
osal_mutex_t* mutex = osal_mutex_create();
osal_mutex_lock(mutex);
// ... critical section ...
osal_mutex_unlock(mutex);

// Timing
osal_sleep_ms(10);  // Sleep for 10 milliseconds
uint32_t time = osal_get_ticks();  // Millisecond timer

// Memory
void* ptr = osal_malloc(size);
osal_free(ptr);
```

### Logging

```cpp
#include "utils/platform_log.h"

// Log with severity levels
platform_log(PL_MEDIUM, "Processing %d bytes", count);
platform_error("Failed to allocate buffer");
platform_debug("Debug info: %s", debugStr);
```

---

## Header Guards

Use traditional `#ifndef` guards (not `#pragma once`):

```cpp
#ifndef BYTEQ_HPP__
#define BYTEQ_HPP__

// declarations...

#endif // BYTEQ_HPP__
```

Guard format: `FILENAME_HPP__` or `FILENAME_H__` (all caps, double underscore suffix)

---

## Testing Guidelines

### Unit Test Structure

Tests use Google Test framework:

```cpp
#include "gtest/gtest.h"

class ByteQueueTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup
    byteq_init(&mQueue, mBuffer, sizeof(mBuffer));
  }

  void TearDown() override {
    // Test cleanup
  }

  // Test data members
  byteq_t mQueue;
  uint8_t mBuffer[256];
};

TEST_F(ByteQueueTest, WriteAndRead) {
  // Arrange
  const uint8_t testData[] = {1, 2, 3, 4};

  // Act
  byteq_write(&mQueue, testData, sizeof(testData));
  uint8_t output[4];
  size_t read = byteq_read(&mQueue, output, sizeof(output));

  // Assert
  EXPECT_EQ(read, 4);
  EXPECT_EQ(output[0], 1);
  EXPECT_EQ(output[3], 4);
}
```

### Test Location

Place test files in appropriate test directories:

```
sop/
├── test/                   # Integration tests
│   └── *.cpp              # sweet_osal_test
├── sop_src/
│   ├── osal/test/         # OSAL-specific tests
│   ├── utils/tests/       # Utils tests
│   └── task_sched/tests/  # Task scheduler tests
```

### Running Tests

```bash
# Build all tests
cd build && cmake .. && make

# Run all tests
ctest -j8

# Run specific test
./sweet_osal_test

# Verbose output
ctest -V -R osal_test
```

---

## Build System Integration

### CMake Targets

The following CMake targets are available (after format target implementation):

```bash
# Format all source files
make format

# Check format without modifying (for CI)
make format-check
```

---

## Embedded Heritage Note

SOP has **20+ years of embedded heritage**. This means:

1. **Lightweight code**: Many utilities are closer to C than C++ (e.g., `byteq`)
2. **Correctness trumps modernity**: Old but proven patterns are preferred over trendy new ones
3. **Incremental modernization**: Gradually refactoring while maintaining reliability
4. **C compatibility**: Many components are designed to work from both C and C++

When in doubt, prioritize **portability and reliability** over **modern C++ features**.

---

## Summary Checklist

Before committing code, verify:

- [ ] File header with copyright included
- [ ] Naming conventions followed (osal_, task_, byteq_ prefixes)
- [ ] Code formatted with `clang-format`
- [ ] Header guards use `#ifndef` pattern
- [ ] Smart pointers used for ownership (C++ code)
- [ ] `const` correctness applied
- [ ] Magic numbers replaced with named constants
- [ ] Public APIs documented with Doxygen
- [ ] Includes properly ordered (OSAL/utils first)
- [ ] Tests pass locally (`ctest`)

---

**Last updated**: 2025-03-10

*For questions or clarifications, please open an issue or pull request.*
