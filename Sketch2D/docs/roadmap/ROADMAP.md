# Sketch2D — the reference app feature parity roadmap

Reference: the reference application binary (sha256 `8df6fc28…`), analysed with REA/Ghidra.
Source layout recovered from the binary: `brush/`, `server/`, `app/`, `utilities/`, `win/`.

Rules for every item:

- One commit per item, extending the current UI (never replacing it).
- Reuse Math / Containers / Graphics / RasterRender / Widgets; extend those libraries when a
  generic piece is missing.
- Every item ships headless tests (`TestSketch2D`, plus `TestWidgets` for library additions).
- Behaviour, limits, defaults and labels follow the binary; evidence is noted per group.

## 0. Defects

- [x] 0.1 Premultiplied tile compositing (dark stroke outline)
- [x] 0.2 Remove tiles left fully transparent by a stroke, free their GPU images

## 1. Input and command foundation

Evidence: `in_shortcut.c`, `in_commands.c`, default shortcut table at `0x1408d11a0`
(113 bindings, flag 1 = repeat while held), `i_field.c`, `ui_escape_stack.c`.

- [x] 1.1 Full keyboard translation (letters, digits, punctuation, F-keys, keypad, editing keys) with
      modifiers and key auto-repeat
- [x] 1.2 Command registry and shortcut map with the reference app's default bindings and repeat flags
- [x] 1.3 Text field widget with text input, caret, selection, numeric +/- stepping
- [x] 1.4 Modal dialog and confirmation dialog
- [x] 1.5 Menus rebuilt on open (dynamic labels, enabled state, checked state)

## 2. History

Evidence: `sv_snapshot.c`; Edit menu labels `"%s: %s"`; History menu shows up to 6 redo and
12 undo states with "(N more)"; limit 2…1000, default 30 (`last_snapshot_limit`);
"Are you sure you want to clear history?".

- [x] 2.1 Snapshot model: tile and layer-stack swaps, history limit, branch truncation
- [x] 2.2 Undo / redo commands (Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z, Ctrl+Shift+Y, repeatable), Edit menu
      "Undo: <step>" labels, toolbar buttons
- [x] 2.3 History menu: state list, jump to state, current mark, "(N more)"
- [x] 2.4 Clear history (confirmation) and Set history limit dialog

## 3. Layers

Evidence: `sv_layer.c`, `sv_layer_cmd.c`, `p_layer.c`, `ui_layer.c`; snapshot labels
"Create layer", "Rename layer", "Move layer up/down", "Select layer", "Duplicate layer",
"Delete layer", "Clear layer", "Merge layers", "Merge all layers", "Show/hide layer",
"Lock/unlock layer", "Layer blend mode", "Layer opacity", "Bake layer opacity".

- [x] 3.1 Layer properties: lock, lock alpha, clip to below, blend mode, opacity (all undoable)
- [x] 3.2 Layer compositing with opacity, clipping and the 11 blend modes
      (Normal, Multiply, Screen, Overlay, Darken, Lighten, Color dodge, Color burn,
      Soft dodge, Soft burn, Color)
- [x] 3.3 New layer above current, duplicate, delete, rename, move up/down
- [x] 3.4 Select above / below / all, multi-selection
- [x] 3.5 Merge with below, merge selected, merge all (confirmation), clear layer (Ctrl+K)
- [x] 3.6 Opacity presets 0–100 %, opacity increase/decrease, bake opacity
- [x] 3.7 Layer panel: rows with thumbnail, name, visibility, lock, blend, opacity; drag reorder;
      Layer menu
- [x] 3.8 Layer errors ("You can't edit a locked layer", …) as balloons

## 4. Canvas rendering

Evidence: `sv_composite_tile.c`, `sv_atlas.c`, `sh_mip.c`, `win_d3d_atlas.c`, `r_draw_shadow.c`.

- [x] 4.1 Mip levels for zoomed-out drawing
- [x] 4.2 Board colour (HSV) and canvas background colour
- [x] 4.3 Framed canvas with drop shadow, infinite canvas toggle
- [x] 4.4 Single-channel view

## 5. Brush engine

Evidence: `br_*.c`, `sh_*.c`; parameters `size opacity flow texture interior hardness roundness
angle spacing grain_scale deckle_scale resaturation blur`, `min_*`, `firm_*`, `pressure_*`,
`stabilizer`, `use_marker_style_blend`, `linear_hack_v2`.

