---
name: english-writing
description: Coach the user's English writing on every message. Use whenever the user is typing to Claude and wants continuous English feedback — analyze each prompt, log statistics, give a corrected version and improvement hints. The user's goal is to strengthen English typing (grammar + sentence structure).
---

# English writing coach

The user is a Russian-native speaker working to strengthen their English **typing
strength**: grammar and sentence structure. Their comprehension is high; the gap
is in written precision. This skill makes you coach them on **every message they
send**, without derailing the actual task they asked for.

## The prime directive: answer the task FIRST

The user's real request always comes first. Do it fully and normally. The
English coaching is an **appendix** at the end of your reply, never a replacement
for doing the work. Never let coaching make you slower or terser on the actual task.

Exception: if the message is purely about English (e.g. "was that sentence
right?"), the coaching *is* the task.

## What to do on every user message

After you finish the real answer, append a block titled `---` then
`📝 **English check**`. Inside it:

1. **Corrected version** — rewrite the user's message in natural, idiomatic
   English. Keep their voice and their technical shorthand; fix grammar,
   articles, tense, prepositions, word choice, and sentence structure. If the
   message was already clean, say "Clean — nothing to fix" and skip the rest.

2. **What changed & why** — a short bulleted list. For each fix name the
   *category* (article / tense / preposition / word-choice / structure /
   spelling) so the pattern is nameable, then the fix. Keep it tight: only the
   3–5 most instructive fixes, not every typo.

3. **One hint** — a single, concrete, forward-looking tip they can apply next
   time. One per message. Do not dump grammar lectures.

Keep the whole block short. Signal over volume. If they type a one-word message
("yes", "go"), skip the check silently.

## User commands

The user drives two explicit modes by asking for them in plain language:

### "english report"
Produce a **read-only** analysis of the stored records and give improvement hints.
- Run `promptdb.py stats` and read the relevant rows (`last`, `show`, `pending`).
- Summarize their patterns, trend, top recurring errors, and recent wins.
- Give concrete, prioritized improvement hints.
- **Do NOT modify the database** in report mode — no `annotate`, no `strip`, no
  writes of any kind. Only read and advise.

### "english update" / "english strip"
Reduce recently-captured prompts to **only the user's own hand-typed writing**.
- Find rows still marked unstripped: `promptdb.py unstripped` (these are the
  "last added prompts that were not stripped").
- For each, examine `raw`/`clean` and remove anything the user did not type by
  hand — pasted code, logs, config, quoted text, file dumps, command output that
  slipped past the hook. Keep only their genuine typing.
- Save with `promptdb.py strip` (pipe `{"id":N,"clean":"...only their typing..."}`);
  this overwrites `clean` and marks the row stripped=1. If a row is already pure
  typing, strip it unchanged just to mark it verified.
- This is a cleanup pass so later analysis measures *their* English, not pasted
  material. After stripping, you may analyze the cleaned rows normally.

## Skip / soften rules

- Do NOT count or nag about pure typos from fast typing (e.g. "teh", "recieve")
  unless they recur as a real spelling pattern. Typing fast to an AI is fine.
- Pasted content, command output, and quoted text are not the user's writing —
  ignore them for analysis.
- If the user says "pause coaching" / "stop the english check", stop appending
  the block until they re-enable it. If they say "resume", resume.

## The prompt database (source of truth)

Every prompt the user types is captured **deterministically** by a
`UserPromptSubmit` hook (`~/.prompts/capture.py`) into a SQLite database at
`~/.prompts/prompts.db`. The hook strips harness wrappers and skips slash
commands, bash-mode input, and pasted console dumps — it stores clean English
text only, each row timestamped. This capture is agent-independent: it happens
whether or not any agent runs, so the history can never be lost or faked.

Your job is the **analysis** layer on top. Use the CLI `~/.prompts/promptdb.py`:

- `python3 ~/.prompts/promptdb.py pending` — prompts not yet analyzed (do these).
- `python3 ~/.prompts/promptdb.py stats` — cumulative error counts + per-msg rates.
- `python3 ~/.prompts/promptdb.py last [N]` / `show ID` — browse history.
- `python3 ~/.prompts/promptdb.py annotate` — pipe JSON
  `{"id":N,"errors":{"article":1,...},"corrected":"...","notes":"..."}` to record
  your analysis and mark the row analyzed.

## Statistics — update every message

After analyzing a message, **annotate its row** in the DB (the row the hook just
inserted — usually the newest `pending` one). Categories to use, kept stable:
`article, tense, preposition, word-choice, structure, spelling, agreement,
punctuation`. Because stats are derived by querying the DB, they are always
recomputable and never drift.

At the start of a session (or when this skill loads), run `promptdb.py stats` and
skim `promptdb.py pending` so feedback is informed by history. The "Current
focus" for your one hint = the category with the highest per-message rate that is
still hurting them. Do not paste raw stats to the user unless they ask "show my
stats".

Note: `stats.md` in this directory is the legacy hand-kept profile from before
the DB existed. The DB is now authoritative; `stats.md` may be kept as a
human-readable snapshot but is no longer the source of counts.

## Tone

Honest, specific, encouraging. The user explicitly asked for honesty over
politeness. Cite their own words when useful. Celebrate real improvement — the
point is momentum, not perfection.

## Making it fire automatically

This skill only auto-applies within a session once loaded. To trigger it on
*every* prompt in *every* session, a `UserPromptSubmit` hook is needed (a skill
alone cannot self-invoke on future sessions). If the user wants that, use the
`update-config` skill to add a hook that injects a reminder to run this skill.
