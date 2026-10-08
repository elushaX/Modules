# Brush: Sketch2D vs Leonardo

Source of truth for Leonardo is the shipped `leonardo.0.17.70.win64.exe`
(sha256 8df6fc28…0eb), decompiled with Ghidra 12.1. Function names are Ghidra addresses; source
file names come from the assert strings embedded next to them. Sketch2D references are to
`optv2` at the time of writing.

Findings come from `tests/BrushWorkflowTests.cpp`, a set of integration tests that drive the real
canvas widget like a person would: strokes at every zoom from 2× to 1/64, sizes from 1.5 px to
2000 px, opacity/flow settings, self-overlapping scribbles, pressure, pinches between strokes,
undo/redo across zooms, erasing and a long 21-stroke mixed session.

## 1. How a stroke becomes dabs

### Leonardo (br_stroke.c, br_stage.c)

- Every input sample is appended to a **stroke buffer** (`FUN_1400cd570`): x, y, pressure, tilt
  x/y, tilt magnitude, rotation, time. Up to 24 000 samples per stroke.
- The stroke is a **curve** through those samples. Positions, pressure and the other channels are
  evaluated with a stage function stored on the stroke (`+0x9b8`): either linear or a
  Catmull-Rom spline (`FUN_1400e1520`, `FUN_1400e1460`) over the sample arrays.
- Dabs are placed by walking the curve parameter `t` (`FUN_1400e1ac0`): for each next dab it
  bisects `t` until the curve distance from the previous dab equals the spacing, which is
  `size × spacing` evaluated at that `t` (min 0.001). The walk keeps its position between
  calls (`+0x9c0`), so new samples only extend the walk. Dabs therefore sit at exact arc-length
  steps **along the smoothed curve**, independent of how many input samples arrived or how fast.
- Each dab (`FUN_1400ce960` → `FUN_1400ce740` → `FUN_1400ce120` → `FUN_1400cef50`) evaluates
  size, opacity, flow, colour jitter and so on from the **pressure at `t`** through each
  parameter's curve (`FUN_1400e0ff0`, a sampled response curve).
- `FUN_1400cef50` stores the dab in fixed point (`×16384`, i.e. 1.0 = 0x4000) and quantises the
  dab position to 1/16 px (coarser for big dabs: 1/8 above 16 px, 1/4 above 64 px, 1/2 above
  256 px) so that rasterised dab masks can be cached and reused.

### Sketch2D (CanvasWidget::paintSegment)

- Input points go through the stabilizer; each output point is joined to the previous one with
  a **straight segment**; dabs are dropped along each segment every `size(p) × spacing`, with the
  remainder carried to the next segment.
- Pressure is linearly interpolated per segment.

### Differences

| | Leonardo | Sketch2D | Effect |
|---|---|---|---|
| Path | spline through all samples | polyline between samples | sparse input (fast hand, coarse event rate, zoomed out) gives visible corners and spacing that follows the input, not the curve |
| Placement | arc length along the curve, solved per dab | linear steps on each segment | spacing differs with input rate (test: 400/400 samples differ between 6-point and 120-point input of the same line) |
| Dab parameters | evaluated at the curve parameter of the dab | evaluated from linearly interpolated pressure | pressure tapers are polygonal |
| Position | quantised to 1/16 … 1/2 px, mask cached | float, mask computed per pixel per dab | Leonardo reuses dab masks; we recompute coverage for every pixel of every dab |

## 2. Opacity vs flow

### Leonardo

- Each dab carries two strengths in fixed point (`FUN_1400cef50`): `+0xbc` (flow / per-dab
  strength) and `+0xba`/`+0xb8` (opacity related), plus the stroke-level opacity from the brush
  (`+0x78`) stored in `+0xc4`.
- Dabs are rasterised into a **per-stroke brush tile** (`br_tile.c`, 128×128 int16 RGBA, one per
  touched tile and level), not into the layer: `FUN_1400d03e0` blends each dab row into the brush
  tile with `dst = (dst − src) × k >> 14 + src`, a lerp toward the dab colour with the dab
  strength `k`. Overlapping dabs inside one stroke therefore converge toward the dab value
  instead of stacking without bound.
- The brush tile is composited into the layer **once per stroke and tile** (`FUN_1400d18c0`), with
  the stroke opacity. A stroke cannot exceed its opacity no matter how often it crosses itself.

### Sketch2D

- `strength = opacity × flow × pressure curves` per dab, and every dab is blended straight into the
  layer pixels (`StrokePainter::paint`, "src over" with `a = coverage × strength`).
- Opacity therefore acts like flow: dabs of one stroke stack until the pixel reaches 100 %.

### Differences and test results

| Test | Expected (Leonardo model) | Sketch2D |
|---|---|---|
| opacity 0.5, flow 1, one straight stroke | alpha 0.50 | **1.00** |
| opacity 1, flow 0.1, one straight stroke | builds up between 0 and 1 | 0.88 ✓ |
| opacity 0.5 scribble crossing itself | ≤ 0.5 everywhere | **1.00** |

## 3. Rendering and levels

### Leonardo

- A stroke is a link on the tile tree. Workers render each touched tile **at the level that is
  being shown**, stamping the dabs at 1/2^L scale into a brush tile of that level
  (`FUN_1400d11f0(brush, L, x >> L, y >> L)`), then fill in finer levels later coarse-to-fine.
- Small dabs at level 0 are super-sampled by splitting the brush tile into four children at
  double resolution.

### Sketch2D (after engine step 6)

- Zoomed out to level ≥ 2 the dabs are stamped once into a preview at the shown level on the UI
  thread and recorded; on finish the full-resolution dabs run as engine jobs. Levels 0 and 1 are
  painted eagerly at full resolution on the UI thread. There is no super-sampling of small dabs.

### Differences and test results

- Long mixed session (21 strokes, sizes 8–3000 px, zoom 2…1/64): the worst single stroke took
  minutes of wall time in the headless run and the worst release took about 7 s. The
  full-resolution work of huge brushes zoomed out is still done on the UI thread at levels 0/1
  and in `settlePaint()` calls, and every dab of a 3000 px brush touches hundreds of full-res
  tiles.
- Leonardo's cost per dab is bounded by the shown level's pixels, and the full-resolution fill
  is background work that never blocks input.

## 4. Fix plan, in Leonardo's terms

1. **Stroke buffer + curve + arc-length placement.** Keep the raw samples of a stroke, evaluate
   position and pressure on a Catmull-Rom curve through them, place dabs by solving for the next
   curve parameter at `size × spacing`, keep the walk position between input events.
2. **Per-stroke brush layer with opacity applied once.** Dabs of a stroke lerp into a stroke
   buffer with their flow strength; the stroke buffer is composited into the layer with the
   stroke opacity, so opacity caps the stroke and self-overlap cannot exceed it.
3. **Render per shown level for every level, not just ≥ 2.** Use the same per-level brush tile
   path at all zooms and push the full-resolution fill to the engine, coarse to fine.
4. **Quantised dab positions with cached masks.** Snap dab centres to 1/16 px (coarser for large
   dabs) and reuse rasterised masks.

Each step lands with its own tests (the BrushWorkflow suite plus focused unit tests), mutation
checks, all suites including GPU, and a published build.
