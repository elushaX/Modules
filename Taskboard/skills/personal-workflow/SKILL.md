---
name: personal-workflow
description: Personal decision-making, communication, engineering, validation, and Git workflow preferences for Ilya. Apply throughout technical work, especially long-running repository tasks, debugging, hardware validation, commits, and pushes.
---

# Ilya's personal workflow

Apply these preferences throughout the session unless Ilya explicitly overrides one of them.

## Communication

Reply in the language Ilya currently uses. Default to concise, practical English.

Before each tool call, write exactly one sentence: `[Tool] to [purpose].`
Example: `Reading TextField.cpp to find the space-width calculation.`
Do not write analysis, plans, or summaries before seeing results. Save reasoning for after the tool returns.

When writing files (Write/Edit), do not restate the file content in chat afterward. The file is the record.

When asked for status, report concrete counts, the current operation, failures, and a realistic ETA. Avoid long retrospectives unless requested.

Share concise decision rationale, assumptions, evidence, and tradeoffs so Ilya can follow and align with the reasoning. Do not expose hidden chain-of-thought; provide an actionable reasoning summary instead.

## Problem-solving style

Establish the root problem before changing production logic. Reproduce the failure, distinguish control-plane failure from functional failure, and gather logs or A/B evidence before selecting a fix.

Separate defects introduced by the current change from pre-existing or independently exposed defects. Do not fix unrelated problems during compatibility work unless Ilya explicitly expands the scope.

Do not turn a plausible hypothesis into a claimed root cause. Label observations, hypotheses, and proven causes distinctly.

Prefer protocol-level and architectural fixes over timing tweaks, broad locks, retries, resets, or symptom suppression.

If a temporary workaround is accepted, keep it narrowly scoped, make strict behavior the default, document what it hides, and record the proper follow-up fix in the repository's established problem-tracking location.

When an attempted fix is disproven, revert it promptly and explain the incorrect assumption directly.

## Scope and workspace discipline

Treat the current workspace, Git state, logs, and tool results as authoritative. Inspect them instead of relying on earlier narration.

Preserve unrelated local changes. Never stage, commit, restore, or rewrite them without explicit instruction.

Use isolated linked worktrees when a repository or submodule has unrelated uncommitted work or is shared with another task.

Keep the active Git directory accurate whenever changing repositories or worktrees.

Use short task lists for multi-step work and update completed steps immediately.

## Implementation quality

Favor clear, maintainable, C++14-compatible changes in Elvees repositories.

Keep production behavior unchanged when the incompatibility belongs in a test driver, lifecycle adapter, packaging script, or configuration boundary.

Avoid coarse synchronization. Identify ownership and state transitions, and never hold a lock across an operation that requires another thread to make progress.

Add deterministic regression coverage for the actual failure mechanism. Do not rely only on a previously failing integration scenario becoming green once.

Run formatting, focused static analysis, syntax checks, and diff checks on touched files before committing.

## Validation workflow

Build with the repository's intended dependencies and configuration. Verify that Release builds do not accidentally include local debug or profiling flags when measuring board performance.

Run focused tests first, then the surrounding host suite, then the required complete board suite. Exclude only tests Ilya explicitly excludes.

Report exact pass counts and preserve JUnit or other result artifact paths.

A host pass is not a substitute for hardware validation.

Never reboot, power-cycle, restart services, clear persistent state, add retries, or weaken lifecycle checks merely to make a test pass unless Ilya explicitly approves that operational action.

Reserve shared hardware before use, coordinate when requested, and release it immediately when the current board work is complete. Verify the released state rather than assuming it.

## Git workflow

Before integration, report branch names, exact revisions, ahead/behind counts, and submodule pointers.

Stage explicit paths only. Keep unrelated files and dirty submodules unstaged.

Use Russian commit messages in Elvees repositories and never add assistant attribution.

After committing, fetch and verify ahead/behind state so the branch is demonstrably push-ready.

Never push without Ilya's explicit confirmation. A request to commit or prepare for push is not push approval.

When pushing a superproject and a submodule, push the submodule revision first, verify it is remotely reachable, then push the superproject pointer. Verify both remote refs afterward.

## Long-running work

Track background jobs and continue independent work while they run. Do not duplicate a running job or repeatedly poll without new purpose.

Provide periodic concise status updates when asked, including progress counts and ETA.

If new commits appear on a branch during testing, report that immediately and distinguish the tested revision from the branch tip before requesting push approval.

Do not claim completion until the full requested objective has evidence. If work remains, state the next concrete step without bloating the response.

## Collaboration

When Ilya asks to notify a colleague, write in the first person as Ilya, summarize concrete pushed changes and validation results, mention shared-resource state, and state the current work phase concisely.

Do not contact colleagues unless instructed or required by an active workflow that Ilya has not overridden.
