_zen_desktop_srcs = [
  'display-system.c',
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
  'scene/view.c',

  protocols['xdg-shell']['private-code'],
  protocols['xdg-shell']['server-header'],
  protocols['zen-desktop']['private-code'],
  protocols['zen-desktop']['server-header'],
]

_zen_desktop_deps = [
  pixman_dep,
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
