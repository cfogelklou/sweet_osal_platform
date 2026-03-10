# GitHub PR Monitoring Workflow

**Repository**: `cfogelklou/sweet_osal_platform`
**Main Branch**: `master` (and `optimizations_by_ai` for development)
**Purpose**: Active monitoring of GitHub PRs for AI agents

---

## Scope

**Default Behavior**: Monitor GitHub Actions failures on `master` and development branches.

**When Triggered**: When the user explicitly requests **"Monitor PR #XXX"**, the agent monitors that specific PR for:
- GitHub Actions status
- Review comments (human or Copilot)

**Important**: This workflow is for **manual PR monitoring** when the user requests it.

---

## Trigger

When the user requests: **"Monitor PR #XXX"**

The AI agent should **actively monitor** the pull request for GitHub Actions status and review comments. This is an ongoing polling cycle that continues until the PR is merged, closed, or reaches a terminal state.

---

## Phase 0: Pre-Check - SSH Key Verification

**BEFORE starting any PR monitoring work, IMMEDIATELY verify SSH keys work:**

```bash
git fetch origin
```

**If `git fetch` prompts for a password/key passphrase, STOP and inform the user:**

> Your SSH keys require a password/passphrase. Git commits and pushes will fail. Please either:
> 1. Add your key to ssh-agent with `ssh-add`
> 2. Use HTTPS with a Personal Access Token
> 3. Set up passwordless SSH keys
>
> I cannot proceed with PR monitoring until git operations work without password prompts.

---

## Phase 1: Initial Assessment

### 1.1 Check PR Status and Review Comments

```bash
# Get PR status
gh pr view {PR_NUMBER} --repo cfogelklou/sweet_osal_platform --json state,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,title,url

# Get ALL review comments (including thread replies)
gh api /repos/cfogelklou/sweet_osal_platform/pulls/{PR_NUMBER}/comments --jq '.[] | {path: .path, line: .line, body: .body, user: .user.login, created: .created_at, in_reply_to: .in_reply_to_id, id: .id}'

# Get PR reviews (Copilot often creates reviews with inline comments)
gh pr view {PR_NUMBER} --repo cfogelklou/sweet_osal_platform --json reviews --jq '.reviews[] | {author: .author.login, state: .state, body: .body}'
```

