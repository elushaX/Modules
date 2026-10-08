"""Where the task manager's code and data live.

The code (this directory) is versioned in the Modules repository. The data - the task database and
every task's session workspace - lives in a separate data directory, by default ~/agents/manager.
Each path can be overridden by its environment variable; nothing else in the server hardcodes a location.
"""
import os

CODE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.environ.get("AGENTS_DATA_DIR", os.path.join(os.path.expanduser("~"), "agents", "manager"))
DB_PATH = os.environ.get("TASKS_DB", os.path.join(DATA_DIR, "tasks.db"))
SESSIONS_ROOT = os.environ.get("AGENTS_SESSIONS_DIR", os.path.join(DATA_DIR, "sessions"))

# Skills are versioned with the code: they live in the Taskboard module next to the server, so the board's
# file browser edits them and git tracks every revision.
SKILLS_DIR = os.environ.get("AGENTS_SKILLS_DIR", os.path.join(os.path.dirname(CODE_DIR), "skills"))

# Secret values live apart from the board (and its history and snapshots) so a board can never leak them.
SECRETS_DB = os.environ.get("AGENTS_SECRETS_DB", os.path.join(DATA_DIR, "secrets.db"))
