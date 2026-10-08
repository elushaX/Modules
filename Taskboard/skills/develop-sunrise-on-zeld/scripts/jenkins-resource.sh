#!/usr/bin/env bash
set -Eeuo pipefail

base=${JENKINS_URL:-https://zeld-lpor-ci.elvees.com}
user=${JENKINS_USER:-nmotin}
token_file=${JENKINS_TOKEN_FILE:-$HOME/jenkins.token}
state_file=${ZELD_LOCK_STATE_FILE:-$PWD/.jenkins-resources}

usage() {
    cat >&2 <<USAGE
Usage:
  $0 status RESOURCE...
  $0 reserve RESOURCE...
  $0 release RESOURCE...
  $0 release-all
State file: $state_file
USAGE
    exit 2
}

[ -r "$token_file" ] || { echo "Cannot read Jenkins token file: $token_file" >&2; exit 1; }
auth="$user:$(cat "$token_file")"
command=${1:-}
[ -n "$command" ] || usage
shift || true

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

fetch_state() {
    curl -fksS -u "$auth" "$base/lockable-resources/api/json?depth=2" >"$tmp"
}

show_one() {
    local resource=$1
    jq -r --arg name "$resource" '
      .resources[] | select(.name==$name) |
      [.name, "reserved="+(.reserved|tostring), "reservedBy="+(.reservedBy // "-"),
       "locked="+(.locked|tostring), "build="+(.build.fullDisplayName // .build.displayName // "-"),
       "queue="+(.queueItemProject // "-")] | @tsv
    ' "$tmp"
}

case "$command" in
    status)
        [ $# -ge 1 ] || usage
        fetch_state
        for resource in "$@"; do show_one "$resource"; done
        ;;
    reserve)
        [ $# -ge 1 ] || usage
        mkdir -p "$(dirname "$state_file")"
        touch "$state_file"
        acquired=()
        for resource in "$@"; do
            if curl -fksS -u "$auth" -X POST                 "$base/lockable-resources/reserve?resource=$resource" >/dev/null; then
                acquired+=("$resource")
                grep -Fxq "$resource" "$state_file" || printf '%s
' "$resource" >>"$state_file"
            else
                echo "Failed to reserve $resource; rolling back this invocation" >&2
                for held in "${acquired[@]}"; do
                    curl -fksS -u "$auth" -X POST                         "$base/lockable-resources/unreserve?resource=$held" >/dev/null || true
                    sed -i "\|^$held$|d" "$state_file"
                done
                exit 1
            fi
        done
        fetch_state
        for resource in "$@"; do show_one "$resource"; done
        ;;
    release)
        [ $# -ge 1 ] || usage
        [ -f "$state_file" ] || { echo "No state file; refusing release" >&2; exit 1; }
        for resource in "$@"; do
            grep -Fxq "$resource" "$state_file" || {
                echo "Resource not acquired by this worktree: $resource" >&2
                exit 1
            }
        done
        for resource in "$@"; do
            curl -fksS -u "$auth" -X POST                 "$base/lockable-resources/unreserve?resource=$resource" >/dev/null
            sed -i "\|^$resource$|d" "$state_file"
        done
        ;;
    release-all)
        [ $# -eq 0 ] || usage
        [ -f "$state_file" ] || exit 0
        mapfile -t resources <"$state_file"
        for resource in "${resources[@]}"; do
            [ -n "$resource" ] || continue
            curl -fksS -u "$auth" -X POST                 "$base/lockable-resources/unreserve?resource=$resource" >/dev/null
        done
        : >"$state_file"
        ;;
    *)
        usage
        ;;
esac
