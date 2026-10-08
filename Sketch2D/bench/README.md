# Sketch2D benchmark

`sketch2dBench` replays scripted drawing input through the real Sketch2D application
(window, Vulkan, compositor pacing) and reports where the time goes.

## Running

    tools/sketch2d-bench.sh                       # Release build, all brush and eraser scenarios
    tools/sketch2d-bench.sh --baseline 1a2b3c4    # compare with a stored revision
    tools/sketch2d-bench.sh --filter eraser       # only matching scenarios
    tools/sketch2d-bench.sh --repeats 10          # tighter statistics

Results go to `bench/history/<revision>/` (`report.md` for reading,
`results.tsv` for tools and later comparisons). Without `--baseline` the
newest earlier revision in `bench/history` is used. A revision with
uncommitted changes is stored as `<revision>-dirty`.

## Recording a session in the app

In Sketch2D open **Debug ▸ Record session**. A red `● REC 0:12` badge appears in
the toolbar; draw normally. Stop with **Debug ▸ Stop recording and save** or
by clicking the badge. The session is written to
`~/sketch2d/sessions/<date_time>/` (or `$MODULES_SESSION_DIR/<date_time>/`) and the
path is shown in a notice:

| file | contents |
|---|---|
| `session.txt` | revision, GPU, date, window size and scale, zoom and pan, the full brush (type, preset, colour, all 26 parameters, pressure flags, size limits, stabilizer, marker / linear / eraser modes), view settings, background, frame, every layer's properties and order, active layer |
| `layers/layer-N.tiles` | the exact pixels of every layer when recording started |
| `input.txt` | every raw window event before coalescing, with its time: pen tool, pressure, tilt, rotation, distance, device timestamp, positions, buttons, keys, scroll |
| `recorded-frames.tsv` | what the user actually experienced: per frame wall and CPU time, every profiled section, GPU times and counters |

Replay it:

    tools/sketch2d-bench.sh --session ~/sketch2d/sessions/2026-10-15_20-49-31

This restores the document, view, brush and settings exactly, replays the
input with its original timing through the real window path, and writes
`replay/report.md` next to the recording. Besides the normal statistics the
report has a *Live session versus replay* table: the frame, stage and GPU
timings the user had while drawing beside the replayed ones. Add `--matrix`
to run the brush and eraser scenarios in the same report.

Text input recordings in `bench/recordings/*.input` (the `input.txt` format)
are also replayed as extra inputs for the whole matrix.

## What is measured

Per frame: wall time, thread CPU time, every profiled section (inclusive) and
GPU timestamp durations of the canvas render pass and the window frame.
Per input event: latency from its recorded time to the end of the frame that
consumed it. Per run: process CPU, peak memory, dabs, tiles, uploads.

Frames follow compositor frame callbacks like the real main loop; use
`--unpaced` to render back to back and expose raw throughput instead.

## Comparison

Each scenario is replayed `--repeats` times (default 5). Every key metric
(input latency p95, frame wall median/p95, frame CPU p95, process p95, draw
p95, total dab time, canvas GPU p95) is computed per repeat, so a comparison
has a sample on each side, not a single number. The report shows, for every
scenario and key metric:

- median and coefficient of variation before and after (`4.000 ±2%`)
- the relative change of the medians
- the two-sided Mann-Whitney U p-value (exact for small tie-free samples)
- q, the Benjamini-Hochberg false-discovery-adjusted p over the whole run
- a verdict: **slower / faster** when p < 0.01, the medians differ by more
  than 5 % and 0.05 ms, and the difference exceeds twice the run-to-run
  spread; otherwise **within noise**; **unmeasured** when a large difference
  has only one repeat to test

With 200 metrics compared, about two flags are expected by chance; the
report prints that number next to the verdict counts. Each scenario group is
also summarised as the geometric mean of after/before ratios with counts of
slower and faster cases, so broad shifts are visible even when no single
case is decisive. `--fail-on-regression` returns exit status 3 when any
metric is significantly slower.

Two stored runs can be compared again without rerunning:

    sketch2dBench --baseline bench/history/A/results.tsv --compare bench/history/B/results.tsv --out /tmp/ab

Measured on this machine (Radeon 760M, headless mutter), the same build run
twice gave 0 slower and 1 faster of 200 metrics (expected by chance: 2).
An artificial 10 µs cost per dab was reported as 32 slower metrics with
q = 0.02, largest where dabs are densest (+1579 % dab time at size 4,
+10 % at size 80).
