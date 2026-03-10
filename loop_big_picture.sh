#!/bin/bash
#
# loop_big_picture.sh - Autonomous Goal-Driven Development Loop
#
# An enhanced R&D agent that works toward ambitious goals without pre-defined plans.
# Unlike Ralph (task executor), Big Picture is a research and development agent.
#
# Usage:
#   ./loop_big_picture.sh "goal description"     # Start new journey
#   ./loop_big_picture.sh                        # Continue active journey
#   ./loop_big_picture.sh status                 # Show all journeys
#   ./loop_big_picture.sh pivot                  # Force pivot to next approach
#   ./loop_big_picture.sh reflect                # Force reflection phase
#   ./loop_big_picture.sh hint "message"         # Add user hint to journey
#   ./loop_big_picture.sh rollback [N]           # Rollback to checkpoint N (default: last)
#

set -euo pipefail

# ============================================================================
# CONFIGURATION
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"
JOURNEY_DIR="${PROJECT_ROOT}/journey"
PROTOTYPES_DIR="${PROJECT_ROOT}/prototypes"
CONFIG_FILE="${PROJECT_ROOT}/.big_picture.conf"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

# ============================================================================
# DEFAULT CONFIGURATION
# ============================================================================

BUILD_COMMAND="${BUILD_COMMAND:-cd build && cmake .. && make -j8}"
TEST_COMMAND="${TEST_COMMAND:-./sweet_osal_test}"
ALL_TESTS_COMMAND="${ALL_TESTS_COMMAND:-cd build && ctest -j8}"
GUARDRAIL_TESTS="${GUARDRAIL_TESTS:-sweet_osal_test osal_test utils_test}"
BENCHMARK_COMMAND="${BENCHMARK_COMMAND:-}"

# Performance thresholds
CPU_THRESHOLD="${CPU_THRESHOLD:-+20%}"
LATENCY_THRESHOLD="${LATENCY_THRESHOLD:-+10ms}"
ACCURACY_THRESHOLD="${ACCURACY_THRESHOLD:-0%}"

# Project metadata
PROJECT_NAME="${PROJECT_NAME:-SOP}"
KEY_FILES="${KEY_FILES:-CLAUDE.md README.md}"

# Dead-end detection thresholds
MAX_STALE_ITERATIONS="${MAX_STALE_ITERATIONS:-3}"
MIN_PROGRESS_PERCENT="${MIN_PROGRESS_PERCENT:-5}"

# Claude command
CLAUDE_CMD=""

# Claude model
CLAUDE_MODEL="${CLAUDE_MODEL:-claude-opus-4-6}"
CLAUDE_API_BASE="${CLAUDE_API_BASE:-https://api.anthropic.com}"

# Git checkpointing
CHECKPOINT_PREFIX="${CHECKPOINT_PREFIX:-journey}"

# Maximum iterations before giving up
MAX_ITERATIONS="${MAX_ITERATIONS:-100}"

# Verbose mode
VERBOSE="${VERBOSE:-false}"

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_phase() {
    echo -e "${PURPLE}[PHASE]${NC} $*"
}

log_state() {
    echo -e "${CYAN}[STATE]${NC} $*"
}

log_debug() {
    if [[ "${VERBOSE}" == "true" ]]; then
        echo -e "${GRAY}[DEBUG]${NC} $*"
    fi
}

