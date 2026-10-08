#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<USAGE
Usage: $(basename "$0") [--baseline REVISION|FILE] [--session DIR] [sketch2dBench options...]

Builds sketch2d in Release (build-bench/), replays the scripted brush and eraser
scenarios headlessly and writes bench/history/<revision>/{report.md,results.tsv}.

By default the run is compared with the newest earlier revision in
bench/history. --baseline picks a revision directory or a results.tsv file.
Every input recording in bench/recordings/*.input is added as a scenario input.

  --session DIR   replay a session recorded in sketch2d with Debug > Record session
                  and write DIR/replay/report.md next to the recording
  --tile-size N   build with canvas tiles of N x N pixels in build-bench-tileN/
USAGE
}

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build-bench"
history="$root/bench/history"
recordings="$root/bench/recordings"

build_release() {
	cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release ${tile_size:+-DLEO_TILE_SIZE=$tile_size} >/dev/null
	cmake --build "$build" -j"$(nproc)" --target sketch2dBench sketch2dApp >"$build/build.log" 2>&1 || {
		grep -n "error:" "$build/build.log" | head -20 >&2
		echo "release build failed, see $build/build.log" >&2
		exit 1
	}
}

baseline_arg=""
session=""
tile_size=""
passthrough=()
while [[ $# -gt 0 ]]; do
	case "$1" in
		-h | --help) usage; exit 0 ;;
		--baseline) baseline_arg="$2"; shift 2 ;;
		--tile-size) tile_size="$2"; build="$root/build-bench-tile$2"; shift 2 ;;
		--session) session="$(cd "$2" && pwd)"; shift 2 ;;
		*) passthrough+=("$1"); shift ;;
	esac
done

mkdir -p "$build" "$history" "$recordings"
build_release

if [[ -n "$session" ]]; then
	cd "$build/Sketch2D"
	"$root/tools/run-headless-wayland.sh" ./sketch2dBench --session "$session" --build Release "${passthrough[@]}"
	echo "report: $session/replay/report.md"
	exit 0
fi

revision="$(git -C "$root" rev-parse --short HEAD)"
if [[ -n "$(git -C "$root" status --porcelain --untracked-files=no -- . ':!Externals')" ]]; then
	revision="$revision-dirty"
fi
out="$history/$revision"

baseline=""
if [[ -n "$baseline_arg" ]]; then
	if [[ -f "$baseline_arg" ]]; then baseline="$baseline_arg"
	else baseline="$history/$baseline_arg/results.tsv"; fi
else
	while read -r candidate; do
		[[ "$(basename "$(dirname "$candidate")")" == "$revision" ]] && continue
		baseline="$candidate"
		break
	done < <(ls -t "$history"/*/results.tsv 2>/dev/null || true)
fi

arguments=(--out "$out" --revision "$revision" --build Release)
[[ -n "$baseline" && -f "$baseline" ]] && arguments+=(--baseline "$baseline")
for recording in "$recordings"/*.input; do
	[[ -f "$recording" ]] && arguments+=(--recording "$recording")
done

cd "$build/Sketch2D"
"$root/tools/run-headless-wayland.sh" ./sketch2dBench "${arguments[@]}" "${passthrough[@]}"
echo "report: $out/report.md"
