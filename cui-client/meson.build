# CUI client to use the zen-desktop.xml protocol.
# Remove me when a corresponding GUI client is developed

_zen_desktop_cui_client_srsc = [
  'main.c',

  protocols['zen-desktop']['private-code'],
  protocols['zen-desktop']['client-header'],
]

_zen_desktop_cui_client_deps = [
  wayland_client_dep,
  zen_common_dep,
]

executable(
  'zen-desktop-cui-client',
  _zen_desktop_cui_client_srsc,
  install: false,
  dependencies: _zen_desktop_cui_client_deps,
)