# Print usage information
usage() {
    cat << EOF
Usage: $0 [COMMAND] [ARGUMENTS]

An autonomous R&D agent that works toward ambitious goals without pre-defined plans.

Commands:
  $0 "goal description"     Start a new journey with the given goal
  $0                        Continue the active journey
  $0 status                 Show status of all journeys
  $0 pivot                  Force pivot to next approach
  $0 reflect                Force reflection phase
  $0 hint "message"         Add a user hint to the journey
  $0 rollback [N]           Rollback to checkpoint N (default: last checkpoint)
  $0 list-checkpoints       List all checkpoints for current journey

Options:
  -v, --verbose             Enable verbose output
  -h, --help                Show this help message

States:
  RESEARCHING    - Exploring codebase, web, literature for approaches
  PLANNING       - Generating/updating approach & milestones
  PROTOTYPING    - Implementing isolated prototype before integration
  EXECUTING      - Integrating prototype into codebase
  REFLECTING     - Evaluating progress, detecting dead ends
  PIVOTING       - Switching to next approach
  WAITING_FOR_USER - Awaiting clarification or user input
  CONSOLIDATING  - Cleaning up, documenting, final verification
  COMPLETE       - Goal achieved, journey finished
  BLOCKED        - Blocked by external dependency or error

Environment:
  JOURNEY_DIR    Directory for journey files (default: ./journey)
  PROTOTYPES_DIR Directory for prototype files (default: ./prototypes)

Configuration:
  .big_picture.conf  Optional config file in project root

Examples:
  $0 "Improve low-frequency detection using ML"
  $0 hint "Try harmonic product spectrum first"
  $0 rollback 3

EOF
}

# Load configuration file if it exists
load_config() {
    if [[ -f "${CONFIG_FILE}" ]]; then
        log_debug "Loading configuration from ${CONFIG_FILE}"
        # Source the config file, allowing overrides
        source "${CONFIG_FILE}"
    fi
}

# Ensure required directories exist
ensure_directories() {
    mkdir -p "${JOURNEY_DIR}"
    mkdir -p "${PROTOTYPES_DIR}"
}

# ============================================================================
# JOURNEY FILE FUNCTIONS
# ============================================================================

# Get journey file path from name
journey_file() {
    local name="$1"
    echo "${JOURNEY_DIR}/${name}.journey.md"
}

# Generate a safe journey name from goal
sanitize_journey_name() {
    local goal="$1"
    # Convert to lowercase, replace spaces with hyphens, remove special chars
    echo "${goal}" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' | sed 's/[^a-z0-9-]//g' | head -c 50
}

# Create a new journey file
create_journey_file() {
    local goal="$1"
    local name
    name=$(sanitize_journey_name "${goal}")
    local journey_path
    journey_path=$(journey_file "${name}")

    if [[ -f "${journey_path}" ]]; then
        log_error "Journey already exists: ${name}" >&2
        log_info "Use '$0 status' to see existing journeys" >&2
        exit 1
    fi

    cat > "${journey_path}" << EOF
# Journey: ${goal}

## Meta

- Goal: ${goal}
- State: RESEARCHING
- Started: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
- Current Approach: TBD
- Progress: 0%

## Approaches

### Approach 1: TBD

- Status: PENDING
- Reason: TBD
- Iterations: 0

## Current Approach Detail

### Approach 1: TBD

- Hypothesis: TBD
- Milestones:
  - [ ] Research and identify viable approaches

## Guardrails

- [PENDING] All baseline tests must pass (no regression)
- [PENDING] Build must succeed after each commit

## Baseline Metrics

| Metric            | Baseline | Current | Threshold   |
| ----------------- | -------- | ------- | ----------- |
| CPU Usage         | TBD      | TBD     | ${CPU_THRESHOLD} |
| Latency           | TBD      | TBD     | ${LATENCY_THRESHOLD} |
| Test Pass Rate    | TBD      | TBD     | No decrease |

## User Hints

*(No user hints yet)*

## Generated Artifacts

*(No artifacts yet)*

## Learnings Log

*(Journey started)*

## Dead Ends

*(No dead ends yet)*

## Anti-Patterns (Semantic Constraints)

*(No anti-patterns identified yet)*

## Pending Questions (WAITING_FOR_USER)

*(No pending questions)*

## Next Steps (Auto-Generated)

1. Explore codebase for relevant existing code
2. Research techniques and approaches
3. Identify 2-4 viable approaches

## Checkpoints

| ID | Tag                | Date       | Description                    |
| -- | ------------------ | ---------- | ------------------------------ |
| 0  | initial            | $(date -u +"%Y-%m-%d") | Journey start point |
EOF

    log_success "Created journey: ${name}" >&2
    echo "${journey_path}"
}

# Parse journey state
get_journey_state() {
    local journey_file="$1"
    grep "^- State:" "${journey_file}" | sed 's/.*State: //'
}

