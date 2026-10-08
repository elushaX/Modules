#!/usr/bin/env bash
# Start the whole system in tmux session "agents":
#   window board      : board TUI (top) + supervisor Claude session (bottom)
#   window dispatcher : dispatcher loop
# Task sessions run headless in their own tmux sessions (task-<id>); data lives in $AGENTS_DATA_DIR
# (default ~/agents/manager), see paths.py.
set -euo pipefail
cd "$(dirname "$0")"
S="${AGENTS_TMUX:-agents}"
SUP_MODEL="${SUPERVISOR_MODEL:-opus}"
WORKERS="${WORKERS:-3}"

uv sync -q
if tmux has-session -t "$S" 2>/dev/null; then exec tmux attach -t "$S"; fi

tmux new-session -d -s "$S" -n board -c "$PWD" "uv run python board.py"
MCP_CONFIG="$(uv run python -c 'import json, sessions; print(json.dumps(sessions.mcp_config()))')"
tmux split-window -v -t "$S:board" -c "$PWD" \
  "claude --model $SUP_MODEL --mcp-config $(printf %q "$MCP_CONFIG") --allowedTools mcp__task-manager \
   -n supervisor --append-system-prompt \"\$(cat supervisor.md)\"; read"
tmux new-window -d -t "$S" -n dispatcher -c "$PWD" "uv run python dispatcher.py --workers $WORKERS; read"
tmux select-pane -t "$S:board.0"
exec tmux attach -t "$S"