**IMPORTANT**: Always check BOTH:
1. Review comments (inline comments on specific lines)
2. PR reviews (top-level reviews that contain inline comments, like Copilot's)

### 1.2 Record Initial State

- PR state (open/closed/merged)
- Mergeable status
- Review decision (approved/changes_requested/commented)
- GitHub Actions status (passing/failing/pending)
- List of unresolved review comments
- List of PR reviews (especially from `copilot-pull-request-reviewer`)

---

## Phase 2: Polling Loop

**Poll Interval**: Every 4 minutes

### Each Polling Cycle:

1. **Check GitHub Actions Status**
   - CMake build (Linux, macOS, Windows)
   - Unit tests (all platforms)
   - Emscripten build (WebAssembly)

2. **Check for New Review Comments**
   - Copilot comments
   - Human reviewer comments
   - New replies to existing threads

3. **Continue Until**:
   - All GitHub Actions checks pass
   - All review comments have replies

---

## Phase 3: GitHub Actions Failures

When a GitHub Actions check fails:

### 3.1 Determine Root Cause

| Failure Type | Action |
|--------------|--------|
| **Transient infrastructure** (cache service down) | Re-run the failed job |
| **Code error** (compilation, test failure) | Fix locally, commit/push |
| **Flaky test** | Re-run once, then investigate if persists |

### 3.2 For Transient Failures (Cache Service, etc.)

```bash
# Re-run the failed checks
gh run rerun {RUN_ID} --repo cfogelklou/sweet_osal_platform
```

### 3.3 For Code Errors - Fix Locally

```bash
# 1. Sync local branch to remote (hard reset to avoid conflicts)
git fetch origin
git checkout -B {BRANCH_NAME} origin/{BRANCH_NAME} --force
git submodule update --init --recursive

# 2. Fix the issue locally

# 3. Run sanity checks
cd build && cmake .. && make -j8
ctest -j8

# 4. Commit and push
git add .
git commit -m "{descriptive commit message explaining the fix}"
git push origin
```

### 3.4 Monitor Re-run

After pushing, the GitHub Actions will automatically re-run. Continue polling to verify the fix.

---

## Phase 4: Review Comments - Critical Rules

**ALL comments MUST be replied to on GitHub** — This is non-negotiable. The user cannot mark comments as resolved or request a new review until replies exist.

### 4.1 Process Each Review Comment

#### a. Read Full Thread

If `in_reply_to_id != null`, read ALL replies to check if already addressed:

```bash
# Fetch all comments and filter by in_reply_to
gh api /repos/cfogelklou/sweet_osal_platform/pulls/{PR_NUMBER}/comments --jq '.[] | select(.in_reply_to == {COMMENT_ID})'
```

#### b. Verify in Current Code

Use the `Read` tool to check if the issue actually exists in the current codebase at the specified path and line.

#### c. Critically Evaluate

| Comment Status | Action |
|----------------|--------|
| **Valid and needs fixing** | Fix it, test locally, commit/push, reply explaining the fix |
| **Already fixed** | Reply explaining it was already addressed (provide commit hash if available) |
| **Incorrect/false positive** | Reply politely explaining why it's not an issue |
| **Requires clarification** | Reply asking for more details |

#### d. ALWAYS Reply

**Quick reference** - Reply to comment ID `{COMMENT_ID}` on PR `{PR_NUMBER}`:
```bash
gh api -X POST "/repos/cfogelklou/sweet_osal_platform/pulls/{PR_NUMBER}/comments/{COMMENT_ID}/replies" \
  -f body="Fixed — [explain what was changed and why]"
```

### 4.2 Reply Templates

| Status | Response Template |
|--------|-------------------|
| Fixed | "Fixed - [explanation of fix]" |
| Already fixed | "Already fixed in [commit hash]" |
| Intentional | "Intentional - [reasoning]" |
| False positive | "Not an issue - [explanation]" |
| Deferred | "Deferred - [will address in issue/ticket]" |

---

## Phase 5: After Local Changes

**CRITICAL**: You MUST `git commit` and `git push` — This is required for the user to see changes and for CI to re-run.

> **Note**: During PR monitoring, you are explicitly authorized to commit and push PR fixes as needed to address CI failures and review feedback.

---

## Phase 6: Completion Criteria

### Stop polling and notify the user when ANY of the following occur:

| Condition | Action |
|-----------|--------|
| **PR Merged** | Stop monitoring THIS PR, notify user of successful merge |
| **PR Closed** | Stop monitoring THIS PR, notify user PR was closed without merge |
| **Checks Pass + Comments Replied** | Notify user ready for merge/review, stop polling THIS PR |
| **Draft PR** | Notify user PR is in draft state, stop polling THIS PR |

**Terminal States**:

```bash
# Check if PR is in a terminal state
gh pr view {PR_NUMBER} --repo cfogelklou/sweet_osal_platform --json state,merged,mergedAt,closedAt,isDraft
```

**Stop monitoring THIS PR if:**
- `state: "MERGED"` → Success, notify user
- `state: "CLOSED"` and not merged → PR closed, notify user
- `isDraft: true` → Draft state, notify user

**Ready for User Action**:

When GitHub Actions pass AND all comments have replies:
- Notify the user: "PR #{PR_NUMBER} is ready for merge. All checks pass and all comments have been addressed."
- Stop polling THIS PR (awaiting user decision to merge or request changes)

**Do NOT automatically merge** — merging requires explicit user approval.

---

## Useful Commands

```bash
# Check GitHub Actions status
gh pr checks {PR_NUMBER} --repo cfogelklou/sweet_osal_platform

# Get failed build logs
gh run view {RUN_ID} --repo cfogelklou/sweet_osal_platform --log-failed

# Open PR in browser
gh pr view {PR_NUMBER} --repo cfogelklou/sweet_osal_platform --web

# List all comments on a PR
gh pr view {PR_NUMBER} --repo cfogelklou/sweet_osal_platform --json comments --jq '.comments[] | {body: .body, author: .author.login, path: .path, line: .line}'

# Get PR diff
gh pr diff {PR_NUMBER} --repo cfogelklou/sweet_osal_platform

# Re-run failed job
gh run rerun {RUN_ID} --repo cfogelklou/sweet_osal_platform
```

---

## CI/CD Configuration

### Workflow File

**Location**: `.github/workflows/ci_pr.yml`

**Platforms Tested**:
- `ubuntu-latest` - Linux builds (includes Emscripten/WebAssembly)
- `macos-latest` - macOS builds
- `windows-latest` - Windows builds

### What Gets Tested

- All unit tests (`sweet_osal_test`, `osal_test`, `utils_test`, etc.)
- Cross-platform compilation (CMake)
- Code formatting (clang-format)

---

## Important Notes

1. **Don't blindly accept all comments** — Verify if issues exist in current code and align with architecture. If a comment is incorrect or outdated, politely explain why with technical justification.

2. **Always test locally before pushing** — Use the build and test commands to catch issues before pushing.

3. **Keep polling** — GitHub Actions may flake. If a check fails without local changes, simply re-run it via the GitHub CLI or UI.

4. **Document decisions** — If you disagree with a review comment, explain your reasoning clearly so the human reviewer can understand your perspective.

5. **Transient infrastructure failures** — GitHub cache service sometimes fails. If you see "Our services aren't available right now", re-run the job.

---

**Last Updated**: 2026-03-10
