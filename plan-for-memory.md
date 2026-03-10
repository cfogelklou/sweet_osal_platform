# Plan: Integrate Memory MCP into Loop Scripts

## Context

The `loop.sh` (Ralph) and `loop_big_picture.sh` (Big Picture) autonomous development scripts don't utilize the memory MCP configured in `.mcp.json`. The memory MCP could help:

- Remember failed implementation attempts (dead ends)
- Store anti-patterns discovered during development
- Track what approaches have been tried
- Maintain context across multiple journey/plan executions

**Problem**: MCP servers are only available in interactive Claude Code sessions. When the scripts invoke `claude -p` directly, MCP servers are not loaded.

## Solution

Add memory integration with **dual-mode support**:

1. **Primary**: Use memory MCP tools if available (when agent detects MCP server)
2. **Fallback**: Use markdown file (`memory.md`) in project root when MCP unavailable

This ensures:
- Interactive Claude Code sessions can use native MCP tools
- Script-based execution (`loop.sh`, `loop_big_picture.sh`) falls back to markdown
- Memory is human-readable and version-controllable

## Implementation Plan

### Phase 1: Add Memory Utility Functions with Dual-Mode Support

**File**: `loop.sh` and `loop_big_picture.sh`

Add shared functions for memory operations with MCP detection and markdown fallback:

```bash
# Memory files (matches .mcp.json config + markdown fallback)
MEMORY_JSON="${PROJECT_ROOT}/memory.json"
MEMORY_MD="${PROJECT_ROOT}/memory.md"

# Detect if memory MCP is available to the agent
# This sets a flag that prompts can reference
detect_mcp_memory() {
    # The agent will detect this automatically
    # We just signal our intent in the prompt
    echo "MCP_DETECTED=false"
}

# Ensure memory markdown file exists with header
ensure_memory_md() {
    if [[ ! -f "$MEMORY_MD" ]]; then
        cat > "$MEMORY_MD" << 'EOF'
# Project Memory

This file stores development learnings, anti-patterns, and failed approaches.
It serves as a fallback when the memory MCP server is unavailable.

## Anti-Patterns

*(No anti-patterns recorded yet)*

## Failed Approaches (Dead Ends)

*(No failed approaches recorded yet)*

## Successful Patterns

*(No successful patterns recorded yet)*

EOF
    fi
}

# Search memory markdown for entities matching a query
search_memory_md() {
    local query="$1"
    if [[ ! -f "$MEMORY_MD" ]]; then
        return
    fi

    # Case-insensitive search in markdown
    awk -v q="$query" '
        BEGIN { IGNORECASE=1; in_section=0; found=0 }
        /^## / { in_section=0 }
        /^## (Anti-Patterns|Failed Approaches|Successful Patterns)/ { in_section=1 }
        in_section && $0 ~ q { print; found=1 }
        /^## / && in_section && !/^## (Anti-Patterns|Failed Approaches|Successful Patterns)/ { in_section=0 }
    ' "$MEMORY_MD"
}

# Add entry to memory markdown
add_memory_md() {
    local section="$1"  # "Anti-Patterns", "Failed Approaches", "Successful Patterns"
    local entity="$2"
    local description="$3"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%d")

    ensure_memory_md

    # Find section and append entry
    local temp=$(mktemp)
    awk -v section="$section" \
        -v entity="$entity" \
        -v desc="$description" \
        -v timestamp="$timestamp" '
        /^## '"$section"'$/ {
            print
            getline
            if (/\*\(No.*recorded yet\)\*\*/) {
                print ""
            } else if (NF > 0) {
                print ""
            }
            print "- **" entity "** (" timestamp ")"
            print "  " desc
            print ""
            # Skip until next section
            while (getline > 0 && !/^## /) {}
            print $0
            next
        }
        { print }
    ' "$MEMORY_MD" > "$temp"
    mv "$temp" "$MEMORY_MD"
}
```

### Phase 2: Modify Prompts to Include Memory Instructions

**File**: `loop.sh` - Update `generate_prompt()` function

Add memory-aware instructions at the end of the prompt:

```bash
generate_prompt() {
    # ... existing prompt ...

    # Add memory section
    cat << 'MEMORY_EOF'

## Memory Usage

**IMPORTANT**: Check available memory BEFORE implementing any approach.

### If Memory MCP tools are available:
Use `mcp__memory__search_nodes` and `mcp__memory__open_nodes` to:
- Search for similar approaches that were tried before
- Look for anti-patterns related to the current task
- Check for failed implementations

After completing each task, use `mcp__memory__create_entities` or `mcp__memory__add_observations` to record:
- New anti-patterns discovered
- Approaches that failed (with reasons)
- Successful patterns to reuse

### If Memory MCP is NOT available:
Read the fallback memory file at: MEMORY_MD

Search for relevant entries and avoid repeating past failures.

After discovering new learnings, update the memory.md file:
```bash
# Add anti-pattern
- **pattern-name** (YYYY-MM-DD)
  Description of why this pattern doesn't work

# Add failed approach
- **approach-name** (YYYY-MM-DD)
  Why it failed and what was learned
```

MEMORY_EOF

    # Also append current memory.md content if it exists
    if [[ -f "$MEMORY_MD" ]]; then
        echo ""
        echo "### Current Memory Contents:"
        echo ""
        cat "$MEMORY_MD"
    fi
}
```

**File**: `loop_big_picture.sh` - Update `generate_iteration_prompt()` function

Add enhanced memory instructions:

