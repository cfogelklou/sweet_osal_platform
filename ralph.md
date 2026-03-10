# Ralph Wiggum - Autonomous Plan-Driven Development for SOP

**Repository**: SOP (Sweet OS Platform)
**Purpose**: Specification for autonomous AI agent to implement tasks from Claude Code planning documents

## Quick Reference

| Action | Command |
|--------|---------|
| Run loop (auto-find active plan) | `./loop.sh` |
| Ingest and run plan | `./loop.sh <plan-file>` |
| Verbose mode | `./loop.sh -v <plan-file>` |
| Status of all plans | `./loop.sh status` |
| Status of specific plan | `./loop.sh status <plan-file>` |

---

## Overview

The Ralph Wiggum workflow enables autonomous implementation of Claude Code planning documents. The agent continuously iterates through tasks until the plan is complete.

---

## The loops/ Directory

Ralph uses a `loops/` working directory to manage active plans:

```
loops/
├── plan-foo.md         # Working copy with checkboxes (tracked in git)
└── plan-bar.md         # Another working plan (tracked in git)
```

**Why loops/?**
- **Plan conversion**: Plans without checkboxes are auto-converted using AI
- **Working copy separation**: Original plans stay untouched; working copies go in `loops/`
- **Multi-plan support**: Multiple plans can be tracked simultaneously
- **Shared progress**: Plans are tracked in git so progress syncs across machines

### Behavior Matrix

| Command | loops/ State | Action |
|---------|--------------|--------|
| `./loop.sh` | Empty | Exit: "No active plans. Use: ./loop.sh <plan-file>" |
| `./loop.sh` | All tasks complete | Exit: "All plans complete!" |
| `./loop.sh` | Has incomplete tasks | Find first plan with `- [ ]` and work on it |
| `./loop.sh docs/foo.md` | foo.md not in loops/ | Copy + convert to `loops/foo.md`, then work |
| `./loop.sh docs/foo.md` | foo.md exists in loops/ | Use existing `loops/foo.md` (user can delete to reset) |
| `./loop.sh status` | - | Show all plans in loops/ with progress |

---

## Ralph Principles

| Principle | Description |
|-----------|-------------|
| **Context is everything** | One task per loop iteration, fresh context each time |
| **Backpressure critical** | Build/test before committing, fail fast |
| **Let Ralph Ralph** | Agent picks next task in order, self-corrects |
| **Plan is disposable** | Can be edited manually at any time |

---

## Workflow

### 1. Create or Use a Plan File

Plans are Claude Code planning mode documents with task checkboxes:

```markdown
# Implementation Plan

## Phase 1: Setup
- [ ] Task 1
- [ ] Task 2

## Phase 2: Implementation
- [ ] Task 3
- [ ] Task 4

## Completed
- [x] Done task
```

### 2. Ingest the Plan

```bash
# Ingest a plan and start working on it
./loop.sh docs/plan-to-implement-feature.md
```

This will:
1. Copy the plan to `loops/plan-to-implement-feature.md`
2. If the plan has no checkboxes, convert it using AI
3. Start working on the first task

### 3. Continue Working

```bash
# Continue working on active plan (finds first with incomplete tasks)
./loop.sh
```

### 4. Each Iteration

1. Read plan file for next `- [ ]` task
2. Implement the task
3. Run build and tests
4. Update plan: `- [ ]` → `- [x]`
5. Commit and push
6. Loop to next task

### 5. Completion

The loop exits automatically when all tasks are complete (no more `- [ ]` items).

---

## Usage

```bash
# Ingest a plan and start working on it
./loop.sh docs/plan-to-implement-feature.md

# Continue working on active plan (finds first with incomplete tasks)
./loop.sh

# Show status of all plans in loops/
./loop.sh status

# Verbose mode (see Claude output in real-time)
./loop.sh -v docs/plan-to-implement-feature.md

# Show help
./loop.sh --help

# Reset a plan (delete from loops/ to re-ingest fresh)
rm loops/plan-to-implement-feature.md
./loop.sh docs/plan-to-implement-feature.md
```

---

## SOP Build Commands

The loop automatically runs these backpressure checks:

```bash
# Build
cd build && cmake .. && make -j8

# Run tests
./sweet_osal_test

# Run all tests
ctest -j8
```

---

## Pre-Check: SSH Key Verification

**BEFORE starting the loop, verify SSH keys work:**

```bash
git fetch origin
```

If this prompts for a password, the loop cannot proceed (it needs non-interactive git push).

---

## Plan File Format

Claude Code planning mode documents must use markdown checkboxes for task tracking:

```markdown
# Plan Title

## Context
Background information...

## Phase 1: Phase Name
- [ ] Task description
- [ ] Another task

### Phase 1.1: Sub-section
- [ ] Detailed step 1
- [ ] Detailed step 2

## Phase 2: Next Phase
- [ ] More tasks

## Completed
- [x] Finished task
```

### Auto-Conversion

If your plan file doesn't have checkboxes, Ralph will automatically convert it using AI when you first ingest it. The AI adds:
- Task checkboxes (`- [ ]` format)
- Explore/Implement/Verify hints for each task

---

## Key Files

| File | Purpose |
|------|---------|
| `loop.sh` | Main loop script |
| `loops/` | Working directory for active plans |
| `ralph.md` | This documentation |
| `CLAUDE.md` | Project instructions for AI |

---

## Troubleshooting

### Build Fails

1. Do NOT commit
2. Fix the issue locally
3. Re-run build/tests
4. Only commit when all pass

### Stuck on Same Task

1. Edit the plan file in `loops/` to break the task into smaller steps
2. The loop will pick up changes on next iteration

### Branch Falls Behind

```bash
git fetch origin
git rebase origin/main
git push --force-with-lease
```

### Reset a Plan

To re-ingest a plan from its source (e.g., after updating the original):

```bash
rm loops/plan-name.md
./loop.sh docs/plan-name.md
```

---

## Requirements

- `~/.zai.json` configured with z.ai credentials
- Build directory configured (`build/`)
- `jq` for parsing config (optional, falls back to default claude)

---

## Related Documents

| Document | Purpose |
|----------|---------|
| `CLAUDE.md` | Full development guidelines and project conventions |
| `C_CODING_GUIDELINES.md` | C/C++ coding standards |
| `README.md` | Project overview and quick start |

---

**Last Updated**: 2026-03-10
