# Sketch2D engine refactor: from eager full-resolution to the Leonardo model

Reference: `docs/research/LeonardoEngineAnalysis.md`. This file lists what the current engine does
differently from the five principles that make Leonardo instant, and an ordered plan in which
every step ends in a working, shippable app.

## A. What the current engine does wrong

| # | Current (file) | Why it is slow | Leonardo | Change |
| --- | --- | --- | --- | --- |
| 1 | Layer = flat `Map<TileCoord, Tile>` at full resolution only (`Layer.hpp`) | Only level 0 exists per layer; everything coarser is derived afterwards | Per-layer quadtree whose top 7 levels are the mip pyramid | `LayerTree`: per-layer level maps 0..6 with parent/child addressing; mips belong to the layer |
| 2 | Mips are built from the **flattened composite**, not per layer (`MipPyramid::update(mCompositor, level)`) | Any change re-composites full resolution, then downsamples; zoomed out you pay full-res compositing + 2×2 chains every frame | Each layer keeps its own levels; compositing happens at the displayed level | Composite per level from per-layer level tiles; full-res composite only when zoom ≥ 1 |
| 3 | Every edit is applied **eagerly at full resolution** (`stampDab`, `applyPixelFunction`, `previewTransform`, bucket) | Cost ∝ affected canvas pixels; a 2000 px brush or a 4k-layer transform stalls the UI | Edits are links; pixels computed lazily per visible tile per level | `Operation` objects queued per tile per level; nothing computed at submit time |
| 4 | Brush dabs always stamped at level 0, mips downsampled later | Big brush zoomed out = thousands of full-res pixels per dab | Dabs stamped directly at 1/2^L for each level that is needed | Dab rasteriser takes a level; stroke operations evaluated at the visible level first |
| 5 | Transform preview restores the whole layer and resamples every pixel on every pointer move (`Document::previewTransform`) | O(layer pixels) per mouse move | New layer version = matrix + reference to old version; kernel samples same level | Transform is an operation over a frozen source state; preview draws the visible level only |
| 6 | Paint workers (`StrokePainter`) only parallelise dab stamping; compositor/mips run on the UI thread inside `render` with paint paused (`whilePaintPaused`) | Rendering waits for painting and vice versa | Separate server loop; worker loop coarse→fine under a ms budget; render never waits | `Engine` thread owning the work queues; render only reads finished tiles; ms budget per frame |
| 7 | No fallback: a tile is either up to date or the frame waits for it | Hitches when work piles up | Missing/unfinished tile → draw coarser parent | Renderer picks the finest *ready* level per screen tile |
| 8 | Upload budget counted in tiles; level never coarsened under pressure | Large invalidations upload for many frames at full detail | If dirty > 200 k px, show one level coarser for 5 frames | Pixel budget + automatic one-level fallback |
| 9 | History stores `Layer::State` maps of shared tiles (OK) but transforms/adjust snapshot whole layers and replay | Copy is cheap, replay is not | Copy-on-write tree; operation over an old version is O(1) | Operations reference immutable source versions; undo swaps versions |
| 10 | 8-bit premultiplied RGBA tiles of 256 px | Banding when compositing many low-opacity dabs; 256² tiles make each dirty unit 4× larger | 16-bit fixed point, 128² tiles | Move to 128 px tiles first (cheap); 16-bit storage as a later step |
| 11 | All tiles always resident in RAM | Memory grows with canvas; nothing can be evicted | Compressed blobs, evicted when cold | Optional final step: compressed cold tiles |

## B. Roadmap (each step: builds, all tests green, app usable, APK published)

**Step 1 — Measure and freeze behaviour.**
Add frame-time / work counters (already `TP_PROFILE_*`) to a headless scenario suite: big brush
zoomed out, 4k-layer transform drag, bucket, adjustment. These become the acceptance numbers.
No behaviour change.

**Step 2 — 128 px tiles.** Flip `SKETCH2D_TILE_SIZE` to 128, fix tests that assume 256.
Smaller dirty units, matches the reference. Working state: identical app, finer invalidation.

**Step 3 — Per-layer level pyramid (`LayerTree`).** Each layer owns levels 0..6; a write at level 0
marks the parent rects dirty and they are rebuilt from the dirty rect only. Compositor composites
the *displayed level* from per-layer level tiles. `MipPyramid`-on-composite is removed.
Working state: zooming out no longer re-composites full resolution.

