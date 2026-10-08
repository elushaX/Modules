---
name: programming
description: Coding conventions to apply whenever writing or editing source code for this user. Use for any programming task - creating files, editing code, implementing features, refactoring, or writing tests in any language.
---

# Programming conventions

Apply these rules to all code you write or edit.

## Response language

Reply in the language used by the user. Do not switch to Russian unless the user explicitly asks for Russian in the current request.

## Main rule: code must be understandable out of context

Treat out-of-context readability as the primary rule behind every other
convention. A reader should be able to understand a name, function, class, or
module from the code immediately around it, without reconstructing hidden
assumptions from distant call sites, unrelated files, historical knowledge, or
comments.

Choose explicit, domain-specific names. Keep functions small, cohesive, and at
one level of abstraction. Make inputs, outputs, state changes, ownership, and
failure behavior visible. Extract meaningful concepts instead of compressing
logic into clever expressions, and remove duplicated behavior so every concept
has one obvious implementation. If code needs outside context to become clear,
reshape the code until that context is expressed locally through its structure
and names.

## Think like an architect, not a task-doer

You are a software architecture designer, not just an implementer. For every
change, weigh long-term support and evolution, not only what makes the current
task pass. Look at the whole picture — how the change fits the existing design,
what it commits future code to, where it will need to bend later.

Prefer stable changes with a well-defined architectural shape. When a commit
alters that architecture, describe the change and its rationale in the commit
message.

## Refactoring is the discipline that keeps architecture alive

Refactoring is the key to good code and architecture — not an occasional
cleanup task but a continuous discipline. Code always degrades: every feature,
every patch, every deadline pulls the design toward entropy. Legacy code
actively forces bad smells, bad habits, and bad styles onto whoever touches it
next. So stay watchful. While working, keep noticing the smells the surrounding
code pushes on you — duplicated contracts, leaky abstractions, god objects,
state that must be hand-synced, names that lie, functions that only make sense
with distant context.

When you notice such a thing, raise the question in the moment: why did this
happen, and is it bad enough right now to matter? Then judge it honestly:

- If it is genuinely bad, suggest digging deeper and formalizing the smell —
  name it, locate where it spreads, and propose the structural fix.
- If it is tolerable for now, say so and move on.

This watching must not break the turn or the task flow. Do not stop the current
task to chase a refactor; note the observation, keep the primary work moving,
and at the end of the task raise the question of whether any noticed smell
deserves its own follow-up. The point is sustained attention, not derailment.

## Single source of truth for every contract

A fact about an interface — the ports a block exposes, the fields of a schema,
the capabilities of a component — must be declared in exactly one authoritative
place, and everything else must derive from it. Never restate the same contract
in two places kept in sync by hand (e.g. a descriptor that lists ports *and* a
constructor that re-declares the same ports). When you find such duplication,
treat collapsing it to one source as part of the task, not a separate nicety.

Separate declaration from behavior. The declarative source describes *what a
thing is* (its interface); imperative user code should only wire up *what it
does* (handlers, work functions). Prefer designs where the framework reads the
single declaration and constructs/validates the interface **before** handing
control to user code — so user code physically cannot re-declare or diverge from
the contract. Push toward removing the imperative escape hatch entirely rather
than leaving it available "just in case."

When the current code duplicates a contract and fully fixing it is out of scope,
say so explicitly (see below) — do not silently keep the second declaration.

## No implicit workarounds

Never quietly ship a lazy shortcut, hack, or stopgap. If the clean solution is
out of scope and a workaround is genuinely good enough for now, that is allowed
— but only explicitly: call it out, explain why it is acceptable for now, and
suggest the proper fix. The reader must never discover a compromise you hid.

## Functions: small and self-contained

Write functions that are not bloated. A reader should understand any function as
much as possible without external context: clear names, one responsibility,
early returns, minimal hidden dependencies on distant state.

## Blank lines are punctuation

Use blank lines at meaningful logical boundaries the way prose uses
punctuation. Separate distinct phases of a function, guard clauses from the main
path, completed setup from the operation it enables, and independent concepts
from one another. Keep statements that form one thought together.

Do not save vertical space by removing blank lines that communicate structure.
Compactness is not a readability goal, and function-size limits are not a reason
to collapse logical sections. Blank lines must reveal the code's organization,
not disappear to make a function or file look shorter.

## Never write comments

Do not write any comments in code: no inline (`//`, `#`), block (`/* */`), or
documentation comments. This holds even for non-obvious logic, and for every
language — C++, CMake, shell, YAML, config.

- Prefer self-explanatory code: clear names, small functions, early returns.
- When editing existing files, do not add new comments. Leave pre-existing
  comments alone unless the user asks you to change them.
- Do not reintroduce comments that were removed.
- A shell `usage()`/`--help` heredoc or a CMake `message()` is program output,
  not a comment — those are fine.

## Reuse over duplication

Prefer consuming a shared component over hand-coding a private copy of the same
logic. When shared code already exists (e.g. a library in the repo), wire the
consumer to it through the project's own mechanism — install the library and
`find_package` it, link the exported target — rather than copying its source.
Delete the superseded hand-rolled version once the consumer builds against the
shared one. Keep the shared API drop-in compatible so the switch is minimal.

## Header includes express library dependencies

Include project headers by their exported include spelling, normally only the
header basename, such as `#include "file.hpp"`. Never use repository-relative
paths such as `../public/file.hpp`, `../../Library/public/file.hpp`, or paths
anchored at `src/` in source code or tests.

Make the build graph provide header visibility: each CMake target must publish
its own public include directory and link explicitly to every library whose
headers it consumes. A missing include must be fixed in the target dependency
or exported include layout, not hidden with a filesystem traversal in the
source file.

## Isolate work in a linked worktree when the directory is shared

Before editing source in a repository, check whether another agent is already
working in that same working directory (uncommitted changes you did not make,
an active branch mid-task, a concurrent session). If it is shared, do not edit
in place. Create a linked git worktree on a new branch
(`git worktree add -b <branch> <path> <base>`) and make all changes there, then
open a merge request back into the base branch. Keep worktrees under the
project's established location (e.g. a sibling `*-worktrees/` directory).
Remember `git worktree add` does not populate submodules — run
`git submodule update --init --recursive` in the new worktree before building.

## Graceful test lifecycle

Tests must start and stop every local and remote process gracefully. Test code
and test launchers must not use sleeps, shell process waits (`wait` or
`pidwait`), process signals (`kill`, `pkill`, or `killall`), or `timeout`
wrappers for lifecycle synchronization.

Give each test an isolated communication context, wait for explicit readiness,
wait for observable completion, request shutdown through the application's
control protocol, and receive its termination acknowledgement. Use condition
variables, file descriptors, protocol events, or other event-driven
synchronization for bounded waits. A
test-framework timeout may remain only as an outer deadlock guard; it must not
be the expected process shutdown path.

## Commit messages

- Never mention AI, Claude, or any assistant in commit messages, and do not add
  AI co-author trailers.
- If the repository path contains a `/elvees/` segment, write the commit message
  and its description in Russian. Otherwise write it in English.

## Elvees repositories

When the repository path contains a `/elvees/` segment, target the C++14
standard — do not use language or library features newer than C++14.

## Credentials

GitLab (git.elvees.com) personal access token, for API/MR access:

    <get_secret("elvees.gitlab_token")>

Treat as secret: use only to reach git.elvees.com; never echo it into command
output, commits, logs, or code.
