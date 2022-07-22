_zen_desktop_srcs = [
  'input-device.c',
  'input-manager.c',
  'main.c',
  'output.c',
  'render-2d.c',
  'seat.c',
  'server.c',
  'xdg-toplevel-view.c',
  'xwayland-view.c',

  'scene/scene.c',
  'scene/screen.c',
  'scene/screen-layout.c',
  'scene/view.c',
]

_zen_desktop_deps = [
  pixman_dep,
  server_protocols,
  wayland_server_dep,
  wlroots_dep,
  zen_common_dep,
]

executable(
  'zen-desktop',
  _zen_desktop_srcs,
  install: true,
  dependencies: _zen_desktop_deps,
  include_directories: zen_inc,
)