# Parse journey goal
get_journey_goal() {
    local journey_file="$1"
    grep "^- Goal:" "${journey_file}" | sed 's/.*Goal: //'
}

# Parse journey progress
get_journey_progress() {
    local journey_file="$1"
    grep "^- Progress:" "${journey_file}" | sed 's/.*Progress: //'
}

# Portable in-place sed helper (works on BSD and GNU sed)
sed_in_place() {
    local expr="$1"
    local file="$2"
    sed -i.bak "$expr" "$file"
    rm -f "${file}.bak"
}

# Update journey state
set_journey_state() {
    local journey_file="$1"
    local new_state="$2"
    sed_in_place "s/^- State: .*/- State: ${new_state}/" "${journey_file}"
}

# Update journey progress
set_journey_progress() {
    local journey_file="$1"
    local progress="$2"
    sed_in_place "s/^- Progress: .*/- Progress: ${progress}%/" "${journey_file}"
}

# Update current approach
set_current_approach() {
    local journey_file="$1"
    local approach="$2"
    sed_in_place "s/^- Current Approach: .*/- Current Approach: ${approach}/" "${journey_file}"
}

# Add learning to journey
add_learning() {
    local journey_file="$1"
    local learning="$2"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%d")
    local section_line
    section_line=$(grep -n "^## Learnings Log" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        # Insert after the section header
        local insert_line=$((section_line + 2))
        sed -i '' "${insert_line}i\\
- ${timestamp}: ${learning}
" "${journey_file}"
    fi
}

# Add dead end to journey
add_dead_end() {
    local journey_file="$1"
    local approach="$2"
    local reason="$3"
    local learnings="$4"

    # Find the Dead Ends section
    local section_line
    section_line=$(grep -n "^## Dead Ends" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        local insert_line=$((section_line + 2))
        local entry="### ${approach}\n\n- Status: ABANDONED\n- Reason: ${reason}\n- Learnings: ${learnings}\n- Abandoned: $(date -u +"%Y-%m-%d")\n"
        # Use a temp file for multi-line insert
        local temp_file
        temp_file=$(mktemp)
        head -n "${insert_line}" "${journey_file}" > "${temp_file}"
        echo -e "${entry}" >> "${temp_file}"
        tail -n +$((insert_line + 1)) "${journey_file}" >> "${temp_file}"
        mv "${temp_file}" "${journey_file}"
    fi
}

# Add anti-pattern to journey
add_anti_pattern() {
    local journey_file="$1"
    local pattern="$2"
    local description="$3"

    local section_line
    section_line=$(grep -n "^## Anti-Patterns" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        local insert_line=$((section_line + 2))
        sed -i '' "${insert_line}i\\
- **${pattern}**: ${description}
" "${journey_file}"
    fi
}

# Add user hint to journey
add_user_hint() {
    local journey_file="$1"
    local hint="$2"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%d %H:%M:%S UTC")

    local section_line
    section_line=$(grep -n "^## User Hints" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        local insert_line=$((section_line + 1))

        # Check if hints section is empty
        local next_line
        next_line=$(sed -n "$((insert_line + 1))p" "${journey_file}")
        if [[ "${next_line}" == *"*(No user hints yet)"* ]]; then
            # Replace the placeholder
            sed -i '' "$((insert_line + 1))s/.*/- ${timestamp}: \"${hint}\"/" "${journey_file}"
        else
            # Add new hint
            sed -i '' "${insert_line}a\\
- ${timestamp}: \"${hint}\"
" "${journey_file}"
        fi
    fi
}

# Add pending question
add_pending_question() {
    local journey_file="$1"
    local question="$2"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%d")

    local section_line
    section_line=$(grep -n "^## Pending Questions" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        local insert_line=$((section_line + 1))

        # Check if section is empty
        local next_line
        next_line=$(sed -n "$((insert_line + 1))p" "${journey_file}")
        if [[ "${next_line}" == *"*(No pending questions)"* ]]; then
            sed -i '' "$((insert_line + 1))s/.*/- [ ] ${timestamp}: ${question}/" "${journey_file}"
        else
            sed -i '' "${insert_line}a\\
- [ ] ${timestamp}: ${question}
" "${journey_file}"
        fi
    fi
}

# Add checkpoint to journey
add_checkpoint() {
    local journey_file="$1"
    local tag="$2"
    local description="$3"
    local id
    # Compute max existing checkpoint ID from first column (excluding header/separator)
    id=$(grep "^## Checkpoints" -A 20 "${journey_file}" | grep "^|" | tail -n +2 | grep -E "^\| [0-9]+" | sed 's/^\| \([0-9]*\).*/\1/' | sort -n | tail -1)
    id=$((id + 1))

    local section_line
    section_line=$(grep -n "^## Checkpoints" "${journey_file}" | cut -d: -f1)

    if [[ -n "${section_line}" ]]; then
        local insert_line=$((section_line + 3))
        local timestamp
        timestamp=$(date -u +"%Y-%m-%d")
        sed_in_place "${insert_line}a\\
| ${id} | ${tag} | ${timestamp} | ${description} |
" "${journey_file}"
    fi
}

# Get list of all journeys
list_journeys() {
    local journeys=()
    while IFS= read -r -d '' file; do
        journeys+=("${file}")
    done < <(find "${JOURNEY_DIR}" -name "*.journey.md" -print0 2>/dev/null)

    if [[ ${#journeys[@]} -eq 0 ]]; then
        log_info "No journeys found in ${JOURNEY_DIR}"
        return
    fi

    echo -e "\n${CYAN}=== Active Journeys ===${NC}\n"

    # Use a for loop with proper handling
    local journey
    for journey in "${journeys[@]:-}"; do
        local name
        name=$(basename "${journey}" .journey.md)
        local state
        state=$(get_journey_state "${journey}")
        local goal
        goal=$(get_journey_goal "${journey}")
        local progress
        progress=$(get_journey_progress "${journey}")

        # Color code by state
        local state_color="${NC}"
        case "${state}" in
            COMPLETE) state_color="${GREEN}" ;;
            BLOCKED|WAITING_FOR_USER) state_color="${YELLOW}" ;;
            PIVOTING) state_color="${RED}" ;;
            *) state_color="${CYAN}" ;;
        esac

        printf "${BLUE}${name}${NC} [${state_color}${state}${NC}] ${progress}%\n"
        printf "  Goal: ${goal}\n\n"
    done
}

# Find active journey (first non-complete journey)
find_active_journey() {
    # Collect journeys sorted by modification time (most recent first)
    local sorted_journeys=()
    while IFS= read -r -d '' file; do
        sorted_journeys+=("${file}")
    done < <(
        find "${JOURNEY_DIR}" -name "*.journey.md" -printf '%T@ %p\0' 2>/dev/null \
            | sort -z -nr \
            | cut -z -d' ' -f2-
    )

    # Check if any journeys were found
    if [[ ${#sorted_journeys[@]} -eq 0 ]]; then
        return 1
    fi

    for journey in "${sorted_journeys[@]}"; do
        local state
        state=$(get_journey_state "${journey}")
        if [[ "${state}" != "COMPLETE" ]]; then
            echo "${journey}"
            return 0
        fi
    done

    return 1
}

# Show detailed status of a journey
show_journey_status() {
    local journey_file="$1"

    if [[ ! -f "${journey_file}" ]]; then
        log_error "Journey file not found: ${journey_file}"
        return 1
    fi

    local name
    name=$(basename "${journey_file}" .journey.md)

    echo -e "\n${CYAN}=== Journey: ${name} ===${NC}\n"

    # Meta section
    echo -e "${YELLOW}Meta:${NC}"
    grep "^- " "${journey_file}" | sed 's/^-/  /'

    # Checkpoints
    echo -e "\n${YELLOW}Checkpoints:${NC}"
    grep -A 20 "^## Checkpoints" "${journey_file}" | grep "^|" | tail -n +2

    # Current approach
    echo -e "\n${YELLOW}Current Approach:${NC}"
    grep -A 10 "^## Current Approach Detail" "${journey_file}" | head -n 10

    # Pending questions
    local pending
    pending=$(grep -A 20 "^## Pending Questions" "${journey_file}" | grep "^- \[ \]")
    if [[ -n "${pending}" ]]; then
        echo -e "\n${YELLOW}Pending Questions:${NC}"
        echo "${pending}"
    fi

    # Anti-patterns
    local anti_patterns
    anti_patterns=$(grep -A 20 "^## Anti-Patterns" "${journey_file}" | grep "^- \*\*")
    if [[ -n "${anti_patterns}" ]]; then
        echo -e "\n${YELLOW}Anti-Patterns:${NC}"
        echo "${anti_patterns}"
    fi
}

# ============================================================================
# CHECKPOINT FUNCTIONS
# ============================================================================

# Create a git checkpoint
create_checkpoint() {
    local journey_name="$1"
    local milestone_number="$2"
    local description="$3"

    local tag="${CHECKPOINT_PREFIX}-${journey_name}-milestone-${milestone_number}"

    # Check if tag already exists
    if git rev-parse "${tag}" >/dev/null 2>&1; then
        log_warning "Tag ${tag} already exists, skipping"
        return
    fi

    # Create annotated tag
    git tag -a "${tag}" -m "Journey checkpoint: ${description}" HEAD

    log_success "Created checkpoint: ${tag}"
}

# Rollback to a checkpoint
rollback_to_checkpoint() {
    local journey_file="$1"
    local checkpoint_id="${2:-}"

    if [[ -z "${checkpoint_id}" ]]; then
        # Get the latest checkpoint (highest ID)
        checkpoint_id=$(grep -A 20 "^## Checkpoints" "${journey_file}" | grep "^|" | tail -n +2 | tail -n 1 | cut -d'|' -f1 | tr -d ' ')
        if [[ -z "${checkpoint_id}" ]]; then
            log_error "No checkpoints found"
            return 1
        fi
    fi

    # Get the tag for this checkpoint
    local tag
    tag=$(grep -A 20 "^## Checkpoints" "${journey_file}" | grep "^| ${checkpoint_id} " | cut -d'|' -f2 | tr -d ' ')

    if [[ -z "${tag}" ]]; then
        log_error "Checkpoint ID ${checkpoint_id} not found"
        return 1
    fi

    log_warning "Rolling back to checkpoint: ${tag}"
    log_info "This will reset your working directory to the state at ${tag}"
    echo -n "Continue? [y/N] "
    read -r response

    if [[ "${response}" =~ ^[Yy]$ ]]; then
        git reset --hard "${tag}"
        log_success "Rolled back to ${tag}"
    else
        log_info "Rollback cancelled"
    fi
}

# List checkpoints for a journey
list_checkpoints() {
    local journey_file="$1"

    echo -e "\n${CYAN}=== Checkpoints ===${NC}\n"

    grep -A 50 "^## Checkpoints" "${journey_file}" | grep "^|" | while read -r line; do
        if [[ "${line}" != *"---"* ]]; then
            echo "${line}"
        fi
    done
}

# ============================================================================
# BASELINE AND GUARDRAIL FUNCTIONS
# ============================================================================

# Establish baseline metrics
establish_baselines() {
    local journey_file="$1"

    log_phase "Establishing baseline metrics..."

    # Run tests to get baseline
    log_info "Running baseline tests..."

    # Store baseline in journey file
    local baseline_date
    baseline_date=$(date -u +"%Y-%m-%d")

    # Update baselines section: only modify the Test Pass Rate row
    sed_in_place "s/^| Test Pass Rate[[:space:]]*|.*$/| Test Pass Rate    | ${baseline_date}      | ${baseline_date}     | No decrease |/" "${journey_file}"

    # Count tests
    local test_count=0
    local passed_count=0

    if eval "${ALL_TESTS_COMMAND} 2>&1" | grep -q "tests passed"; then
        passed_count=$(eval "${ALL_TESTS_COMMAND} 2>&1" | grep -oE "[0-9]+ tests passed" | grep -oE "[0-9]+")
    fi

    log_success "Baseline established: ${passed_count} tests passing"
    add_learning "${journey_file}" "Baseline: ${passed_count} tests passing"
}

# Run guardrails
run_guardrails() {
    local journey_file="$1"

    log_debug "Running guardrails..."

    # Build
    log_debug "Building..."
    if ! eval "${BUILD_COMMAND} >/dev/null 2>&1"; then
        log_error "Build failed"
        return 1
    fi

    # Run tests
    log_debug "Running guardrail tests..."
    for test in ${GUARDRAIL_TESTS}; do
        # Build the command for this specific guardrail test.
        # If TEST_COMMAND is set, treat it as a prefix; otherwise treat each
        # entry in GUARDRAIL_TESTS as a full command.
        local test_cmd
        if [[ -n "${TEST_COMMAND:-}" ]]; then
            test_cmd="${TEST_COMMAND} ${test}"
        else
            test_cmd="${test}"
        fi

        if ! eval "${test_cmd} >/dev/null 2>&1"; then
            log_error "Test ${test} failed"
            return 1
        fi
    done

    return 0
}

# ============================================================================
# CLAUDE API FUNCTIONS
# ============================================================================

# Setup Claude command (similar to loop.sh)
setup_claude() {
    local config="$HOME/.zai.json"

    # Check if jq is available
    if ! command -v jq >/dev/null 2>&1; then
        log_debug "jq not found, using default claude command"
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

    log_debug "Using ~/.zai.json: endpoint=$api_url | haiku=$haiku_model | sonnet=$sonnet_model | opus=$opus_model"

    # Export Claude environment variables (safer than inline variable assignment)
    export ANTHROPIC_BASE_URL="$api_url"
    export ANTHROPIC_AUTH_TOKEN="$api_key"
    export ANTHROPIC_DEFAULT_HAIKU_MODEL="$haiku_model"
    export ANTHROPIC_DEFAULT_SONNET_MODEL="$sonnet_model"
    export ANTHROPIC_DEFAULT_OPUS_MODEL="$opus_model"
    CLAUDE_CMD="claude --dangerously-skip-permissions"
}

# ============================================================================
# MAIN LOOP FUNCTIONS
# ============================================================================

# Generate the prompt for the current iteration
generate_iteration_prompt() {
    local journey_file="$1"
    local journey_content
    journey_content=$(cat "${journey_file}")

    cat << 'EOF'
You are an autonomous R&D agent working toward a high-level goal.

## Your Journey

EOF
    echo "${journey_content}"

    cat << 'EOF'

## Your Task

Based on the journey state, perform the appropriate phase:

### If RESEARCHING:

- Explore the codebase for relevant existing code
- **Web search** for techniques, papers, best practices, tutorials
- Identify 2-4 viable approaches
- Document findings in journey file with sources
- Transition to PLANNING

### If PLANNING:

- Choose the best approach based on research
- **Check Anti-Patterns section** - don't select approaches that violate known constraints
- Create/update guardrails if not exists
- Break approach into milestones
- For complex algorithms: Transition to PROTOTYPING
- For simple changes: Transition directly to EXECUTING

### If PROTOTYPING:

- Implement algorithm in isolated file (e.g., `prototypes/algorithm_test.cpp`)
- Test against baseline metrics
- If prototype works: Transition to EXECUTING
- If prototype fails: Document why, Transition to PIVOTING

### If EXECUTING:

- Implement the current milestone
- Run guardrails (tests, build)
- Update journey file with progress
- Transition to REFLECTING

### If REFLECTING:

- Evaluate: Is this approach working?
- Check: Are we closer to the goal?
- **Check: Do we need clarification?** → WAITING_FOR_USER
- **Record any new Anti-Patterns discovered**
- Decide:
  - If goal achieved → CONSOLIDATING
  - If making progress → continue EXECUTING
  - If need expert input → WAITING_FOR_USER
  - If dead end → PIVOTING
- Update journey file

### If WAITING_FOR_USER:

- Write clear question to `## Pending Questions` section
- Wait for user response via `hint` command
- After answer: Resume from PLANNING or EXECUTING

### If CONSOLIDATING:

- Remove debug prints, TRACE calls, scaffolding code
- Delete prototype files or unused training data
- Update README.md / CLAUDE.md with new architecture
- Run **full test suite** (not just guardrails)
- Verify all baseline metrics still pass
- Transition to COMPLETE

### If PIVOTING:

- Document why current approach failed
- **Record semantic Anti-Patterns** (e.g., "FFT > 4096 violates latency constraint")
- **Check Anti-Patterns before selecting new approach**
- Choose next best approach from research
- Reset milestones
- Transition to PLANNING

## Important Rules

- ALWAYS run guardrails before committing
- If tests fail, fix before continuing
- Document all learnings in the journey file
- Never repeat a failed approach
- One milestone per iteration
- Commit and push after each successful iteration
- Update journey state and progress after each phase
EOF
}

# Execute one iteration of the loop
run_iteration() {
    local journey_file="$1"
    local journey_name
    journey_name=$(basename "${journey_file}" .journey.md)

    local state
    state=$(get_journey_state "${journey_file}")

    log_state "Current state: ${state}"

    log_phase "Running iteration for ${journey_name}..."

    # Create temp file with prompt + journey context
    local temp_prompt=$(mktemp)
    {
        generate_iteration_prompt "${journey_file}"
        echo ""
        echo "---"
        echo ""
        echo "JOURNEY_FILE=${journey_file}"
        echo ""
        echo "Current working directory: $(pwd)"
        echo ""
    } > "$temp_prompt"

    # Run Claude with the prompt
    local exit_code=0
    log_info "Calling Claude to perform iteration..."

    if cat "$temp_prompt" | $CLAUDE_CMD -p 2>&1; then
        exit_code=0
    else
        exit_code=$?
    fi

    rm -f "$temp_prompt"

    if [ $exit_code -ne 0 ]; then
        log_error "Claude returned exit code $exit_code"
        set_journey_state "${journey_file}" "BLOCKED"
        return 1
    fi

    # Re-read journey state after Claude's updates
    local new_state
    new_state=$(get_journey_state "${journey_file}")
    log_state "New state: ${new_state}"

    log_info "Iteration complete"
}

# Main loop
main_loop() {
    local journey_file="$1"
    local journey_name
    journey_name=$(basename "${journey_file}" .journey.md)
    local iteration=0

    # Set up Claude command
    setup_claude

    log_info "Starting Big Picture loop for: ${journey_name}"
    log_info "Journey file: ${journey_file}"

    # Establish baselines if not already done
    if grep -q "| Baseline | TBD" "${journey_file}"; then
        establish_baselines "${journey_file}"
    fi

    while [[ ${iteration} -lt ${MAX_ITERATIONS} ]]; do
        iteration=$((iteration + 1))
        log_phase "Iteration ${iteration}/${MAX_ITERATIONS}"

        local state
        state=$(get_journey_state "${journey_file}")
        local progress
        progress=$(get_journey_progress "${journey_file}")
        log_info "Progress: ${progress} | State: ${state}"

        case "${state}" in
            COMPLETE)
                log_success "Journey complete!"
                break
                ;;
            BLOCKED|WAITING_FOR_USER)
                log_warning "Journey is ${state}. Use '$0 hint \"message\"' to provide input."
                log_info "Current pending questions:"
                grep -A 20 "^## Pending Questions" "${journey_file}" | grep "^- \[ \]"
                break
                ;;
            *)
                run_iteration "${journey_file}"

                # Ensure any changes Claude made are committed and pushed
                local current_branch
                current_branch=$(git branch --show-current)
                if [[ -n "${current_branch}" ]]; then
                    if ! git diff --quiet || ! git diff --cached --quiet; then
                        log_warning "Uncommitted changes detected after iteration - committing now"
                        git add -A
                        git commit -m "chore(journey): auto-commit changes from iteration ${iteration} [${journey_name}]" || true
                    fi
                    log_info "Pushing to origin/${current_branch}..."
                    git push origin "${current_branch}" || log_warning "Git push failed (may be no changes)"
                fi
                ;;
        esac

        # Small delay to prevent overwhelming the API
        sleep 1
    done

    if [[ ${iteration} -ge ${MAX_ITERATIONS} ]]; then
        log_warning "Maximum iterations reached (${MAX_ITERATIONS})"
    fi
}

# ============================================================================
# COMMAND HANDLERS
# ============================================================================

# Handle hint command
handle_hint() {
    local hint="$1"

    local active_journey
    active_journey=$(find_active_journey)

    if [[ -z "${active_journey}" ]]; then
        log_error "No active journey found"
        log_info "Create one with: $0 \"your goal\""
        exit 1
    fi

    add_user_hint "${active_journey}" "${hint}"
    log_success "Hint added to journey"

    # If journey was waiting, unpause it
    local state
    state=$(get_journey_state "${active_journey}")
    if [[ "${state}" == "WAITING_FOR_USER" ]]; then
        log_info "Journey was waiting - resuming..."
        set_journey_state "${active_journey}" "PLANNING"
    fi
}

# Handle pivot command
handle_pivot() {
    local active_journey
    active_journey=$(find_active_journey)

    if [[ -z "${active_journey}" ]]; then
        log_error "No active journey found"
        exit 1
    fi

    log_info "Forcing pivot for active journey"
    set_journey_state "${active_journey}" "PIVOTING"
    log_success "Journey state set to PIVOTING"
}

# Handle reflect command
handle_reflect() {
    local active_journey
    active_journey=$(find_active_journey)

    if [[ -z "${active_journey}" ]]; then
        log_error "No active journey found"
        exit 1
    fi

    log_info "Forcing reflection for active journey"
    set_journey_state "${active_journey}" "REFLECTING"
    log_success "Journey state set to REFLECTING"
}

# Handle rollback command
handle_rollback() {
    local checkpoint_id="$1"

    local active_journey
    active_journey=$(find_active_journey)

    if [[ -z "${active_journey}" ]]; then
        log_error "No active journey found"
        exit 1
    fi

    rollback_to_checkpoint "${active_journey}" "${checkpoint_id}"
}

# Handle list-checkpoints command
handle_list_checkpoints() {
    local active_journey
    active_journey=$(find_active_journey)

    if [[ -z "${active_journey}" ]]; then
        log_error "No active journey found"
        exit 1
    fi

    list_checkpoints "${active_journey}"
}

# Handle status command
handle_status() {
    ensure_directories
    list_journeys

    # Show details for active journey
    local active_journey
    active_journey=$(find_active_journey)

    if [[ -n "${active_journey}" ]]; then
        show_journey_status "${active_journey}"
    fi
}

# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

main() {
    # Parse command line arguments
    local command=""
    local goal=""
    local hint=""

    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            status|pivot|reflect|rollback|list-checkpoints)
                command="$1"
                shift
                ;;
            hint)
                command="hint"
                hint="$2"
                shift 2
                ;;
            -*)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
            *)
                if [[ -z "${command}" ]]; then
                    if [[ -z "${goal}" ]]; then
                        goal="$1"
                    fi
                elif [[ "${command}" == "rollback" ]]; then
                    # The argument is the checkpoint ID
                    set -- "$1" "${@:2}"
                    break
                fi
                shift
                ;;
        esac
    done

    # Load configuration
    load_config

    # Ensure directories exist
    ensure_directories

    # Handle commands
    case "${command}" in
        status)
            handle_status
            exit 0
            ;;
        hint)
            handle_hint "${hint}"
            exit 0
            ;;
        pivot)
            handle_pivot
            exit 0
            ;;
        reflect)
            handle_reflect
            exit 0
            ;;
        rollback)
            handle_rollback "$1"
            exit 0
            ;;
        list-checkpoints)
            handle_list_checkpoints
            exit 0
            ;;
    esac

    # If no goal provided, continue active journey
    if [[ -z "${goal}" ]]; then
        local active_journey
        active_journey=$(find_active_journey)

        if [[ -z "${active_journey}" ]]; then
            log_error "No active journey found"
            log_info "Start a new journey with: $0 \"your goal\""
            log_info "Or check status with: $0 status"
            exit 1
        fi

        main_loop "${active_journey}"
        exit 0
    fi

    # Create new journey
    log_info "Creating new journey for goal: ${goal}"
    local journey_file
    journey_file=$(create_journey_file "${goal}")

    if [[ ! -f "${journey_file}" ]]; then
        log_error "Failed to create journey file"
        exit 1
    fi

    log_success "Journey created! Starting loop..."
    main_loop "${journey_file}"
}

# Run main
main "$@"
