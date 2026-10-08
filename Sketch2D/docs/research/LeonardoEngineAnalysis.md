# Leonardo 0.17.70 engine analysis

Source: `leonardo.0.17.70.win64.exe` (sha256 8df6fc28…0eb), analysed statically with Ghidra 12.1
through REA. Function addresses are virtual addresses in that binary. Every claim is marked
**observed** (read from decompiled code or strings) or **inferred** (deduced, not yet confirmed).

## 0. Module map

Assert strings embed the original source paths, so each function can be attributed to a source
file. Files are laid out contiguously in the binary in this order (addresses are the first
attributed functions):

| Area | Files | Address range |
| --- | --- | --- |
| Renderer front end (`r_*`) | r_cbuffer, r_commands, r_draw*, r_image, r_main, r_media | 0x140031000–0x14003a400 |
| View / canvas drawing (`vi_*`) | vi_contour_draw, vi_contour_raz, vi_cursor, vi_draw, vi_paint_canvas | 0x14005fc00–0x140072000 |
| Brush engine (`br_*`) | br_brush, br_init, br_scan, br_stroke, br_alloc, br_smudge, br_tile, br_misc, br_texture, br_stamp, br_stage | 0x1400c5590–0x1400e1e90 |
| Shapes / selection (`sh_*`) | sh_span, sh_main, sh_ant, sh_mip, sh_flood | 0x1400e050e–0x1400e5950 |
| Server (`sv_*`) | init, misc, status, layer, select_all, snapshot, sample, grid, unit, warning, blob, composite_point, perspective, database, assert, alloc, link, parse, debug, commands, node, kernel_misc, unmap, calculate, frame, save, kernel_adjust, composite_tile, composite_rect, kernel, unserialize, serialize | 0x1400e8580–0x140112060 |
| Server storage (`x9_*`, `se_*`) | x9_compress, x9_assert, x9_daemon, x9_conn_loop, se_zone, se_mem | 0x1401029e0–0x140117460 |
| Image utilities (`im_*`) | blur, mask, edit, info, read, write, mipmap, resize, cache, photoshop | 0x14011e000–0x14013b180 |
| Direct3D back end (`win_d3d_*`) | atlas, image, init, main, draw, utils | 0x140136e70–0x140141ee3 |

The full function → file table is in `docs/research/leonardo/file_funcs.json`.

## 1. Layer storage: one quadtree that is also the mip pyramid

Functions: `FUN_140103390` (sv_node.c, assign node keys), `FUN_1400feac0` (sv_link.c, schedule
work into the tree), `FUN_1401036e0` (sv_alloc.c / sv_work.c, allocate a link).

**Observed**

- All nodes live in one global array of 0x78-byte records (`DAT_1408f8ff8`, count
  `DAT_1408f8fdc`); a node is referenced by a 32-bit index, never by pointer. Index 0 is "none".
- Each node has 4 child indices at +0x1c..+0x28 and a reference count at +0x08.
  `FUN_140103390` walks the tree assigning each node a depth (+0x16, asserted ≤ 15) and a
  2-bits-per-level path key (+0x18): `key = child << (depth*2) | parentKey`.
- Depth 0..15 → **16 levels**. The code computes `level = 15 - depth`. Nodes with
  `level < 7` (the seven finest levels) carry pixels (state 2/3: "has data"/"empty"); coarser
  nodes are pure index nodes (state 1).
- Copy-on-write: before writing, a node with refcount > 1 is duplicated into a new record
  (state 5, refcount 1) and the parent re-pointed (`FUN_1400feac0`, branch on `+8 > 1`). This
  is what lets every history snapshot share unchanged subtrees.
- Each pixel node keeps a dirty rectangle in four bytes (+0x34..+0x37) in sub-tile units,
  widened whenever new work touches it.

**Inferred**

- With 128 px tiles at level 0, a level-*L* node spans 128·2^L canvas pixels, and the seven
  pixel levels are the mip chain: the mip pyramid is not a separate structure, it *is* the
  upper part of the same tree. A root at depth 0 spans 128·2^15 ≈ 4.2 M px per side.

## 2. Operations are "links" scheduled lazily per level

**Observed** (`FUN_1400feac0`, `FUN_1401036e0`, `FUN_1400ff820`)

- Every edit (stroke, transform, fill, adjustment) is a *link*: a 0x1f0-byte record from a pool
  of 8 KB chunks holding a kernel callback (+0x180), its parameters, a bounding box
  (+0x98..+0xa4, clamped to ±100000) and flags.
- Scheduling recurses down the quadtree from the root, culling by the link's box, by an optional
  circle (flag 0x2000: centre +0xa8/+0xac, radius +0xb0) and by optional half-planes
  (flag 0x4000, list at +0xb4/+0xb8), and by masks from other layers (flags 0x1000 / 0x2 —
  clipping and selection).
