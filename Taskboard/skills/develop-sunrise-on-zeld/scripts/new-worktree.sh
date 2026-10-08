#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
    echo "Usage: $0 SOURCE_REPOSITORY TASK_SLUG [BASE_REF]" >&2
    exit 2
}

[ $# -ge 2 ] && [ $# -le 3 ] || usage
source_repo=$(readlink -f "$1")
slug=$2
base_ref=${3:-HEAD}

git -C "$source_repo" rev-parse --git-dir >/dev/null
git -C "$source_repo" rev-parse --verify "$base_ref^{commit}" >/dev/null

safe_slug=$(printf '%s' "$slug" | tr '[:upper:]_' '[:lower:]-' | tr -cd 'a-z0-9-')
[ -n "$safe_slug" ] || { echo "TASK_SLUG has no safe characters" >&2; exit 1; }
[ "$safe_slug" = "$slug" ] || echo "Normalized task slug: $safe_slug" >&2

owner=${USER:-agent}
stamp=$(date +%Y%m%d-%H%M%S-%N)-$$
repo_name=$(basename "$source_repo")
work_root=${SUNRISE_WORK_ROOT:-$(dirname "$source_repo")}
destination="$work_root/$repo_name-$safe_slug-$owner-$stamp"
branch_prefix=${SUNRISE_BRANCH_PREFIX:-work}
branch="$branch_prefix/$owner/$safe_slug-$stamp"

[ ! -e "$destination" ] || { echo "Destination already exists: $destination" >&2; exit 1; }
git -C "$source_repo" worktree add -b "$branch" "$destination" "$base_ref"

if [ "${INIT_SUBMODULES:-1}" = 1 ]; then
    git -C "$destination" submodule update --init --recursive
fi

printf 'WORKTREE=%q
' "$destination"
printf 'BRANCH=%q
' "$branch"
