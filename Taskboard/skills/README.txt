# Skills

Reusable instruction packs that agents load on demand. Each skill is a folder here holding a
`SKILL.md` whose front matter declares a `name` and a `description`. Because this folder lives inside
the Taskboard module, every skill is reviewed and edited in the board's **Skills** tab and versioned
with the rest of the code.

An agent reaches these skills through the task-manager MCP server:

- `list_skills()` — every skill with its name and description.
- `get_skill(name)` — the full `SKILL.md` body for one skill.

A task description can name the skills its agent should load (for example, Elvees work points at the
Elvees skills, self-hosting work at the infrastructure skills). Task-agnostic skills live here too, so
they are always available for review and editing.

The harness discovers skills through one symlink to this folder, not a link per skill:

    ~/.agents/skills -> <checkout>/Taskboard/skills

Adding or removing a skill here needs no link to be created, and there is exactly one place the
catalogue comes from. This file is `.txt` on purpose: the harness treats a root-level `*.md` as a flat
skill and would warn that it has no front matter.

## Skills never hold secret values

A skill may list the **names** of the secrets it needs so a reviewer can see what access it requires,
but it must never contain the secret value itself. The agent fetches each value at run time:

- `list_secrets()` — the stored secret keys and descriptions (never values).
- `get_secret(key)` — the value for one key.

Secret values are kept in a separate store outside the board, so editing, reviewing, or exporting a
board can never leak them.
