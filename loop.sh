#!/bin/bash

# Ralph Wiggum Loop Script for SOP (Sweet OS Platform)
# Autonomous plan-driven development using continuous iteration
#
# Usage:
#   ./loop.sh                       Find and work on active plan in loops/
#   ./loop.sh <plan-file>           Ingest plan and run loop
#   ./loop.sh -v <plan-file>        Verbose mode (show Claude output)
#   ./loop.sh status                Show status of all plans in loops/
#   ./loop.sh status <plan-file>    Show remaining tasks for specific plan
#   ./loop.sh --help                Show help
#
# Example:
#   ./loop.sh docs/plan-to-implement-feature.md
#   ./loop.sh -v docs/plan-to-implement-feature.md
#
# See: ralph.md for full documentation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Directory for working plan copies (tracked in git)
LOOPS_DIR="$PROJECT_ROOT/loops"

# Verbose mode (can be set via -v flag or VERBOSITY env var)
VERBOSE="${VERBOSE:-0}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[Ralph]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[Ralph]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[Ralph]${NC} $1"
}

log_error() {
    echo -e "${RED}[Ralph]${NC} $1"
}

log_debug() {
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "${CYAN}[Ralph:debug]${NC} $1"
    fi
}

# Claude command (uses z.ai config if available, otherwise falls back to claude)
CLAUDE_CMD=""
setup_claude() {
    local config="$HOME/.zai.json"

    # Check if jq is available
    if ! command -v jq >/dev/null 2>&1; then
        log_warning "jq not found, using default claude command"
        CLAUDE_CMD="claude"
        return
    fi

    # Check if config file exists
    if [ ! -f "$config" ]; then
        CLAUDE_CMD="claude"
        return
    fi

    # Read config from ~/.zai.json (same logic as zc() function)
    local api_url api_key haiku_model sonnet_model opus_model
    IFS=$'\t' read -r api_url api_key haiku_model sonnet_model opus_model < <(
        jq -r '[.apiUrl // "", .apiKey // "", .haikuModel // "", .sonnetModel // "", .opusModel // ""] | @tsv' "$config" 2>/dev/null || echo -e "\t\t\t\t"
    )

    # Validate config
    if [ -z "$api_url" ] || [ -z "$api_key" ]; then
        CLAUDE_CMD="claude"
        return
    fi

    # Set default models
    [ -z "$haiku_model" ] && haiku_model="glm-4.5-air"
    [ -z "$sonnet_model" ] && sonnet_model="glm-4.7"
    [ -z "$opus_model" ] && opus_model="glm-4.7"

    log_info "Using ~/.zai.json: endpoint=$api_url | haiku=$haiku_model | sonnet=$sonnet_model | opus=$opus_model"

    # Export Claude environment variables (safer than inline variable assignment)
    export ANTHROPIC_BASE_URL="$api_url"
    export ANTHROPIC_AUTH_TOKEN="$api_key"
    export ANTHROPIC_DEFAULT_HAIKU_MODEL="$haiku_model"
    export ANTHROPIC_DEFAULT_SONNET_MODEL="$sonnet_model"
    export ANTHROPIC_DEFAULT_OPUS_MODEL="$opus_model"
    CLAUDE_CMD="claude --dangerously-skip-permissions"
}

# Ensure loops directory exists
ensure_loops_dir() {
    if [ ! -d "$LOOPS_DIR" ]; then
        mkdir -p "$LOOPS_DIR"
        log_info "Created loops/ directory"
    fi
}

# Check if a plan file has any checkboxes (- [ ] or - [x])
has_checkboxes() {
    local file="$1"
    grep -qE '^\- \[([ x])\]' "$file" 2>/dev/null
}