- [x] 5.1 Brush model with every parameter and per-parameter pressure (minimum, firmness, use)
- [x] 5.2 Dab shapes: roundness, angle, hardness, interior, spacing
- [x] 5.3 Presets: Round, Pencil, Ink, Marker, Calligraphy, Flat, Air, Jagged
- [x] 5.4 Paper grain and deckle textures
- [x] 5.5 Stabilizer
- [x] 5.6 Marker-style blending (linear hack ships with smudge in 5.7)
- [x] 5.7 Smudge (resaturation) and blur brushes, linear hack
- [x] 5.8 Eraser mode for every brush
- [x] 5.9 Brush list panel, brush options panel, reset brush (parameters / stabilization /
      textures / everything), brush size / opacity / flow / angle drag tools
      (`com_tools.c` command table: `while_down_brush_size/flow/angle/opacity` 1000182–1000185,
      `tool_brush_angle/size` 1000178/9, `brush_reset` 1000202, size ±1000204/5 by 15 %, angle
      ±1000206/7 by 10°, flow/opacity ±1000208–11 by 5 %; Brush menu labels
      "Reset all brushes", "Smaller brush", "Bigger brush", "Reset all brushes (everything)",
      "Reset all brush parameters/stabilization/textures", "Show Brush list" F4,
      "Show Brush options" F5; tablet: hold the size button or pick Adjust size/opacity/flow/angle
      in the quick menu, then drag on the canvas)
- [x] 5.10 Brush cursor: contour, cross, arrow, spotlight, crosshair for small brushes
      (`vi_cursor.c`, `vi_contour_draw.c`; View > Cursor: Contour / Brush / Arrow / Cross,
      "Show contour for eraser & smudge" on, "Show crosshair for small contour" on,
      "Show contour spotlight" off, spotlight colour HSV 60/1/1 at 0.4, "Show cursor for pen" on)

## 6. Colour

Evidence: `ui_color_wheel.c`, `ui_color_square.c`, `ui_color_disk.c`, `p_color_sliders.c`,
`p_color_swatch.c`, `p_color_history.c`, `ui_color_footer.c`.

- [x] 6.1 Colour panel pages: wheel, square, RGB, HSV, HSL sliders (page labels "Wheel",
      "Square", "RGB", "HSV", "HSL"; slider tracks show the colour gradient; grey picks keep the hue)
- [x] 6.2 Old / new colour compare, copy / paste colour (`ui_color_footer.c`: old/new swatch,
      hex text and "Copy" button; tapping the old half restores it; reopening marks a new old colour)
- [x] 6.3 Swatches (custom) and colour history (`p_color_history.c`: newest first, at most 7
      colours, each with its stroke count (`color_history_hex_%i`, `color_history_strokes_%i`);
      `p_color_swatch.c`: `swatches.json` with `"object" : "swatches"`, `created`, `modified` and
      `"data" : [ { "hex" : "#RRGGBB" } ]`; history and swatches always shown under the picker in the
      colour popup, + adds a swatch, hold a swatch to remove it)
