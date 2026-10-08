#!/usr/bin/env bash
set -Eeuo pipefail

jump=${ZELD_JUMP_HOST:-nmotin@supermarket-wm}
zeld=${ZELD_HOST:-zeld-lpor-nmotin}

usage() {
    cat >&2 <<USAGE
Usage:
  $0 --shell
  $0 --stdin
  $0 --script FILE [ARG...]
  $0 'single shell command'
USAGE
    exit 2
}

run_stdin() {
    ssh -F /dev/null -o BatchMode=yes "$jump"         "ssh -o BatchMode=yes $zeld 'bash -s'"
}

run_script() {
    local file=$1
    shift
    local arg
    local remote_args=
    [ -r "$file" ] || { echo "Cannot read script: $file" >&2; exit 1; }
    for arg in "$@"; do
        [[ "$arg" =~ ^[A-Za-z0-9_./:@+-]+$ ]] || {
            echo "Unsafe script argument: $arg" >&2
            exit 2
        }
        remote_args="$remote_args $arg"
    done
    ssh -F /dev/null -o BatchMode=yes "$jump"         "ssh -o BatchMode=yes $zeld 'bash -s --$remote_args'" <"$file"
}

case ${1:-} in
    --shell)
        [ $# -eq 1 ] || usage
        exec ssh -F /dev/null -tt "$jump" "ssh -tt $zeld"
        ;;
    --stdin)
        [ $# -eq 1 ] || usage
        run_stdin
        ;;
    --script)
        [ $# -ge 2 ] || usage
        shift
        run_script "$@"
        ;;
    '')
        usage
        ;;
    *)
        [ $# -eq 1 ] || usage
        printf '%s
' "$1" | run_stdin
        ;;
esac
