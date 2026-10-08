# Graphics

Native Wayland window, Vulkan presentation, nanovg canvas and ImGui debug UI.
Wayland is the only supported platform.

## Window backend

- `WaylandConnection` binds the globals: `xdg_wm_base`, `wl_seat`,
  `wl_data_device_manager`, `wp_viewporter`, `wp_fractional_scale_v1`,
  `wp_cursor_shape_v1`, `zwp_text_input_v3` and `xdg-decoration`.
- `WaylandWindow` owns the surface and toplevel, applies configures (size,
  maximized, tiled, activated), fractional scale through a viewport, the
  window geometry and input region, and starts compositor move/resize.
- `WaylandSeat` handles pointer (cursor shapes, wheel), keyboard (xkbcommon
  keymap, compose, key repeat through a timerfd in the same poll), clipboard
  (`text/plain;charset=utf-8`) and IME commits from `text-input-v3`.
- `WindowFrame` is the pure geometry of the client-side frame: shadow margin,
  content rect, input region and edge hit-testing. It is unit-tested.
- `WindowInputDispatcher` turns backend input into `EventHandler` events and
  forwards it to observers such as `ImGuiPlatform`, which feeds ImGui IO.

## Frame

The surface is the content plus a transparent shadow margin. Each frame draws
the UI in content coordinates, then ImGui, then a separate nanovg pass that
cuts the rounded corners, strokes the outline and paints the shadow behind the
content. The swapchain uses premultiplied alpha when the surface supports it.

## Tests

`TestGraphics` covers frame geometry and key mapping without a display.
