# Stroke commit and layer update: the right architecture first

Owner's request (task #4): not small optimizations but the architecturally right solution for
what happens after a stroke is released (engine tiles, levels, history); small optimizations
after that; possibly offload level updates to the GPU later.

## What happens today (build 569)

1. **While the pen is down** (`Document::stampDab`)
   - level 0 and 1 (zoom ≥ 1/2): every dab is rasterised immediately at full resolution on the
     UI thread through `StrokePainter` lanes, per-tile stroke masks cap opacity;
   - level ≥ 2: dabs are rasterised into *preview* tiles of the shown level
     (`LayerLevels::previewTile`) and recorded in `mDeferredDabs`.
2. **On release** (`Document::finishStroke`)
   - eager path: wait for painter lanes, `engine.drain()`, remove empty tiles, snapshot the whole
     document into history (`commit`);
   - deferred path: `flushDeferredDabs` turns the recorded dabs into one `StrokeTileOperation`
     per touched level-0 tile (rows merged into spans), submits them to the engine, commits a
     history entry that still points at the *old* tiles and patches it as results land
     (`patchHistory`, chained slots for strokes landing over strokes).
3. **Levels** (`LayerLevels::update`, called from the compositor each frame): changed level-0
   tiles are downsampled up the pyramid; since build 569 a parent waits until all four
   children have no engine work and is marked stale meanwhile.
4. **History**: a snapshot of every layer's tile map per entry (shared tiles, copy on write).

## Why this is architecturally wrong

| Problem | Effect | Root cause |
|---|---|---|
| Two stroke paths (eager vs deferred) chosen by zoom | Different code, different bugs (preview opacity, history patching, chaining); every feature implemented twice | Strokes are not a first-class object; "render now" vs "render later" leaks into the document |
| Full-resolution work on the UI thread at levels 0/1 | Huge brushes at 50–100 % zoom stall input; long workflow test: 7–12 s release | Work is proportional to brush area, not to the shown pixels |
| Previews only at level ≥ 2, and they are thrown away | Work is done twice (preview + full res); preview lifetime logic (`releaseFinishedPreviews`) is fragile | The shown level is not a real result of the stroke, just a temporary picture |
| History entry committed before pixels exist, then patched | `patchHistory` has to walk later entries and match tiles; subtle with undo during landing | History stores pixels instead of *what changed* |
| Level pyramid rebuilt by downsampling level 0 | Every landed tile triggers CPU downsampling up 6 levels; the shown level waits for level 0 | Levels are derived data instead of being rendered from the stroke |
| One global engine queue keyed by (layer, tile) with ad-hoc chaining | Ordering between strokes is enforced by chaining slots per tile | No notion of per-tile versions / dependency |

## The Leonardo model (docs/research/LeonardoEngineAnalysis.md §2–7)

- An edit is a **link**: an immutable description (stroke = sample list + brush) attached to the
  tiles it touches, at every level.
- A tile at level L is **computed from the link at level L directly** (dabs stamped at 1/2^L
  scale), never by downsampling level 0 for edits in progress.
- Work is scheduled **coarse to fine, visible first**, within a frame budget; unfinished tiles
  fall back to their parent.
- Tiles are **versions** (copy on write nodes); history keeps the old version, the new version
  is filled in lazily. Undo swaps versions; nothing is patched.

## Target architecture for Sketch2D

### 1. `StrokeEdit` — one immutable edit object
- Holds the stroke's dab list (positions, pressure-derived shape, colour, flow, opacity cap,
  erase/marker/alpha-lock flags, selection), its bounding box and the layer.
- Created at pen down, grows while the pen is down (appending dabs), sealed on release.
- The *same* object is used for preview and final pixels — no eager/deferred split.

### 2. Tile versions with pending edits
- Each layer tile slot holds `base version` + ordered list of pending `StrokeEdit`s.
- A tile at level L is *resolved* by applying the pending edits to the base at level L
  (dab rasterised at that level, stroke mask per tile and level for the opacity cap).
- Resolved results are cached per (tile, level, edit sequence); resolving level 0 also makes
  the level ≥ 1 cache entries for that tile obsolete only for that edit.

### 3. Scheduler: visible level first, then finer, then coarser
- Each frame the compositor asks for the shown level's visible tiles; unresolved ones become
  jobs at that level first (cheap: work ∝ screen pixels), then the next finer level, down to
  level 0 in the background, then remaining off-screen tiles.
- A job resolves one (tile, level) for the current edit list; jobs for one tile run in order
  (per-tile serial, as the engine already guarantees).
- The UI thread never rasterises dabs; it only appends dabs to the live `StrokeEdit` and marks
  the shown-level tiles dirty.

### 4. Levels: rendered for edits, downsampled only for imports
- While edits are pending, level L pixels come from resolving the edit at level L (step 2).
- When level 0 of a tile is final, its parents are *not* re-downsampled for that edit — they
  already hold the edit's level-L result. Downsampling stays for whole-layer imports, filters
  and transforms until those become edits too (roadmap steps 7–8).
- This removes the "rebuild parent once all children are final" logic introduced in 569 for
  strokes (it stays as the rule for downsampled data).

### 5. History as version swaps (roadmap step 9)
- A history entry stores, per touched tile, (old version, new version) — the new version is the
  tile with the edit applied, possibly not resolved yet.
- Undo/redo swap the version pointers; unresolved versions resolve on demand.
- No `patchHistory`, no chaining slots: ordering is the version chain.

## Status

- Every stroke now paints dabs only into preview tiles of the shown level, including level 0
  (`LayerLevels::basePreviewTile`). Full-resolution layer tiles are produced by
  `StrokeTileOperation` in the engine; release seals the stroke and commits history without
  waiting. Stroke opacity coverage is shared per tile across a stroke's flushes.
- Readers that need final pixels call `Document::settlePaint()`; the renderer does not.
- `tp::Map::remove` no longer rehashes on every call (it was O(n²) and dominated apply time).
- Long workflow (21 strokes up to 3000 px): release ≤ 370 ms (was 14–139 s), worst frame ≤ 740 ms.
- Remaining: the 3000 px stroke at 1/64 still flushes ~1 s of tile scheduling on the UI thread,
  and undo / sampling still settle; steps 1–2 and 5 below remove those.

## Order of work

1. Introduce `StrokeEdit` and route **both** current paths through it, keeping today's
   execution (eager at L0/1, deferred at ≥ 2). Tests: existing brush, engine and workflow suites
   stay green; new unit tests for StrokeEdit growth and sealing.
2. Tile versions + pending edit list on `Layer`; resolve at level 0 in engine jobs; delete
   `mDeferredDabs`, `StrokeTileOperation` chaining slots and `patchHistory`.
3. Resolve at shown level (any level, including 0/1) with stroke masks per level; delete preview
   tiles and `releaseFinishedPreviews`; scheduler visible-first then finer.
4. History stores version pairs; undo/redo swap.
5. Then small optimizations: dab mask cache with quantised centres, span merging, 16-bit tiles,
   per-level work budgets.
6. Later: GPU resolve of shown-level tiles (dab rasterisation and downsample in compute
   shaders), once the CPU model is final and measured.

Each step: integration tests first (quick suite), mutation checks, long workflow suite on
request, publish.

## Risks / open questions

- Memory: cached resolved tiles per level ×7 — bounded by evicting non-visible levels.
- Smudge reads canvas pixels while painting → needs its source tiles resolved at the stroke's
  level first (dependency on earlier edits on the same tile).
- Selection masks and alpha lock must be evaluated per level (selection needs its own pyramid).
