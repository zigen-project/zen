project(
  'zen', 'c', 'cpp',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>= 0.52.1',
  default_options:  [ 'warning_level=3', 'werror=true', 'optimization=2', 'c_std=gnu11' ],
)

if get_option('debug')
  add_global_arguments('-DDEBUG', language: [ 'c', 'cpp' ])
endif

public_inc = include_directories('include')

# dependencies
dep_wayland_server = dependency('wayland-server')
dep_wayland_client = dependency('wayland-client')
dep_wayland_scanner = dependency('wayland-scanner', version: '>= 1.14.91')
dep_glfw3 = dependency('glfw3')
dep_glew = dependency('glew')
dep_zgn_protocol = dependency('zigen-protocols')
dep_cglm = dependency('cglm')
dep_libinput = dependency('libinput')
dep_udev = dependency('libudev')
deb_xkbcommon = dependency('xkbcommon')
dep_openvr = dependency('openvr')
dep_m = meson.get_compiler('c').find_library('m')

opt_backend = get_option('zen_backend')
opt_client = get_option('clients')

subdir('protocol')
subdir('libzen-compositor')
subdir('zen-shell')
subdir('zen-renderer')
subdir('zen-backend')
subdir('zen')
subdir('tests')

if opt_client
  dep_glm = dependency('glm')
  dep_curl = dependency('libcurl')
  subdir('clients')
endif
