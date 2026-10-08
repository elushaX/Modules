#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<USAGE
Usage: $(basename "$0") <program> [arguments...]

Runs <program> against a private headless Wayland compositor (mutter) so its
window never appears on the desktop, never takes focus and never receives the
desktop's pointer or keyboard. Exits with the program's exit status.
USAGE
}

if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]; then
	usage
	exit 2
fi

: "${XDG_RUNTIME_DIR:?XDG_RUNTIME_DIR must be set}"
display_name="modules-headless-$$"
ready_line="Using Wayland display name '$display_name'"

coproc COMPOSITOR {
	exec mutter --headless --no-x11 --wayland-display="$display_name" --virtual-monitor 1600x1000 2>&1
}
compositor_pid=$COMPOSITOR_PID

stop_compositor() {
	kill -TERM "$compositor_pid" 2>/dev/null || true
	wait "$compositor_pid" 2>/dev/null || true
}
trap stop_compositor EXIT

while IFS= read -r line <&"${COMPOSITOR[0]}"; do
	[[ "$line" == *"$ready_line"* ]] && break
done
if [[ "${line:-}" != *"$ready_line"* ]]; then
	echo "headless compositor exited before announcing $display_name" >&2
	exit 1
fi

cat <&"${COMPOSITOR[0]}" >/dev/null &

WAYLAND_DISPLAY="$display_name" "$@"