- For each touched pixel node (level < 7) it pushes a 16-byte work item
  `{link, node, rect bytes}` into a **per-level ring queue** (`DAT_1408f94f0 + level*0x20`), and
  counts pending work per level on the link (+0x20 + level*8). Queue overflow logs
  "Work overflow, dropping work".
- Nothing is computed at scheduling time — only queued.

**Inferred**

- Workers drain the queues coarse-first or for the levels the view needs, so a giant stroke
  costs one queue item per touched *visible-level* tile, and finer levels are filled in later.

## 3. Limits of scale and space

| Limit | Value | Evidence |
| --- | --- | --- |
| Tree depth | 16 levels (depth 0..15, asserted ≤ 15) | `FUN_140103390` |
| Pixel levels | 7 finest (level 0..6); coarser levels are index-only | `FUN_140103390` (`15 - depth < 7`) |
| Tile | 128 × 128 px, 16-bit RGBA (512 B rows, 4 channels asserted) | `FUN_140106330` asserts `0x80`, `0x200`, `4` |
| Max canvas span | 128 · 2^15 = 4 194 304 px per side (inferred from depth × tile) | |
| Edit / link bbox clamp | ±100 000 px | `FUN_1401039e0`, `FUN_1400fda60` (0x186a0) |
| Node array | starts at 32, grows on demand in steps of 32 (no fixed cap) | `FUN_1400f64e0` |
| Zoom | `zoom_in_limit` default −6, `zoom_out_limit` default 1 (log2 → ×1/64 … ×2) | settings table @0x140003900 |
| Coarsest drawn mip | level 6 → a screen pixel can stand for 64 × 64 canvas px | pixel levels |

So: drawing is effectively limited to ±100 000 px (a 200 k × 200 k region), the tree can address
4.2 M px, and zoom-out stops at 1/64 — exactly the depth of the pixel mips, so every zoom has a
matching pixel level and nothing is ever computed finer than the screen needs.

## 4. The worker drain loop: coarse first, time-boxed

`FUN_140107ce0` (`SV_CalcLoop`, sv_calculate.c) — **observed**

- Picks the **highest non-empty level** (6 → 0) from the per-level queues each iteration, so
  coarse mips are computed before fine ones.
- Runs until a millisecond budget (argument) expires, then returns; called from the server frame
  (`SV_CalcLoop( stopMinusTwo + 1/2, flags )` in sv_layer.c), interleaved with
  `SV_SampleLoop`, `SV_FloodLoop`, `SV_SelectLoop`, `SV_SaveLoop`, `SV_UnmapLoop`.
- Each item runs `FUN_140106330` (`WorkOnLink`): fetch the tile (`FUN_1400f6f20`; if it is not in
  memory yet → "Waiting on node", item retried later), build a sample context, call the link's
  kernel (`link+0x180`, or default compositor `FUN_14010c7c0` row by row with an optional
  selection mask), then mark the 1024-bucket upload cache dirty for that rect.
- When the touched node is the top of a 7-level chain, it immediately rebuilds the 6 coarser
  mips by 2×2 downsampling only the dirty rect (`FUN_140105c50`).

## 5. Building a frame: choose the mip, cap the upload

`FUN_140109980` (`SV_BuildFrame`, sv_frame.c) — **observed**

1. Walk every visible layer tree (`FUN_140109500`) in *count* mode (flag 8): for each quad at the
   wanted level, `FUN_140108ac0` returns how many pixels of it are dirty in the GPU cache.
2. If the total exceeds **200 000 px** (0x30d40), the floor mip for this frame becomes
   `previous + 1` (one level coarser) and a 5-frame hold counter is set.
3. `FUN_1401098f0` picks the floor mip = max over layers of the finest level that is actually
   computed; levels still waiting in the work queues are skipped (falls back to a coarser node).
4. `FUN_140109690` (`SV_EmitQuadsAndUploadTiles`) emits one textured quad per node at that level
   (`FUN_140109330` for normal quads, `FUN_140109240` for solid/empty ones) and uploads the dirty
   rects through the 1024-bucket LRU tile cache; per-layer timings are recorded.

Effect: screen work per frame is bounded both in quad count (one level per screen) and in upload
bandwidth (≤ 200 k px); anything not ready shows the coarser parent instead of waiting.

## 6. Transform: a new layer version whose pixels are a function of the old one

`FUN_1400fa0d0` (`.transformDelta <x> <y> [rotate] [x-scale] [y-scale]`, sv_commands) and kernel
`FUN_140104b40` (sv_kernel_misc.c) — **observed**