```bash
generate_iteration_prompt() {
    # ... existing prompt ...

    # Add memory-aware section
    cat << 'MEMORY_EOF'

## Memory Integration

This journey should leverage memory to avoid repeating past mistakes.

### Phase: RESEARCHING
- If MCP available: Use `mcp__memory__search_nodes` for relevant entities
- Otherwise: Read memory.md for anti-patterns
- Document any new findings

### Phase: PLANNING
- **CRITICAL**: Check Anti-Patterns section before selecting approaches
- If an approach violated constraints in memory, DON'T select it
- Record why you're choosing your selected approach

### Phase: PIVOTING
- Before pivoting, search memory for similar failed approaches
- Add the failed approach to memory with reasons
- Record the semantic constraint (anti-pattern) for future reference

### Phase: CONSOLIDATING
- Add successful patterns to memory for reuse
- Document any new anti-patterns discovered

### Memory Output Format

When using MCP:
```
mcp__memory__create_entities: entities=[{"name": "approach-name", "entityType": "failed-approach", "observations": ["why it failed"]}]
```

When using markdown fallback, append to memory.md:
```markdown
## Failed Approaches (Dead Ends)
- **approach-name** (YYYY-MM-DD)
  Reason: X didn't work because Y
  Lesson: Use Z instead
```

MEMORY_EOF

    # Append memory.md contents for reference
    if [[ -f "$MEMORY_MD" ]]; then
        echo ""
        echo "### Current Memory.md Contents:"
        echo ""
        grep -A 100 "^##" "$MEMORY_MD" || true
    fi
}
```

### Phase 3: Journey-to-Memory Synchronization

**File**: `loop_big_picture.sh` - Add sync function

```bash
# Sync journey learnings to memory.md during CONSOLIDATING phase
sync_journey_to_memory() {
    local journey_file="$1"

    ensure_memory_md

    # Extract anti-patterns from journey file
    local anti_patterns
    anti_patterns=$(sed -n '/^## Anti-Patterns/,/^## /p' "$journey_file" | grep "^- \*\*" || true)

    # Extract dead ends from journey file
    local dead_ends
    dead_ends=$(sed -n '/^## Dead Ends/,/^## /p' "$journey_file" | grep "^### " || true)

    # Add anti-patterns to memory.md
    while IFS= read -r line; do
        if [[ "$line" =~ ^- \*\*(.+)\*\*:\ (.+)$ ]]; then
            local pattern="${BASH_REMATCH[1]}"
            local desc="${BASH_REMATCH[2]}"
            add_memory_md "Anti-Patterns" "$pattern" "$desc"
        fi
    done <<< "$anti_patterns"

    # Add dead ends to memory.md
    while IFS= read -r line; do
        if [[ "$line" =~ ^###\ (.+) ]]; then
            local approach="${BASH_REMATCH[1]}"
            # Extract reason from following lines
            local reason=$(sed -n "/^### $approach/,/^###/p" "$journey_file" | grep "Reason:" | sed 's/.*Reason: //')
            add_memory_md "Failed Approaches" "$approach" "Abandoned: $reason"
        fi
    done <<< "$dead_ends"
}
```

Call `sync_journey_to_memory` in the CONSOLIDATING state handling in `main_loop()`.

### Phase 4: Memory Query Command

Add optional command to both scripts for querying memory:

```bash
# Add to main() case statement
query-memory)
    ensure_memory_md
    if [[ -n "$2" ]]; then
        echo "Searching memory.md for: $2"
        echo ""
        search_memory_md "$2"
    else
        cat "$MEMORY_MD"
    fi
    ;;
```

Usage:
```bash
./loop_big_picture.sh query-memory "fft"
./loop_big_picture.sh query-memory "anti-pattern"
./loop.sh query-memory "socket"
```

## Files to Modify

1. **`loop.sh`**
   - Add `MEMORY_MD="${PROJECT_ROOT}/memory.md"` (after line 37)
   - Add `ensure_memory_md()`, `search_memory_md()`, `add_memory_md()` functions (after line 105)
   - Modify `generate_prompt()` to include memory instructions
   - Add `query-memory` command to `main()`

2. **`loop_big_picture.sh`**
   - Add `MEMORY_MD="${PROJECT_ROOT}/memory.md"` (after line 38)
   - Add `ensure_memory_md()`, `search_memory_md()`, `add_memory_md()`, `sync_journey_to_memory()` functions (after line 111)
   - Modify `generate_iteration_prompt()` to include memory instructions
   - Add sync call in CONSOLIDATING state handling
   - Add `query-memory` command to `main()`

## Verification

1. Run `./loop.sh status` or `./loop_big_picture.sh status` to ensure scripts still work
2. Start a journey/plan and verify `memory.md` is created/updated
3. After a journey completes or pivots, check `memory.md` has new entries
4. In a new journey, verify memory context appears in prompts
5. Test query command: `./loop_big_picture.sh query-memory "test"`

```bash
# Check memory file
cat memory.md

# Query for specific topics
./loop_big_picture.sh query-memory "failure"
./loop_big_picture.sh query-memory "anti-pattern"
```

## Notes

- **No jq dependency**: Markdown fallback works without jq (simpler than JSON parsing)
- **Memory file location**: Uses `memory.md` in project root (separate from MCP's `memory.json`)
- **Entity naming**: Use descriptive names like "fft-size-4096", "direct-socket-write"
- **MCP tools are auto-detected**: The agent will automatically use MCP tools when available; prompts instruct it to fall back to markdown when MCP is unavailable
- **Memory.md is version-controllable**: Can track in git to see evolution of learnings
