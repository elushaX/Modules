# Sketch2D — infinite-canvas raster painting app

A classic-style infinite-canvas raster painter, built on the user's own
Math / Containers / Graphics (Vulkan) / Widgets libraries.

## Mechanisms replicated (verified via REA + Ghidra against
the reference application binary, source tree `C:\xade\code\`)

- **Sparse runtime tiles joined in 2D.** The canvas is an unbounded 2D grid of
  fixed-size raster tiles, allocated on demand and stored sparsely (hash map
  keyed by integer tile coordinate). Empty regions cost nothing.
  (`server/sv_composite_tile.c`, `sv_layer.c`.)

- **Per-frame dirty-rect uploads.** Each frame only the changed pixels of the
  affected tiles are uploaded to the GPU, not whole tiles.
  Evidence string: `"Frame %i: uploaded %6i pixels in %i tiles in %i ms"`
  (`SV_EmitQuadsAndUploadTiles(frame, api, floormip)`, `FUN_140109980`).

- **Dab-based brushes.** A stroke is a sequence of dabs (stamps) composited into
  the tiles it covers, marking per-tile dirty rects.
  Evidence strings: `"%i dabs, %i tiles, %s"`, `"%.0f dabs is %.1f MB"`
  (`brush/br_tile.c`, `BR_ClearBrushTiles`).

- **Atlas / texture size buckets.** Tile textures are packed into atlas buckets
  of 128 / 256 / 512 px (texture size classes), distinct from the canvas tile
  edge length.

- **Debug tile/atlas view.** The original exposes `debugTiles`,
  `skipRenderTiles`; Sketch2D provides an overlay showing the tile grid, per-tile
  dirty rects, and atlas occupancy.

## Open unknown (recorded in REA)

- Exact canvas tile edge length in the reference app is not yet proven from the binary
  (the 128/256/512 numbers are atlas buckets). Sketch2D uses a 256 px tile as a
  single tunable constant (`sketch2d::kTileSize`).

## Rendering approach (this build)

- **Dedicated Vulkan tile pass.** Sketch2D renders its tiles directly in its own
  Vulkan pass into the swapchain frame (a textured-quad pipeline), *before* the
  nanovg UI is drawn on top by the engine's Canvas. This keeps the hot path
  (many tiles) off the vector renderer.

## Build order

1. Tile model — sparse `Map<TileCoord, Tile>`, CPU raster (RGBA8), dirty rect.
2. Canvas document — world/tile/screen transforms (pan + zoom).
3. Vulkan tile renderer — per-tile VkImage, dirty-rect upload, textured-quad
   pipeline, one draw per visible tile.
4. Dab brush — stamp a soft dab into covered tiles, mark dirty rects.
5. Sketch2D widget + app — infinite-canvas widget, pointer painting, pan/zoom.
6. Debug tile/atlas overlay.