# Convert a plan to checkbox format using AI
convert_plan_with_ai() {
    local source_file="$1"
    local dest_file="$2"
    local source_basename=$(basename "$source_file")

    log_info "Converting plan to checkbox format using AI..."

    # Create a temp file for the conversion prompt
    local temp_prompt=$(mktemp)
    cat > "$temp_prompt" << 'CONVERT_PROMPT'
Convert this planning document to use markdown checkboxes for task tracking.

Rules:
1. Keep all existing context, phases, and documentation
2. Convert action items and implementation steps to '- [ ]' format
3. Use '- [x]' for any items already marked as complete/done
4. Preserve the document structure and hierarchy
5. Don't add new content, just reformat existing tasks
6. For each task, add brief Explore/Implement/Verify hints where appropriate
7. Exploration and Implementation tasks should plan to, and use multiple sub-agents in parallel
8. Ensure that there is a DoD for each task, both a failure case and a success case. The failure case should break loop.sh.
9. If a task is redundant or erroneous, mark it with '- [-]' and record the reason.

Output the complete converted document.
CONVERT_PROMPT

    # Read source file and append to prompt
    echo "" >> "$temp_prompt"
    echo "---" >> "$temp_prompt"
    echo "" >> "$temp_prompt"
    echo "Source document:" >> "$temp_prompt"
    echo "" >> "$temp_prompt"
    cat "$source_file" >> "$temp_prompt"

    # Use Claude to convert
    local temp_output=$(mktemp)
    if cat "$temp_prompt" | $CLAUDE_CMD -p > "$temp_output" 2>&1; then
        # Extract just the converted content (skip any preamble)
        # Look for markdown content starting with # or - [ ]
        local in_content=0
        local found_content=0
        > "$dest_file"

        while IFS= read -r line; do
            # Start capturing when we see a header or checkbox
            case "$line" in
                \#*|"- ["*|"- [x]"*)
                    in_content=1
                    found_content=1
                    ;;
            esac

            if [ "$in_content" -eq 1 ]; then
                echo "$line" >> "$dest_file"
            fi
        done < "$temp_output"

        # If we didn't find structured content, just copy the whole output
        if [ "$found_content" -eq 0 ]; then
            cp "$temp_output" "$dest_file"
        fi

        log_success "Converted plan saved to: $dest_file"
    else
        log_warning "AI conversion failed, copying plan as-is"
        cp "$source_file" "$dest_file"
    fi

    rm -f "$temp_prompt" "$temp_output"
}

# Ingest a source plan file into loops/ directory
ingest_plan() {
    local source_file="$1"
    local source_basename=$(basename "$source_file")
    local dest_file="$LOOPS_DIR/$source_basename"

    # Check if source exists
    if [ ! -f "$source_file" ]; then
        log_error "Plan file not found: $source_file"
        exit 1
    fi

    # If already in loops/, use existing
    if [ -f "$dest_file" ]; then
        log_info "Using existing plan in loops/: $dest_file"
        log_info "(Delete it to re-ingest fresh: rm $dest_file)"
        PLAN_FILE="$dest_file"
        return 0
    fi

    # Check if source has checkboxes
    if has_checkboxes "$source_file"; then
        log_info "Plan has checkboxes, copying to loops/..."
        cp "$source_file" "$dest_file"
        log_success "Plan copied to: $dest_file"
    else
        # Need to convert using AI
        setup_claude
        convert_plan_with_ai "$source_file" "$dest_file"
    fi

    PLAN_FILE="$dest_file"
}

