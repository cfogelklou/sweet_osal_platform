# Adapt Staged Files for Sweet OS Platform (SOP)

## Context

The staged changes come from SAU (Sweet Audio Utils), a higher-level project that depends on SOP (Sweet OS Platform). The staged files need to be adapted to describe SOP instead of SAU.

**Current Situation:**

- SOP already has copies of these files (CLAUDE.md, C_CODING_GUIDELINES.md, ralph.md, wow_github_monitoring.md, loop.sh, loop_big_picture.sh, copy-ai-instructions.sh, loops/.gitkeep, .gitignore)
- These files currently describe SAU (audio processing library)
- They need to be updated to describe SOP (OS abstraction layer)
- The .github/workflows/ci_pr.yml already uses ubuntu-latest, macos-latest, windows-latest (no self-hosted runners to remove)

**Key Adaptations Needed:**

- Project name: SAU → SOP / sweet_osal_platform
- Directory structure: sau_src/ → sop_src/ or sop/sop_src/
- Component descriptions: audio processing → OS abstraction
- Build/test commands: update paths
- Keep same patterns, conventions, and structure

---

## Files to Update

### 1. .gitignore

- Add `build/` directory to gitignore

### 2. CLAUDE.md

- Change "SAU (Sweet Audio Utils)" → "SOP (Sweet OS Platform)"
- Update project description to focus on OS abstraction
- Update directory structure (sau_src/ → sop_src/)
- Update component descriptions (audio → OSAL)
- Update build/test commands
- Remove SAU-specific sections (MoTuner, Strobe Tuner, FFT components)
- Add SOP-specific sections (OSAL, Task Scheduler, Utils, BufIO, etc.)

### 3. C_CODING_GUIDELINES.md

- Change "SAU C/C++ Coding Guidelines" → "SOP C/C++ Coding Guidelines"
- Update project references
- Keep coding conventions (they apply to both projects)
- Update example code to use SOP classes/functions

### 4. loop.sh

- Change script header comment: "for SAU" → "for SOP"
- Update build commands: sau_src/ → sop_src/
- Update test commands for SOP tests (sweet_osal_test, osal_test, utils_test, etc.)
- Update path references

### 5. loop_big_picture.sh

- Similar adaptations as loop.sh
- Update project references

### 6. ralph.md

- Change "for SAU" → "for SOP"
- Update repository context
- Update build/test commands for SOP

### 7. scripts/copy-ai-instructions.sh

- Already generic, should work as-is
- Verify paths are correct

### 8. wow_github_monitoring.md

- Update project references
- Keep monitoring logic

### 9. loops/.gitkeep

- Placeholder file, no changes needed

---

## Critical File Paths

**Files to modify:**

- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/.gitignore`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/CLAUDE.md`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/C_CODING_GUIDELINES.md`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/loop.sh`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/loop_big_picture.sh`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/ralph.md`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/scripts/copy-ai-instructions.sh`
- `/Volumes/Projects/dev/strobopro_dev/src/sharedSrc/rn_audio/sau/sop/wow_github_monitoring.md`

**Reference files (from SAU staged changes):**

- Staged .gitignore - adds build/
- Staged CLAUDE.md - has updated structure and sections
- Staged C_CODING_GUIDELINES.md - comprehensive guidelines
- Staged loop.sh - autonomous implementation script
- Staged loop_big_picture.sh - big picture variant

---

## Implementation Plan

### Phase 1: Update .gitignore

- [ ] Add `build/` to .gitignore

### Phase 2: Update CLAUDE.md

- [ ] Update title: "SAU (Sweet Audio Utils)" → "SOP (Sweet OS Platform)"
- [ ] Update project overview section
- [ ] Update directory structure (sau_src/ → sop_src/)
- [ ] Replace SAU-specific components with SOP components:
  - Remove: MoTuner, Strobe Tuner, FFT Components, Signal Processing sections
  - Add/Keep: OSAL, Task Scheduler, Utils, BufIO, MiniSocket, SimplePlot sections
- [ ] Update testing section (SAU tests → SOP tests)
- [ ] Update build system details
- [ ] Update platform-specific notes
- [ ] Remove/replace audio-related context sections
- [ ] Update Ralph Loop section with SOP build/test commands

### Phase 3: Update C_CODING_GUIDELINES.md

- [ ] Update title and header
- [ ] Update project references throughout
- [ ] Update example code to use SOP components
- [ ] Keep all coding conventions (they're project-agnostic)

### Phase 4: Update loop.sh

- [ ] Update script header comment
- [ ] Update build commands: `ninja -j4 motunit` → `make -j8` (or appropriate SOP test)
- [ ] Update test paths and commands
- [ ] Update any remaining SAU references

### Phase 5: Update loop_big_picture.sh

- [ ] Same adaptations as loop.sh

### Phase 6: Update ralph.md

- [ ] Update repository context
- [ ] Update SAU build commands section
- [ ] Update project references

### Phase 7: Update scripts/copy-ai-instructions.sh

- [ ] Verify script works for SOP context
- [ ] Update paths if needed

### Phase 8: Update wow_github_monitoring.md

- [ ] Update project references
- [ ] Verify monitoring context applies to SOP

### Phase 9: Verify and Test

- [ ] Check all files for remaining SAU references
- [ ] Verify .github/workflows/ci_pr.yml (already uses github-hosted runners)
- [ ] Ensure file paths are correct
- [ ] Verify scripts are executable

---

## SOP-Specific Content to Reference

**From existing SOP README.md:**

- "An easily portable abstraction layer for Windows, Linux, Free-RTOS, Android, and iOS platforms"
- "OS abstractions and utilities that have been proven useful over my career"
- "Many of these source files originated in embedded projects 20 years ago"

**SOP Components (from sop_src/):**

- osal/ - OS Abstraction Layer (threading, mutexes, timing, memory)
- utils/ - Utilities (lists, queues, logging, CRC, UUID, file operations)
- task_sched/ - Task scheduler (run-to-completion state machine)
- buf_io/ - Buffered I/O with FIFO queues
- mini_socket/ - Socket abstraction
- simple_plot/ - Plotting utilities
- mbedtls/ - Crypto integration
- libsodium/ - Sodium crypto wrapper

**SOP Tests:**

- sweet_osal_test - OS abstraction tests
- osal_test - OSAL-specific tests
- utils_test - Utility function tests
- test_simple_plot - Plotting tests
- transport_test - Transport layer tests

---

## Verification

After implementation:

1. Check all updated files for remaining "SAU" or "sau_src" references
2. Verify directory paths point to sop_src/ or sop/sop_src/
3. Test scripts if possible
4. Verify workflow file (no self-hosted runners present)

---

## Note on Self-Hosted Runners

The `.github/workflows/ci_pr.yml` already uses `ubuntu-latest`, `macos-latest`, and `windows-latest`. No self-hosted runners are present, so no changes are needed for the workflow file.