- [x] 6.4 Default colours, swap, darker / brighter, grayer / purer, cooler / warmer,
      hue rotate, lightness presets (command handler `FUN_140006e10` cases 1000229–1000252:
      primary HSV 220/0.6/0.5 and secondary 0/0/1 (`color_primary`, `color_secondary`); default
      colours set primary value 0 and secondary 1; hue steps 10° (cooler/warmer clamp at 240° and
      never wrap past 0°/360°), saturation and value steps 5 %; lightness 0..100 % with Alt+`/1..0,
      and 0 followed quickly by 1..9 gives 1..9 %; copy/paste use `%02X%02X%02X` hex; new Color
      menu)
- [x] 6.5 Picker tool: sample all layers, sample opacity, styles balloon / cursor / magnifier / ring
      (settings `sample_all_layers` default on, `sample_opacity`, `picker_style_v2`; eyedropper tool
      and I key, hold Alt to pick while painting; the ring shows new over old colour; picking never
      changes the document)

## 7. Tools

- [ ] 7.1 Tool model and tool strip wiring (every placeholder becomes a real tool)
- [x] 7.2 Hand, zoom, rotate view (tool strip buttons, H / O shortcuts and a tablet Tools
      menu; hand drags the view, zoom clicks ×2 around the pointer, Alt-click zooms out, dragging
      scrubs one doubling per 150 px; rotate turns around the view centre and snaps to upright)
- [x] 7.3 Paint bucket (sample visible layers / alpha, threshold, leak detection) (reference
      defaults `bucket_threshold` 0.1, `bucket_all_layers` on, `bucket_only_alpha` off,
      `bucket_spread` 1 px; scanline fill over the composite of visible layers, a fill that
      reaches the open edge reports "Found a leak…", a framed canvas fills up to its frame; uses
      the brush colour and opacity, one undo step, U key, Color ▸ Paint bucket options, tablet
      Tools menu; `bucket_blur` not ported yet)
- [x] 7.4 Gradient (linear / radial, colour to transparent, eraser) (drag from the primary to
      the secondary colour over the canvas frame with the brush opacity; colour to transparent
      fades the primary out, the eraser gradient fades existing pixels; needs a framed canvas as in
      the reference, "You must have a framed canvas or a selection to use gradient"; G key, tool
      strip, Color ▸ Gradient options, tablet Tools menu; selections arrive with section 8)
- [x] 7.5 Move (move / duplicate pixels, only selection) (move and duplicate pixels of the
      active layer by whole pixels, live while dragging, one undo step; Alt-drag duplicates; with a
      selection only the selected pixels move, scaled by its soft coverage, and the selection moves
      along; V key, tool strip, Layer ▸ Move tool, tablet Tools menu)
- [x] 7.6 Transform (free, flip, flop, proportional), rotate, resize (Ctrl+T / Insert / tool
      strip / tablet Tools menu start a free transform of the active layer: drag inside moves,
      corner handles scale around the opposite corner, outside rotates around the centre, Shift or
      the Prop. button keeps proportions; Flip / Flop on the transform bar or Ctrl+F / Ctrl+Shift+F;
      Enter or Apply commits one undo step, Esc or Cancel restores; picking another tool applies;
      "You can't transform an empty layer"; the tool strip now scrolls when it does not fit)
- [x] 7.7 Crop (crop tool drags a region with the outside shaded; it becomes the canvas frame
      and pixels outside it are removed from every layer, one "Crop pixels" undo step; Ctrl+Del
      crops to the current frame or picks the tool on an infinite canvas; reference messages
      "Please choose a region to crop" and "You can't crop a alpha locked layer"; tablet Tools)
- [ ] 7.8 Export tool (region)

## 8. Selection

Evidence: `sv_select_all.c`, `sh_flood.c`, `sh_ant.c`, `p_selection_adjust.c`.

- [x] 8.1 Selection mask with anti-aliasing and marching ants (tiled 8-bit coverage mask with
      4×4 supersampled edges, part of every undo step; brush, eraser, smudge, bucket and gradient
      are clipped by it; outlined with dashed black / white ants; ant animation still static)
- [x] 8.2 Rectangle, ellipse, polygon and lasso selection, add / subtract (K and L keys, tool strip,
      Select menu and toolbar Select button, tablet Tools menu; Shift adds, Alt subtracts; polygon
      lasso closes on its first point or Enter, Esc cancels; click to deselect option)
- [x] 8.3 Magic wand (reference defaults `magic_threshold` 0.1, `magic_all_layers` on,
      `magic_only_alpha` off; shares the bucket's flood fill but also selects open areas; Shift
      adds, Alt subtracts, one undo step; W key, tool strip, Select ▸ Magic wand options, tablet)
- [x] 8.4 Select all as rectangle / lasso / canvas frame, invert, deselect (Esc) (Ctrl+A selects the
      bounding rectangle of the active layer's pixels, Ctrl+Shift+A selects exactly its pixels by
      alpha, select canvas frame, Ctrl+I invert, Ctrl+D / Esc deselect; all undoable)
- [x] 8.5 Spread and blur selection (Select ▸ Spread / Blur selection by the chosen amount,
      -8…+16 px; spread grows or shrinks, also for inverted selections; blur is a separable box blur
      of the coverage)
- [x] 8.6 Fill colour, fill to new layer underneath, auto fill, clear selection (fills with the
      primary colour scaled by coverage; underneath adds a layer below the active one; auto fill
      fills each new rectangle / lasso selection; Delete clears selected pixels or the whole layer;
      reference messages for no area, inverted selection and alpha locked layers)

## 9. Edit and adjustments

- [~] 9.1 Cut, copy, paste, copy to new layer (system clipboard) (Ctrl+X / C / V and Edit ▸ Copy to
      new layer: copies the active layer's pixels inside the selection at their position, soft
      edges included; paste adds a "Pasted" layer and starts a free transform; app-internal
      clipboard only — exchanging images with other apps needs image clipboard support in the
      platform window layer, which only handles text today)
- [x] 9.2 Fill, clear, crop, flip horizontal / vertical (done in 7.6, 7.7 and 8.6; now also listed
      in the Edit menu, which the tablet quick menu exposes)
- [x] 9.3 Desaturate, invert, value to alpha (and inverted) (on unpremultiplied colour, luma
      0.299/0.587/0.114, limited by the selection's coverage, one undo step each; value to alpha
      refuses alpha locked layers)
- [x] 9.4 Adjustments with live preview: brightness / exposure, hue / saturation, colour balance,
      colorize, alpha (Edit ▸ Adjust and the reference shortcuts Ctrl+L / Ctrl+B / Ctrl+U open a
      dialog whose sliders re-render the active layer from its original pixels while dragging;
      Apply is one undo step, Cancel / Esc restores; limited by the selection; exposure in stops,
      hue/saturation/lightness and colorize in HSL; alpha leaves alpha locked layers alone)

## 10. Lines and guides for drawing

Evidence: `p_line_ellipse.c`, `sv_perspective.c`, line panel strings.

- [~] 10.1 Normal line: hold still for a straight line, smooth with red tail, catch-up (hold still for
      line done with the reference defaults, on and 0.5 s: holding the pen still turns the stroke
      into a straight line from its start that then follows the pen, one "Line" undo step; Line
      menu on the menu bar, toolbar Line button and tablet quick menu; hold time saved with the
      session; the smooth line with red tail is still open)
- [~] 10.2 Line, ellipse, ellipse in 3D, ruler (Line menu modes with the reference Ctrl+1 / 3 / 4:
      Line is freehand with hold for line, Ruler draws a straight line from the press to the pen,
      Ellipse draws an ellipse in the dragged box, Shift for a circle; each one undo step, mode
      saved with the session; ellipse in 3D still open)
- [x] 10.3 Perspective 1 / 2 / 3 point, isometric with angle, radial (Line ▸ Perspective mode,
      Ctrl+5, and Isometric, Ctrl+7: strokes are straight lines snapped to the closest guide —
      towards a vanishing point, or horizontal / vertical where the type has them; isometric
      angle 26.6° / 30° / 45°; guides drawn while the mode is active; move a vanishing point by
      dragging after Move perspective point, lock, reset; saved with the session)
- [x] 10.4 Symmetry (show / hide / move / lock / reset) (Line ▸ Symmetry, Ctrl+6: every brush and
      eraser dab, also of ruler and ellipse strokes, is mirrored across a line that starts vertical
      through the view centre; show / hide, move (drag after choosing it), lock, reset; one undo
      step per stroke; saved with the session)
- [ ] 10.5 Snap to guides and grid, clip to sides, show angle / length, keep proportions

## 11. Canvas and view

- [ ] 11.1 Pan, zoom at cursor / centre, zoom snap, latch 100 %, fit on screen, max in / out
- [~] 11.2 Rotate left / right / reset / snap, flip view, lock view (rotate left / right / reset and
      the snap to 0° already existed; View ▸ Lock view now freezes pan, zoom and rotation for every
      input; flip view still open — it needs mirrored drawing in the tile shaders)
- [x] 11.3 Full screen (View ▸ Full screen and F11 toggle xdg-toplevel full screen through new
      Window::setFullScreen / isFullScreen in the Graphics library; Android is always full screen)
- [ ] 11.4 Rulers and guides (create, move, delete, lock, colour, snap)
- [ ] 11.5 Grid (block size, subdivisions, line colours, thick lines)
- [ ] 11.6 Canvas frame size, presets (A3, A4, Letter, Legal, Tabloid, portrait / landscape),
      units (px, mm, cm, in) and resolution, clear canvas

## 12. Files

Evidence: `sv_file.c`, `sv_save.c`, `sv_serialize.c`, `sv_unserialize.c`, `im_*`,
`im_photoshop_*`, `ui_export_bar.c`.

- [ ] 12.1 Native .sketch2d document: new, open, save, save as, close, unsaved-changes prompt
- [ ] 12.2 Open recent, auto open, auto save, crash recovery
- [ ] 12.3 Import images (PNG, JPEG, BMP, TGA, TIFF) and new file from clipboard
- [ ] 12.4 Export PNG, JPEG (quality), BMP, TGA, TIFF, PSD; PPI; each layer separately
- [ ] 12.5 Export bar: thumbnails, copy to clipboard, show in folder, delete, print

## 13. Panels, settings and shell

- [ ] 13.1 Show / hide panels, close all panels, sidebar, numpad, console, performance panel
- [ ] 13.2 Settings: user interface, side margins, window opacity, left handed, flip toolbar / ribbon
- [ ] 13.3 Shortcut editor (shortcuts.json, reset, show file)
- [ ] 13.4 Tooltips, balloons, escape stack, wait dialogs for long work

## 14. Input devices

- [ ] 14.1 Pen input through Wayland tablet protocol, pen pressure mapping (min / max input,
      output curve, firmness), pen / mouse auto detection
- [ ] 14.2 Right / side button mapping, wheel / dial actions
- [ ] 14.3 Multi-touch: pinch zoom / rotate, one-finger hand, three-finger tap undo

## Out of scope

Account and licensing, trial, call-home, software update, rate-us, localisation.