# Find the first active plan in loops/ with incomplete tasks
find_active_plan() {
    ensure_loops_dir

    local found_any=0
    local found_active=""

    # Look for markdown files in loops/
    shopt -s nullglob
    local plan_files=("$LOOPS_DIR"/*.md)
    shopt -u nullglob

    for plan_file in "${plan_files[@]}"; do
        found_any=1

        # Check for incomplete tasks
        if grep -q '^\- \[ \]' "$plan_file" 2>/dev/null; then
            found_active="$plan_file"
            break
        fi
    done

    if [ -n "$found_active" ]; then
        PLAN_FILE="$found_active"
        return 0
    fi

    # No active plans
    if [ "$found_any" -eq 0 ]; then
        log_error "No plans found in loops/ directory."
        echo ""
        log_info "To get started, use: ./loop.sh <plan-file>"
        log_info "Example: ./loop.sh docs/my-plan.md"
    else
        log_success "All plans in loops/ are complete!"
    fi
    return 1
}

# Count remaining tasks in plan file
count_remaining_tasks() {
    local file="${1:-$PLAN_FILE}"
    local count=$(grep -c '^\- \[ \]' "$file" 2>/dev/null || true)
    echo "${count:-0}"
}

# Count completed tasks in plan file (includes skipped tasks marked with - [-])
count_completed_tasks() {
    local file="${1:-$PLAN_FILE}"
    local done=$(grep -c '^\- \[x\]' "$file" 2>/dev/null || true)
    local skipped=$(grep -c '^\- \[-\]' "$file" 2>/dev/null || true)
    echo $(( ${done:-0} + ${skipped:-0} ))
}

# Count skipped tasks in plan file
count_skipped_tasks() {
    local file="${1:-$PLAN_FILE}"
    local count=$(grep -c '^\- \[-\]' "$file" 2>/dev/null || true)
    echo "${count:-0}"
}

# Show status of a single plan file
show_plan_status() {
    local file="$1"
    local basename=$(basename "$file")

    local remaining=$(count_remaining_tasks "$file")
    local completed=$(count_completed_tasks "$file")
    local total=$((remaining + completed))

    if [ "$total" -eq 0 ]; then
        echo -e "  ${YELLOW}$basename${NC} - No tasks found"
        return
    fi

    local pct=0
    if [ "$total" -gt 0 ]; then
        pct=$((completed * 100 / total))
    fi

    local skipped=$(count_skipped_tasks "$file")
    local skipped_note=""
    [ "$skipped" -gt 0 ] && skipped_note=" (${skipped} skipped)"

    if [ "$remaining" -eq 0 ]; then
        echo -e "  ${GREEN}$basename${NC} - ${completed}/${total} complete (${pct}%)${skipped_note} ${GREEN}DONE${NC}"
    else
        echo -e "  ${CYAN}$basename${NC} - ${completed}/${total} complete (${pct}%)${skipped_note} - ${remaining} remaining"
    fi
}

# Show status of all plans in loops/
show_all_status() {
    ensure_loops_dir

    log_info "=== Plans in loops/ ==="
    echo ""

    local found_any=0
    shopt -s nullglob
    local plan_files=("$LOOPS_DIR"/*.md)
    shopt -u nullglob

    for plan_file in "${plan_files[@]}"; do
        found_any=1
        show_plan_status "$plan_file"
    done

    if [ "$found_any" -eq 0 ]; then
        log_info "No plans found in loops/"
        echo ""
        log_info "To add a plan: ./loop.sh <plan-file>"
    fi
}

# Show detailed status of a specific plan
show_status() {
    if [ ! -f "$PLAN_FILE" ]; then
        log_error "Plan file not found: $PLAN_FILE"
        exit 1
    fi

    local remaining=$(count_remaining_tasks)
    local completed=$(count_completed_tasks)
    local total=$((remaining + completed))

    log_info "=== Plan Status: $(basename "$PLAN_FILE") ==="
    log_info "Tasks: $completed/$total complete ($remaining remaining)"
    echo ""

    if [ "$remaining" -gt 0 ]; then
        log_info "=== Next Tasks ==="
        grep -n '^\- \[ \]' "$PLAN_FILE" | head -5 | while read -r line; do
            echo "  $line"
        done
    else
        log_success "All tasks complete!"
    fi

    echo ""
    log_info "=== Recent Completed ==="
    grep -n '^\- \[x\]' "$PLAN_FILE" | tail -5 | while read -r line; do
        echo "  $line"
    done
}

# Generate the prompt for Claude
generate_prompt() {
    cat << 'PROMPT_EOF'
You are implementing tasks from a planning document. Follow these steps:

1. Read the plan file at the path specified in the environment.
2. Find the NEXT incomplete task (marked with '- [ ]').
3. Read the plan context to understand what needs to be done.
4. Implement the task following the plan's instructions.
5. Run the build and tests to verify:
   - cd build && cmake .. && make -j8 && ctest -j8
   OR to build the whole project and run all tests (to verify we haven't broken anything):
   - cd build && make -j8 && ctest -j4 --VV
6. If tests fail, fix the issues and re-run.
7. Update the plan file:
   a. Mark the task complete (change '- [ ]' to '- [x]').
   b. Record any new findings, surprises, or caveats under a '## Findings' section
      at the bottom of the plan (create it if it doesn't exist).
   c. If implementation revealed new work that wasn't in the plan, add it as new
      '- [ ]' tasks in the appropriate phase (or in a new '## Discovered Tasks' phase).
8. Commit and push your changes with a descriptive message.

IMPORTANT:
- Only complete ONE task per iteration.
- Always run the build and tests before committing.
- Each task has a Definition of Done (DoD) with a success case and a failure case.
- If the DoD SUCCESS case is met: mark the task complete ('- [x]') and commit.
- If the DoD FAILURE case is hit: do NOT mark the task complete. Write a clear failure
  message to stderr and exit with a non-zero exit code to stop loop.sh.
- If a task is REDUNDANT or ERRONEOUS (already done, contradicts another task, based on
  a false assumption, etc.): mark it skipped ('- [-]'), record the reason in the
  '## Findings' section, and continue to the next task without failing.
- If a task is blocked for a reason other than a DoD failure, note it in the plan and move on.
- Follow the existing code style and patterns in the codebase.
PROMPT_EOF
}

# Run continuous build loop
run_loop() {
    # Set up Claude command (uses ~/.zai.json if available)
    setup_claude

    if [ ! -f "$PLAN_FILE" ]; then
        log_error "Plan file not found: $PLAN_FILE"
        exit 1
    fi

    log_info "Starting Ralph loop with plan: $PLAN_FILE"
    log_info "Press Ctrl+C to stop"
    echo ""

    # Check if plan has any tasks
    local remaining=$(count_remaining_tasks)
    local completed=$(count_completed_tasks)
    local total=$((remaining + completed))

    if [ "$total" -eq 0 ]; then
        log_error "No task checkboxes found in plan file."
        echo ""
        log_info "The plan file must use markdown checkboxes for task tracking."
        log_info "Please add tasks to your plan using this format:"
        echo ""
        echo "  ## Phase 1: Phase Name"
        echo "  - [ ] Task description 1"
        echo "  - [ ] Task description 2"
        echo ""
        log_info "Run 'gemini -p \"Add checkboxes to $PLAN_FILE\"' for auto-conversion."
        exit 1
    fi

    # Check SSH keys work before starting
    log_info "Checking git access..."
    if ! git fetch origin 2>/dev/null; then
        log_warning "Git fetch failed - check SSH keys"
    fi

    # Main loop
    local iteration=0
    while true; do
        iteration=$((iteration + 1))

        # Check if all tasks are complete
        remaining=$(count_remaining_tasks)
        completed=$(count_completed_tasks)
        total=$((remaining + completed))

        if [ "$remaining" -eq 0 ]; then
            log_success "=== All $total tasks complete! ==="
            log_info "Exiting loop..."
            break
        fi

        log_info "=== Iteration $iteration ==="
        log_info "Progress: $completed/$total complete ($remaining remaining)"

        # Show next task
        local next_task=$(grep '^\- \[ \]' "$PLAN_FILE" | head -1)
        log_info "Next task: $next_task"
        echo ""

        # Create temp file with prompt + plan context
        local temp_prompt=$(mktemp)
        {
            generate_prompt
            echo ""
            echo "---"
            echo ""
            echo "PLAN_FILE=$PLAN_FILE"
            echo ""
            echo "Here is the current state of the plan file:"
            echo ""
            cat "$PLAN_FILE"
        } > "$temp_prompt"

        # Run Claude with the prompt
        local exit_code=0
        if [ "$VERBOSE" -eq 1 ]; then
            log_info "--- Claude Output (verbose mode) ---"
            cat "$temp_prompt" | $CLAUDE_CMD -p
            exit_code=$?
            log_info "--- End Claude Output ---"
        else
            log_info "(Use -v flag to see Claude output in real-time)"
            cat "$temp_prompt" | $CLAUDE_CMD -p > /tmp/ralph_output.txt 2>&1
            exit_code=$?

            # Show output if failed
            if [ -f /tmp/ralph_output.txt ]; then
                [ $exit_code -ne 0 ] && cat /tmp/ralph_output.txt
                rm -f /tmp/ralph_output.txt
            fi
        fi

        # DoD failure case: Claude signalled failure — break the loop
        if [ $exit_code -ne 0 ]; then
            log_error "Task failed DoD failure case (exit code $exit_code). Stopping loop."
            rm -f "$temp_prompt"
            exit 1
        fi

        # Cleanup temp file
        rm -f "$temp_prompt"

        # Push changes if any
        local current_branch=$(git branch --show-current)
        if [ -n "$current_branch" ]; then
            log_info "Pushing to origin/$current_branch..."
            git push origin "$current_branch" || log_warning "Git push failed (may be no changes)"
        fi

        # Brief pause between iterations
        sleep 2
        echo ""
    done

    log_success "Ralph loop completed successfully!"
}

# Show usage
show_usage() {
    cat << EOF
${BLUE}Ralph Wiggum Loop${NC} - Autonomous plan-driven development for SOP

${GREEN}Usage:${NC}
  $0                           Find and work on active plan in loops/
  $0 <plan-file>               Ingest plan and run loop
  $0 -v <plan-file>            Verbose mode (show Claude output in real-time)
  $0 status                    Show status of all plans in loops/
  $0 status <plan-file>        Show remaining tasks for specific plan
  $0 --help                    Show this help

${GREEN}Options:${NC}
  -v, --verbose     Show Claude output in real-time
  VERBOSITY=1       Alternative way to enable verbose mode

${GREEN}Working Directory:${NC}
  Plans are managed in the loops/ directory:
  - Running with a plan file copies it to loops/ (with checkbox conversion if needed)
  - Running without args finds the first active plan in loops/
  - Delete a plan from loops/ to re-ingest fresh from source

${GREEN}Examples:${NC}
  # Ingest a plan and start working on it
  $0 docs/plan-to-fix-fft-multi.md

  # Continue working on active plan (finds first with incomplete tasks)
  $0

  # Show status of all plans in loops/
  $0 status

  # Run with verbose output
  $0 -v docs/plan-to-fix-fft-multi.md

  # Reset a plan (delete to re-ingest fresh)
  rm loops/plan-to-fix-fft-multi.md
  $0 docs/plan-to-fix-fft-multi.md

${GREEN}Plan File Format:${NC}
  Plans are markdown files with task checkboxes:
    - [ ] Incomplete task
    - [x] Completed task

${GREEN}Build Commands (SOP):${NC}
  Build:  cd build && cmake .. && make -j8
  Test:   ./sweet_osal_test

${YELLOW}Requirements:${NC}
  - ~/.zai.json configured with z.ai credentials
  - Build directory configured (build/)

${YELLOW}Documentation:${NC} See ralph.md for full details
EOF
}

# Main entry point
main() {
    # Parse arguments
    local plan_arg=""
    local status_mode=0

    while [[ $# -gt 0 ]]; do
        case "$1" in
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            status)
                status_mode=1
                shift
                # Check if there's a plan file argument after status
                if [[ $# -gt 0 ]] && [[ "$1" != -* ]]; then
                    plan_arg="$1"
                    shift
                fi
                ;;
            -h|--help|help)
                show_usage
                exit 0
                ;;
            -*)
                log_error "Unknown option: $1"
                echo ""
                show_usage
                exit 1
                ;;
            *)
                # Plan file argument
                if [ -z "$plan_arg" ]; then
                    plan_arg="$1"
                fi
                shift
                ;;
        esac
    done

    # Handle status mode
    if [ "$status_mode" -eq 1 ]; then
        if [ -n "$plan_arg" ]; then
            # Status for specific plan
            # Resolve to absolute path if relative
            if [[ "$plan_arg" != /* ]]; then
                PLAN_FILE="$PROJECT_ROOT/$plan_arg"
            else
                PLAN_FILE="$plan_arg"
            fi
            show_status
        else
            # Status for all plans
            show_all_status
        fi
        exit 0
    fi

    # Set plan file
    if [ -n "$plan_arg" ]; then
        # User provided a plan file - ingest it
        ensure_loops_dir

        # Resolve to absolute path if relative
        if [[ "$plan_arg" != /* ]]; then
            plan_arg="$PROJECT_ROOT/$plan_arg"
        fi

        ingest_plan "$plan_arg"
    else
        # No plan file provided - find active plan in loops/
        if ! find_active_plan; then
            exit 0
        fi
    fi

    run_loop
}

main "$@"
