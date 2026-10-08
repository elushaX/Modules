# Sketch2D documentation

- `architecture/ARCHITECTURE.md` — how the app is put together.
- `roadmap/ROADMAP.md` — product roadmap.
- `roadmap/EngineRefactorPlan.md` — the step by step engine refactor.
- `research/LeonardoEngineAnalysis.md` — what the Leonardo engine does, from its binary.
- `research/BrushVsLeonardo.md` — brush differences to Leonardo and the fix plan.
- `research/leonardo/` — raw material used by the analysis.

## Source layout

`src/libs/<lib>/{public,private}`, each a static library, lowest first:

| Library | Target | Contents |
|---|---|---|
| common | Sketch2DCommon | code not tied to drawing: undo history, stroke smoothing, stroke curve, channel matrices |
| brush | Sketch2DBrush | brush parameters, presets, dab shape |
| engine | Sketch2DEngine | tiles, layers, level pyramid, background engine, selection, stroke painter |
| document | Sketch2DDocument | document model, compositor, colour model, layer thumbnail pixels, view settings |
| library | Sketch2DLibrary | sessions, recording, sketch library storage |
| ui | Sketch2DInterface | every widget: canvas widget, commands, studio tablet interface, library view, panels, update dialog |
| render | Sketch2D | Vulkan tile renderer, icons, viewport, GPU thumbnails |
| app | Sketch2DApplication | the application object |

Nothing below `ui` depends on widgets.

`src/apps` holds the desktop and Android entry points. Only the tablet interface is supported.

## Tests

| Run | Command | Time |
|---|---|---|
| quick: unit + quick integration (default) | `./TestSketch2D` in `build/Sketch2D` | ~15 s |
| GPU app integration | `ctest -R TestSketch2DApp` in `build` | ~10 s |
| long integration (human-like sessions) | `TEST_LEVEL=long ./TestSketch2D` | minutes |
| everything | `TEST_LEVEL=all ./TestSketch2D` | minutes |
| bench smoke in ctest | configure with `-DMODULES_BUILD_BENCH_TESTS=ON` | minutes |

Suites named `Long…` or `Bench…` only run when asked. `TEST_SUITE=<name>` runs one suite.

## Menus

`Sketch2DRoot::mainMenu` is the one menu structure (File, Edit, View, Canvas, Layer, Select, Line,
History, Brush, Color, Debug, Help). On PC it is shown as the window menu bar and the studio gear
button is hidden; on the tablet the gear opens the same menu as pages.
