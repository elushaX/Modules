# Widgets

A small retained-mode UI library: minimal, flexible, headless-testable.

- `Widgets` — the library. Depends on `Math` only (plus the `InputCodes` /
  `Rect` / `Color` headers). No window, no GPU.
- `WidgetsApp` — the adapter that runs a `Ui` inside a `Graphics` window:
  `CanvasPainter`, `InputTranslator`, `WidgetApplication`.
- `WidgetsGallery` / `WidgetsExample` — a gallery of every widget, used both
  as a demo application and as an end-to-end test subject.
- `TestWidgets` — headless tests.

## Five concepts

| Concept | Responsibility |
|---|---|
| `Widget` | A node: children, sizing request, assigned rect, state, hooks |
| `Layout` | Optional strategy on a container: measure children, then arrange them |
| `Event` | A value delivered to widgets in their local coordinates; bubbles to ancestors until accepted |
| `Popup` | A widget shown above the tree, anchored to a rect, with placement and dismissal rules |
| `Ui` | The context: owns layout, input routing, popups, ticking, tooltips, stats |

## Frame

```
ui.dispatch(event)   input: hit test, hover diff, capture, click synthesis, bubbling, popup dismissal
ui.advance(deltaMs)  ticks: widget animations, geometry springs, tooltip dwell
ui.updateLayout()    only if something requested layout: measure (cached) + arrange (skips clean subtrees)
ui.paint(painter)    tree, then popups, each subtree translated and clipped
ui.needsFrame()      true while layout/paint is pending, something animates, or a tooltip is pending
```

Layout always runs before an event is routed and after it is handled, so
handlers and hit testing never see stale geometry.

## Writing a widget

A custom widget overrides only what it needs:

```cpp
class Swatch : public ui::Widget {
public:
	explicit Swatch(RGBA color) : mColor(color) {}

protected:
	ui::Extent measureContent(ui::Axis, halnf) const override { return ui::Extent::flexible(24); }
	void paint(ui::Painter& painter) override { painter.fillRect(localRect(), mColor, 4); }
	void onEvent(ui::Event& event) override {
		if (event.type() == ui::Event::Type::Click) event.accept();
	}

private:
	RGBA mColor;
};
```

Hooks: `measureContent`, `onEvent`, `onTick`, `paint`, `paintOverlay`,
`hitTest`, `acceptsKeyboardFocus`, `onHoverChanged`, `onAttached`,
`onDetached`. Requests: `requestLayout`, `requestPaint`, `startTicking`,
`stopTicking`. Every built-in widget is written against exactly this API.

## Sizing

Each axis has one `AxisSizing`:

| Mode | Meaning |
|---|---|
| `fit()` | Preferred size comes from content (text, icon, children) |
| `fixed(length)` | Exactly this length |
| `fill(weight)` | Content size, then grows with `weight` share of leftover space |

Each can be bounded with `.atLeast(min)` and `.atMost(max)`. Resolution
produces an `Extent { minimum, preferred, maximum, grow }` that layouts work
with.

`measureContent` declares how compressible the content is:

| Extent | Use |
|---|---|
| `Extent::natural(length)` | Never smaller than its content: text, buttons, toggles, menu items |
| `Extent::shrinkable(preferred, minimum)` | Can give up space down to `minimum`: slider tracks, separators, generic boxes |
| `Extent::rigid(length)` | Exactly this length |

When a container overflows, space is taken back only from items with slack
above their minimum. A `ScrollView` reports a zero minimum along its scroll
axis, so a long list scrolls instead of squashing the controls around it.

## Algorithms

**Measure** (bottom-up, cached per axis and cross length): a leaf reports
`measureContent`; a container asks its `Layout`; the widget's `AxisSizing`
then resolves the result. The cache is invalidated up the ancestor chain by
`requestLayout`.

