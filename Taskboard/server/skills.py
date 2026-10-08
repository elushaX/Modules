"""Reusable skills: git-versioned instruction packs agents load on demand.

Each skill is a folder under skills/<name>/ holding a SKILL.md whose YAML-ish front matter carries a
name and a description. The folder lives inside the Taskboard module (next to this server), so skills are
reviewed and edited in the board's file browser and versioned with the rest of the code. Elvees- or
self-hosting-specific packs sit here alongside task-agnostic ones; a task description can name the skills
its agent should load, and the agent reads them through the MCP tools below instead of hardcoding the text.

Secret values never live here. A skill may list the NAMES of the secrets it needs (so a reviewer sees the
required access), and the agent fetches each value at run time through the secrets tools.
"""
import os

from paths import SKILLS_DIR

SKILL_FILE = "SKILL.md"
SKILL_BODY_LIMIT = 60000


def _skill_path(name):
    folder = os.path.join(SKILLS_DIR, name)
    return os.path.join(folder, SKILL_FILE)


def _is_skill_name(name):
    return bool(name) and name not in (".", "..") and "/" not in name and "\\" not in name


def _read(path):
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            return handle.read(SKILL_BODY_LIMIT + 1)
    except OSError:
        return None


def _front_matter(text):
    """Pull name/description out of a leading '---' YAML block without a YAML dependency.

    Only the two keys this board cares about are read; nested or folded values collapse to a single line.
    """
    if not text.startswith("---"):
        return {}
    end = text.find("\n---", 3)
    if end < 0:
        return {}
    fields = {}
    key = None
    for line in text[3:end].splitlines():
        if not line.strip():
            continue
        if line[0] not in " \t" and ":" in line:
            key, _, value = line.partition(":")
            key = key.strip()
            fields[key] = value.strip().strip("'\"")
        elif key is not None:
            fields[key] = (fields[key] + " " + line.strip()).strip()
    description = fields.get("description", "")
    if description in (">-", "|-", ">", "|"):
        description = ""
    return {"name": fields.get("name", ""), "description": description}


def _summary(name):
    path = _skill_path(name)
    text = _read(path)
    if text is None:
        return None
    meta = _front_matter(text)
    return {
        "name": name,
        "title": meta.get("name") or name,
        "description": meta.get("description", ""),
    }


def list_skills():
    """Every skill folder with its declared name and description, sorted by folder name."""
    try:
        names = sorted(entry for entry in os.listdir(SKILLS_DIR) if _is_skill_name(entry))
    except OSError:
        return []
    summaries = []
    for name in names:
        summary = _summary(name)
        if summary is not None:
            summaries.append(summary)
    return summaries


def get_skill(name):
    """The full SKILL.md body for one skill, with its parsed name and description."""
    if not _is_skill_name(name):
        raise ValueError(f"invalid skill name {name!r}")
    path = _skill_path(name)
    text = _read(path)
    if text is None:
        raise ValueError(f"skill {name!r} not found")
    truncated = len(text) > SKILL_BODY_LIMIT
    if truncated:
        text = text[:SKILL_BODY_LIMIT] + "\n[...truncated]"
    meta = _front_matter(text)
    return {
        "name": name,
        "title": meta.get("name") or name,
        "description": meta.get("description", ""),
        "body": text,
        "truncated": truncated,
    }