**Step 4 — Coarser-parent fallback + pixel upload budget.** Renderer draws, per screen tile, the
finest ready level; dirty upload capped at 200 000 px with one-level fallback for 5 frames.
Working state: no frame ever waits for uploads.

**Step 5 — Engine thread and operation queues.** Introduce `Operation` (kernel + params + bbox +
source version) and per-level ring queues. A dedicated engine loop drains coarse→fine under a
ms budget; the UI thread only submits operations and reads ready tiles. Existing eager code is
wrapped as level-0 kernels first, so behaviour stays the same.
Working state: same output, UI thread never blocks on painting.

**Step 6 — Strokes evaluated per level.** The dab rasteriser takes a level; a stroke operation
stamps its dabs at 1/2^L for each queued tile; the visible level is queued first, level 0 later.
Working state: big brushes instant at any zoom; full resolution catches up in the background.

**Step 7 — Transform as an operation over an immutable source.** Begin transform freezes the
layer version; every drag replaces one operation (matrix) — no restore/resample on the UI thread;
the kernel samples the same level of the source (nearest while dragging, bilinear/bicubic on
apply). Working state: transform drag instant on any layer size.

**Step 8 — Adjustments, filters, bucket, gradient, move as operations.** Port the remaining eager
edits to kernels over a source version so their previews also run per visible level.
Working state: every edit tool follows the model.

**Step 9 — History as version swaps.** Undo/redo swaps layer versions (already shared tiles);
operations hold their source version so redo needs no replay.

**Step 10 (optional) — 16-bit fixed-point tiles and cold-tile compression.** Storage format change
plus eviction of untouched tiles, matching Leonardo's memory model.

At the end of step 9 Sketch2D satisfies the five principles: work ∝ screen pixels, lazy
coarse-to-fine evaluation, capped upload with fallback, copy-on-write versions, and a render path
that never waits for the engine.

## C. Baseline (step 1, `EngineScenarios` in TestSketch2D, debug build, this machine)

| Scenario | Time on the calling thread | Dab pixels |
| --- | --- | --- |
| One 1500 px dab at zoom 1/16 (+ finish stroke) | 58.8 ms | 2 250 000 |
| Transform preview, 2048² layer, zoom 1/8 | 1791 ms | — |
| Bucket, 2048² layer | 545 ms | — |
| Invert, 4096² layer | 2279 ms | — |

Leonardo-model targets: dab and transform cost proportional to the visible level
(≈ 1/256 and 1/64 of these pixel counts), filters and bucket queued so the calling thread
returns in under one frame.

Step 2 (128 px tiles): same scenarios 62 ms / 2203 ms / 562 ms / 3558 ms — smaller tiles alone do
not help while every edit stays eager at full resolution; they are the unit for the steps below.

Step 3 (per-layer levels): `LayerLevels` keeps levels 1..6 per layer, rebuilt only for the changed
quarter of each parent; the compositor composites the displayed level from those level tiles,
so a zoomed-out edit re-composites one level tile instead of the full-resolution area.
`MipPyramid` (mips of the flattened composite) is removed.

Step 6 (strokes per level): zoomed out at level ≥ 2 a dab is stamped straight into that level's
tiles of the layer pyramid at 1/2^L scale (marked preview so downsampling leaves them alone),
while the full-resolution dab pieces are only recorded. Finishing the stroke commits history at once
and hands the recorded pieces to the engine as one stroke operation per tile; once the engine is idle
the previews are released and rebuilt from the real pixels. 8 dabs of 1500 px at zoom 1/16: 2.8 ms
on the calling thread, finish 3.1 ms (was 59 ms for a single dab plus finish).

Step 5 (engine): `Engine` owns per-level job queues drained coarsest-first by its own threads,
or in time slices on the UI thread when it has none. An `Operation` computes a target tile from a
frozen source and its result is applied (dirty marks, change notes, tile removal) on the caller's
thread. Rendering pauses the engine while it reads tiles. Filters and adjustment previews are the
first operations: invert on a 4096² layer returns in 31 ms instead of 2.2 s on the calling thread.

Step 4 (fallback + budget): the compositor keeps a cache per level; each frame composites the
wanted level and one coarser. If the wanted level needs more than 200 000 px uploaded, the coarser
level is shown for 5 frames while uploads catch up (coarse uploads first), and any tile of the shown
level that is not on the GPU yet is drawn from its parent clipped to its square.