- Each drag update creates a snapshot "Transform update". For every selected layer it
  1. bounds the source (`FUN_140103270`), maps the bbox through the matrix (`FUN_1400ebd70`) and
     pads it by the max scale (ceil),
  2. **forks a new root** for the layer (`FUN_140102fc0`) — the old tree stays as the source,
  3. creates one link (flags |= 0x10, "full rewrite") with kernel `FUN_140104b40` and stores the
     forward and inverse 2×3 matrix + pivot (link +0x144..+0x16c and +0x1b0..+0x1dc),
  4. schedules it into the per-level queues (`FUN_1400ff820`). Nothing is resampled here.
- The kernel runs per destination tile *at that tile's level*: `FUN_1400eb650` rescales the
  inverse matrix by `1 / tileScale` (the level), then for every destination pixel computes the
  source position and samples the **same level** of the old tree:
  - mode 0: nearest (copy 4 × int16),
  - mode 1: bilinear in fixed point (`>> 14`, 1.0 = 16384),
  - mode 2: bicubic 4 × 4 (16 taps per channel, clamped 0..1).
- The source tile ring is provided by the sampler (`HandleSampleTile`, `HandleSampleRect`); if a
  needed source node is not resident the item is retried later instead of blocking.

Consequence: dragging a transform only costs one snapshot + one link per layer. The visible
level (e.g. level 3 when zoomed out) is resampled from level 3 of the source — 64× fewer pixels
than full resolution — and full resolution is produced in the background or when you zoom in.
The same lazy pattern explains "big brush is instant".

## 7. Brush: a stroke is a link whose kernel stamps dabs into a per-level brush tile

`FUN_140106330` (`WorkOnLink`), `FUN_1400d11f0` (br_tile.c alloc), `FUN_1400cd8b0` (br_stroke.c),
`FUN_1400d1740` (br_tile.c rasterise), `FUN_1400e1520` (br_stage.c) — **observed**

- A stroke link carries a brush object (link +0x190). When a worker reaches a tile of level L for
  that link, it asks the brush for a **brush tile at level L**:
  `FUN_1400d11f0(brush, L, tileX >> L, tileY >> L)` — coordinates are shifted by the level, so a
  zoomed-out tile is rendered by stamping the dabs at 1/2^L scale, not by downsampling a
  full-resolution result.
- Brush tiles are 128 × 128 × 4 int16 (0x20000-byte buffers from a free list), allocated lazily.
  For brushes whose effective size × spacing is below per-feature thresholds, the level-0 tile is
  split into 4 children at double resolution (super-sampled small dabs).
- The dabs touching a tile are rasterised scanline by scanline (`FUN_1400d1740` →
  `FUN_1400d03e0`), then the tile is composited into the layer node with the 16-bit
  fixed-point blend (`FUN_1400d16c0`, values `>> 14`).
- Pressure/size curves are Catmull-Rom splines over a sample table (`FUN_1400e1520`).

So a stroke is rendered per **visible tile at the visible level**. A 2000 px brush zoomed to 1/16
costs about a 125 px brush; the full-resolution version is filled in later by the coarse-to-fine
worker loop. This is the main reason big brushes stay instant.

## 8. Storage: tiles are compressed blobs in a database, mapped on demand

sv_blob.c, sv_unmap.c (`FUN_140105990`), x9_compress.c (`FUN_140112800`), x9_daemon.c,
x9_conn_loop.c — **observed**

- Every pixel node's tile is a *blob* with an id, SHA-1 (checked by `FSCK`), access time and
  mapped/unmapped state ("%i rows, %i stubs, %i blobs, %i mapped"). The document file is that
  blob database ("Leo zone format").
- A node can exist without its pixels in RAM. Workers needing an unmapped tile request it and the
  item is retried ("WorkOnLink: Waiting on node", "re-requested blob").
- `SV_UnmapLoop` drops cold tiles from RAM (state → 3, buffers freed, `FUN_140105990`) once the
  memory high-water mark is exceeded (difference > 0x2ee000 units); dirty tiles are saved first
  (`FUN_140109f70`).
- Blob codec header: 1 type byte + int32 size. Types: 1 raw, 2 / 3 / 4 three different
  decoders, 5 and 6 two-stage (entropy decode then a second transform). Compression output buffer
  is sized `n * 1.01 + 256`.
- x9_daemon / x9_conn_loop run the database as a separate message loop ("X9: Unknown message
  type", token protocol), i.e. saving and loading never stall drawing.

## 9. Summary: why Leonardo is instant

1. Work is proportional to **screen pixels**: each frame uses one level of the quadtree matching
   the zoom; brushes and transforms are evaluated at that level directly.
2. **Nothing is computed eagerly**: edits become links queued per tile per level; workers run
   coarse-to-fine under a millisecond budget; unfinished tiles fall back to their parent.
3. **GPU upload is capped** at 200 000 px per frame, coarser level if exceeded.
4. **Copy-on-write nodes** make history, transforms (old tree = source) and layer forks O(1).
5. **16-bit fixed-point** pixels and integer blending; memory pressure handled by compressed
   blobs that are unmapped when cold and loaded asynchronously.