**Distribute** (`distributeSpace`): start each item at its preferred length,
then repeatedly give out free space by `grow` weight (or take it back
proportionally to each item's slack above its minimum), clamping to
`[minimum, maximum]`. Free space is recomputed from scratch every round, so
the loop converges in at most `n + 1` rounds without epsilon tricks.

**Snap** (`placeSnapped`): edges are rounded, not lengths, so adjacent
children always tile without gaps.

**Arrange** (top-down): a parent assigns each child a rect. A child whose
rect is unchanged and which has no pending layout is skipped entirely.

**Popup placement** (`placePopup`): place on the requested side, flip to the
opposite side if it doesn't fit, then slide inside the viewport.

**Hover**: diff the old and new root-to-target paths and notify only the
widgets that entered or left.

## Layouts

| Layout | Use |
|---|---|
| `RowLayout` / `ColumnLayout` (`BoxLayout`) | Flex-like stacking with gap, padding, main/cross alignment |
| `StackLayout` | Every child fills the whole area (pages, overlays) |
| `FreeLayout` | Children at `requestPosition()` with their preferred size (floating panels, desks) |
| `GridLayout` | Equal columns, rows as tall as their tallest cell |

`SplitView` and `ScrollView` own private layouts with typed slots instead of
"child number N means X".

## Docking

`DockArea` is a CLion-style tool-window frame around a center widget, built
from `DockPanel`s:

- **Stripes.** A thin rail on the left and right lists every panel. Left and
  bottom panels share the left rail (bottom panels at its lower end); right
  panels use the right rail. Clicking a stripe button toggles its panel.
- **One panel per side.** Opening a panel closes the other one docked on the
  same side, as in CLion.
- **Header.** Title, a "⋮" options button and a "−" hide button, as in
  CLion. The options menu has *View Mode* (Dock Pinned, Dock Unpinned,
  Float), *Move To* (Left, Right, Bottom) and *Hide*.
- **View modes:**
  - *Docked / pinned*: the panel takes a zone and pushes the center aside.
  - *Undocked / unpinned*: the panel overlays the center and hides as soon as
    the user activates something outside it (pressing the center, another
    panel, a toolbar). Its stripe button can still toggle it.
  - *Float*: a movable window inside the dock area, resizable from its
    corner grip, independent of side zones.
- **Resize.** Drag a docked panel's inner edge; length is clamped so the
  center always stays visible.
- **Drag to dock.** Dragging a header beyond a small threshold detaches the
  panel, which follows the pointer as a raised window. While any panel is
  dragged, docked or floating, a hot target square is shown for every side.
  Moving the pointer onto one grows an animated preview of the zone the
  panel will take; releasing there docks it on that side and moves its
  stripe button. Releasing anywhere else floats the panel at the drop
  position; nothing docks without a hot target.

"Activates something outside" comes from `Ui::activeWidget()`, the widget
last pressed. The active path follows popup owners, so using a panel's
options menu counts as activity inside the panel. Every ancestor reports `containsActiveWidget()` and gets
`onActiveWithinChanged()`; that is how unpinned panels auto-hide without
polling.

## Kanban

`KanbanBoard` shows the items of a `KanbanModel` in columns and lets the user
drag them between and within columns.

- **Columns** are configured in code with `setColumns({ KanbanColumn... })`:
  title, accent, optional card limit (WIP), whether drops are accepted, and
  ordering. `Manual` columns place a dropped card where the placeholder opens;
  `ModelDefined` columns highlight as a whole and leave the order to the model.
- **Data** comes from the model: `items(column)` lists ids in display order and
  `canMove(move)` may veto a move. The board never edits the model; it reports
  `onMoved(KanbanMove)` and re-reads the model, so the model is the single source
  of truth. Call `modelChanged()` after external edits; cards are kept per id and
  animate when they change column.
- **Cards** are `KanbanCard` widgets (title, detail, accent strip, badge) made by
  an optional factory and refreshed by a card updater, as in `TreeView`.
  `setCardHeight(h)` gives every card the same height: the title keeps one line
  and the detail as many lines as fit, ending in an ellipsis
  (`Label::setMaximumLines`). Without it cards grow with their text.
- **Drag** starts after a small pointer movement (a long press with touch). The
  card is lifted above the board and follows the pointer, edges auto-scroll, a
  refused target is outlined in red, Esc cancels. Click selects, double-click
  activates.
- **Column visibility**: every column header has an options ("...") menu with a
  toggle per column, so any column can be shown or hidden from any other one.
  `KanbanColumn::visible` sets the initial state, `setColumnVisible` changes it and
  `setOnColumnVisibilityChanged` reports it so the host can persist it. The last
  visible column cannot be hidden. Hidden columns take no space and receive no drops.
- **Width**: visible columns share the width equally, never narrower than the
  minimum of `setColumnWidthRange`; when they don't fit, the strip scrolls
  horizontally (scroll bar, trackpad swipe or horizontal wheel).

## Tab views

`TabView` shows one page at a time under a row of tab buttons. The style is
fixed at construction:

- `TabView::Style::Tabs` (default) switches pages instantly.
- `TabView::Style::Carousel` adds Previous/Next arrow buttons around the tabs and
  slides pages horizontally: the current page leaves to one side while the next
  comes in from the other. Navigation is cyclic, so Next on the last page shows
  the first. A horizontal wheel (or Shift+wheel) switches pages only when nothing
  inside the page accepted it, because wheel events bubble from the hovered
  widget up. Mouse notches switch at once; trackpad deltas pull the pages with
  the same magnetic step as `TreeView` (see below).

## Magnetic step

`MagneticStep` (in `ScrollPhysics.hpp`, next to `FlingSpline` and
`StretchEffect`) is the one implementation of a discrete step reached by wheel
or pinch input. Widgets feed it input in step units with `push()` and advance it
every tick; it owns the accumulated pull, the resistance curve
`give * (1 - exp(-|pull| / give))`, the commit threshold, the release countdown
and a critically damped spring without overshoot. The visible `offset()` moves
from the first delta. Crossing `commitFraction` commits a step and starts a new
pull, so one gesture can pass several steps; `moveBy()` steps directly (mouse
notches, buttons). Tuning comes from one `MagneticStepTuning` that the step
reads live; `MagneticStepTuning::standard()` is the shared default and
`TreeViewStyle::step` lets a tree view adjust its own.

Users: `TreeView` level pull, camera rest between compact and detailed cards,
and the pull into the full view; `TabView::Style::Carousel` pages.

Each carousel page sits in its own slot with geometry animation off; the carousel
layout places the current slot and, while sliding, its neighbour, and the page
area clips them.

## Application storage

Every application keeps its own data in one folder, `~/<application>/`. A
`WidgetApplication` subclass names it by implementing
`persistentAppStorage()` (part of `WindowHost`), for example
`AppStorage::inHome("taskboard")`. Widgets reach it through
`ui()->windowHost()->persistentAppStorage()`:

- `images/` holds images pasted into a `MarkdownEditor`. Without an explicit
  `setImageStore`, a pasted image is saved there and linked by absolute path.
- `sessions/` is the default folder for session recordings
  (`$MODULES_SESSION_DIR` overrides it).
- `hangs/` holds hang reports (`$MODULES_HANG_DIR` overrides it).
- Other files are the application's own, such as taskboard's
  `session-presets.json` or sketch2d's `sketches/` library.

Tests and benchmarks that create a whole application give it a temporary
storage folder (for example `Sketch2DOptions::forTesting`) so they never
write into the user's real data.

## Popups

One mechanism serves menus, submenus, dropdowns, popup buttons and tooltips:

```cpp
ui::PopupRequest request;
request.content = &menu;           // any widget; attached while open
request.owner = &button;           // pressing the owner doesn't dismiss; owner removal closes it
request.anchor = button.windowRect();
request.options.placement = ui::Placement::Below;
ui->openPopup(request);
```

The stack tracks chains: a popup opened from inside another popup is its
child. Opening from the main tree replaces unrelated popups. Pressing outside
closes popups above the pressed one. Escape closes the topmost. `modal`
blocks input below. `interactive = false` (tooltips) makes it ignore the
pointer.

## Input

- Pointer events go to the deepest visible widget under the pointer (popups
  first), then bubble to ancestors until one calls `accept()`.
- A widget that calls `ui()->capturePointer(*this)` receives moves and the
  release even outside its bounds. Only the capturing widget can release.
- `Click` is synthesized when press and release land on the same widget.
- Keyboard events go to the focused widget (set when a focusable widget is
  pressed), else the topmost popup, else the root.
- Disabled widgets receive nothing; their ancestors still do.
- `PointerDown` and `Click` carry `clickCount()`: presses of the same button
  within 400 ms and 4 px count up, so double-click is `clickCount() == 2`.
  The clock comes from `Ui::advance`, which keeps it deterministic in tests.
- `setCursor(shape)` gives a widget a pointer cursor; the hovered widget or its
  nearest ancestor with a cursor wins and is sent to the window host.

## Window

The window has no system decorations. `Ui::setWindowHost` connects the tree to
the window (`WidgetApplication` is the host in apps; tests use
`FakeWindowHost`). Without a host the window controls stay hidden.

- `MenuBar` ends with `WindowControls`: monochrome minimize, maximize/restore
  and close buttons (close hovers red, the one non-gray color).
- Pressing empty menu-bar space starts a compositor window move, double-click
  toggles maximize, and secondary press opens the system window menu.
- The frame itself (rounded corners, outline, shadow, edge resizing) belongs to
  `Graphics`: `WindowFrame` owns the geometry and edge hit-testing, the Wayland
  backend reserves a shadow margin, sets the window geometry and input region,
  and the canvas cuts the corners and paints the shadow last. Maximized and
  tiled windows drop the margin and corners. Without compositor alpha support
  the frame falls back to an opaque square window that still resizes from its
  edges.

## Animation

Geometry is animated by default, as in the original library. Every widget
has two rects:

- `targetRect()`: where layout wants the widget.
- `rect()`: where the widget is now. Painting, hit testing, `windowRect()`
  and the arrangement of its children all use this one.

When layout assigns a new target, a spring (`Motion::fluid()`, the same
stiffness 0.0041 and damping 0.25 per millisecond as the original
`SpringRect`) moves `rect()` towards it. Each step re-arranges the widget's
children inside the current rect, so a panel that is opening pushes the
main view and lays out its own content as it grows; nothing is faked at
paint time. Children of a moving widget follow it directly instead of
running their own springs, so motion never compounds. The curve has no
overshoot: 90% of the travel takes about 135 ms, and it settles to within
a quarter pixel in about 300 ms.

Appearing is animated too:

- Popups grow out of the edge of their anchor.
- Dock panels slide out of their side.
- A widget that becomes visible grows from its center.

`setGeometryAnimated(false)` opts a widget out; scroll view content and
dock panels being resized by the pointer use it so they track input
exactly. `finishGeometryAnimation()` jumps to the target.

`Math/SpringAnimations.hpp` steps springs in substeps of at most 1 ms with
an explicit delta, so the curve is the same at any frame rate and tests are
deterministic. Controls animate hover, press and toggle visuals with
`AnimatedFloat` / `AnimatedColor` and the `Motion::quick()` /
`Motion::smooth()` presets.

## Text styles

`Painter::styledText` / `TextMetrics::styledTextWidth` take a `TextStyle`
(`fontSize` + `FontFace`: Regular, Bold, Italic, BoldItalic, Monospace).
`text()` / `textWidth()` are Regular shorthands. `Canvas` loads
`rsc/FontBold.ttf` (Cantarell Bold) and `rsc/FontMono.ttf` (JetBrains Mono),
both OFL (`Graphics/rsc/licenses`); italic is a synthetic skew. A missing
face falls back to the regular font.

## Markdown

Parsing is md4c (`Externals/md4c`, MIT), linked privately into `Widgets`.

| Piece | Responsibility |
|---|---|
| `markdown::MarkdownDocument` | md4c callbacks → `Block` tree (kind, children, styled `TextRun`s, byte `SourceRange` into the source), outline, task toggling |
| `RichText` | One paragraph of `TextSpan`s: per-span face, colour, background chip, link, strike/underline; word wrap, alignment, link hover/click, click → source offset |
| `MarkdownView` | Builds widgets from the document (structure ported from lv_markdown, tables/links/images from imgui_md): headings, lists, task checkboxes, quotes, admonitions, code, tables, rules |
| `MarkdownImage` | An image paragraph: loaded through `Ui::imageLoader()`, shrinks to the column width, alt-text placeholder when missing |
| `MarkdownEditor` | CLion-like Editor / Split / Preview: monospace `TextArea` with markdown highlighting, debounced re-render, scroll-synced panes, Ctrl+wheel zoom of both panes, clicking preview text moves the caret, task checkboxes rewrite the source |

Rebuilding happens on `setMarkdown`; widgets replaced while one of them is
handling an event are retired and destroyed on the next tick. Task toggles
are applied on the next tick for the same reason.

Scroll sync: whichever pane scrolls first leads until the next tick; the
other jumps to the block or source line at the leader's top edge.
`ScrollView::setOnScrolled` reports every applied offset change.

Images: `ImageLoader` is an optional `Ui` service (like `TextMetrics`).
`WidgetsApp` installs `CanvasImageLoader`, which caches nanovg images by
path. Relative image paths resolve against `MarkdownView::setBaseDirectory`.

Pasting images: `WindowHost::clipboardImage()` returns the clipboard's
image bytes and MIME type (Wayland: PNG preferred, any `image/*` otherwise;
Android: none yet). Ctrl+V in the editor asks `MarkdownEditor`'s
`ImageStore` callback to save the bytes and return a path, then inserts
`![](path)` as its own paragraph. Without an image on the clipboard, Ctrl+V
pastes text as before. The gallery stores pasted images in `images/` next
to the open file (or in `rsc` for the sample) and copies them along on
Save as.

Opening links: clicking a link in the preview, or Ctrl+clicking it in the
source, calls `MarkdownEditor::openLink`. An optional `LinkHandler` may
claim the link first. `#anchor` links scroll to the heading with that
GitHub-style anchor (`markdown::headingAnchor`, duplicates get `-1`, `-2`).
Everything else goes to `WindowHost::openUrl`: links with a scheme as they
are, relative paths resolved against the base directory into absolute file
paths. Wayland spawns `xdg-open` with an `xdg-activation` token so the
compositor lets the opened app take focus; Android starts an
`ACTION_VIEW` intent.

## Debugging

- `ui.dumpTree()` prints every widget's name, rect and state.
- `ui.stats()` counts layout passes and measured, arranged, skipped,
  delivered and painted widgets.
- `WidgetApplication` toggles the widget debugger with D. It takes its own column on the right; the Ui viewport shrinks to the area left of it, and pointer input over the panel goes to ImGui only, so the panel never covers or steals input from the UI.
- `setDebugName` names widgets in dumps and the inspector.

### Debug menu and session recording

Every `WidgetApplication` owns `DebugTools` and attaches a **Debug** menu
to its menu bar by default: *Debug UI* (the inspector), *Record session*
and *Stop recording and save*. It is a trailing menu, so it stays last
when the app adds its own menus later. Apps with their own menu bar call
`debugTools().attachTo(theirBar)` (and `detach()` before the bar is
destroyed); `setDebugToolsEnabled(false)` removes it. `DebugTools::addMenuItems`
adds the same entries to any other menu.

Recording (`SessionRecorder`, GraphicsInput) taps every raw window event
before coalescing and turns the profiler on. While it runs, a `● REC m:ss`
badge (`SessionRecordingControl::badge`) shows next to the window
controls; clicking it stops. Stopping asks where to save, defaulting to
`~/<application>/sessions/` (or `$MODULES_SESSION_DIR`); cancelling asks
before discarding. A session folder holds:

| file | contents |
|---|---|
| `environment.txt` | application, revision, GPU, date, window size and scale |
| `input.txt` | every raw input event with its time (`InputRecording`) |
| `recorded-frames.tsv` | per frame wall and CPU time, profiled sections, GPU times, counters |

Apps add their own state with `SessionRecorder::setSnapshot`: it is
captured when recording starts and written into the same folder.
Sketch2D uses it for `session.txt` and layer tiles, which its bench
replays.

## Testing

`tests/support` provides everything needed to test any widget tree headlessly:

- `UiDriver` owns a `Ui` and drives it like a user would: `moveTo`, `press`,
  `release`, `click`, `drag`, `wheel`, `key`, `advance`, `settle`,
  `hoverFor`, `paint`.
- `RecordingPainter` records paint commands with their window rects and
  clips, so tests can assert what text is visible and that clip/translation
  stacks are balanced.
- `MonospaceTextMetrics` makes text measurement deterministic.
- `Box` / `EventProbe` are minimal widgets for exact layout and event checks.

Suites: algorithms (sizing, distribution, snapping, placement, springs),
layouts, input routing, popups and tooltips, every widget, menus, window
chrome, and the gallery application end to end. `Sketch2D/tests` reuses the same support code to
test the Sketch2D UI with a fake renderer.

## What changed from the old library

| Old | New |
|---|---|
| Four copies of a rect (`mArea` spring target/current, `mAreaCache`, layout area) copied back and forth each frame | A target rect from layout and a sprung current rect that children are laid out in |
| Size policy and min/max stored on the child's own layout object; every leaf allocated a layout | `AxisSizing` on the widget; only containers have a layout |
| Child picks its own rect from parent, children and itself; dependency graph, topological sort, dead policy table | Measure up, arrange down; no graph |
| `UpdateManager` mixed hover, event delivery, layout scoping, animation and redraw; `process()` polled every frame | Separate `InputRouter`, `TickScheduler`, layout dirty flags, paint request; widgets only run on events, ticks and paint |
| Popups re-parented under a shared layer plus a focus lock to fake modality | `PopupStack` with owners, chains and dismissal rules |
| Four friend managers and many `dynamic_cast`s | A narrow public API; one internal access class for the engine |
| Layouts keyed on child count (`ToolBarLayout`, `FixedSizeBarsLayout`, `ScrollableLayout`) | Generic `Row`/`Column`/`Stack`/`Free`/`Grid` plus typed slots |
| `std::to_string` debug reasons built on every `setArea` | No hot-path strings; `dumpTree` and stats on demand |
