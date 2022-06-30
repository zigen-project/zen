zen_drm_backend_srcs = [
  'backend.c',
  'kms.c',
]

zen_drm_backend_deps = [
  libdrm_dep,
  udev_dep,
  zen_backend_libinput_dep,
  zen_session_dep,
  zen_util_dep,
]

zen_drm_backend_lib = static_library(
  'zen-drm-backend',
  zen_drm_backend_srcs,
  include_directories: zen_backend_inc,
  dependencies: zen_drm_backend_deps,
  install: false,
)

zen_drm_backend_dep = declare_dependency(
  link_with: zen_drm_backend_lib,
  include_directories: zen_backend_inc,
)
