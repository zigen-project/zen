#include "zen/server.h"

#include <stdio.h>
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/xwayland.h>
#include <zen-desktop-protocol.h>

#include "zen-common/log.h"
#include "zen-common/util.h"
#include "zen/display-system.h"
#include "zen/input-manager.h"
#include "zen/output.h"
#include "zen/scene/scene.h"
#include "zen/xdg-toplevel-view.h"
#include "zen/xwayland-view.h"

struct zn_server {
  struct wl_display *display;
  struct wl_event_loop *loop;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;
  struct wlr_xwayland *xwayland;

  // these objects will be automatically destroyed when wl_display is destroyed
  struct wlr_compositor *w_compositor;
  struct wlr_xdg_shell *xdg_shell;

  struct zn_display_system *display_system;
  struct zn_input_manager *input_manager;

  struct zn_scene *scene;

  char *socket;

  struct wl_listener new_input_listener;
  struct wl_listener new_output_listener;
  struct wl_listener xdg_shell_new_surface_listener;
  struct wl_listener display_system_switch_listener;
  struct wl_listener xwayland_new_surface_listener;

  int exit_code;
};

static void
zn_server_new_input_handler(struct wl_listener *listener, void *data)
{
  struct zn_server *self = zn_container_of(listener, self, new_input_listener);
  struct wlr_input_device *wlr_input = data;
  zn_input_manager_handle_new_wlr_input(self->input_manager, wlr_input);
}

static void
zn_server_new_output_handler(struct wl_listener *listener, void *data)
{
  struct zn_server *self = zn_container_of(listener, self, new_output_listener);
  struct wlr_output *wlr_output = data;
  struct zn_output *output;

  zn_debug("New output %p: %s (non-desktop: %d)", (void *)wlr_output,
      wlr_output->name, wlr_output->non_desktop);

  if (wlr_output->non_desktop) {
    zn_debug("Not configuring non-desktop output");
    // TODO: DRM lease support for VR headsets
    return;
  }

  if (wlr_output_init_render(wlr_output, self->allocator, self->renderer) ==
      false) {
    zn_abort("Failed to initialize output renderer");
    return;
  }

  output = zn_output_create(wlr_output, self);
  if (output == NULL) {
    zn_warn("Failed to create a zen output");
    return;
  }
  UNUSED(output);
}

static void
zn_server_xdg_shell_new_surface_handler(
    struct wl_listener *listener, void *data)
{
  struct wlr_xdg_surface *xdg_surface = data;
  struct zn_server *self =
      zn_container_of(listener, self, xdg_shell_new_surface_listener);

  if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
    zn_debug("New xdg_shell popup");
    return;
  }

  zn_debug("New xdg_shell toplevel title='%s' app_id='%s'",
      xdg_surface->toplevel->title, xdg_surface->toplevel->app_id);
  wlr_xdg_surface_ping(xdg_surface);

  (void)zn_xdg_toplevel_view_create(xdg_surface->toplevel, self);
}

static void
zn_server_display_system_switch_handler(
    struct wl_listener *listener, void *data)
{
  struct zn_server *self =
      zn_container_of(listener, self, display_system_switch_listener);
  struct zn_display_system_switch_event *event = data;

  // TODO: implement here

  zn_display_system_send_applied(self->display_system, event->type);
}

static void
zn_server_xwayland_new_surface_handler(struct wl_listener *listener, void *data)
{
  struct wlr_xwayland_surface *xwayland_surface = data;
  struct zn_server *self =
      zn_container_of(listener, self, xwayland_new_surface_listener);

  zn_debug("New xwayland surface title='%s' class='%s'",
      xwayland_surface->title, xwayland_surface->class);
  wlr_xwayland_surface_ping(xwayland_surface);

  (void)zn_xwayland_view_create(xwayland_surface, self);
}

struct wl_event_loop *
zn_server_get_loop(struct zn_server *self)
{
  return self->loop;
}

struct wlr_renderer *
zn_server_get_renderer(struct zn_server *self)
{
  return self->renderer;
}

struct zn_scene *
zn_server_get_scene(struct zn_server *self)
{
  return self->scene;
}

int
zn_server_run(struct zn_server *self)
{
  self->xwayland = wlr_xwayland_create(self->display, self->w_compositor, true);
  if (self->xwayland == NULL) {
    zn_error("Failed to create xwayland");
    return EXIT_FAILURE;
  }

  self->xwayland_new_surface_listener.notify =
      zn_server_xwayland_new_surface_handler;
  wl_signal_add(&self->xwayland->events.new_surface,
      &self->xwayland_new_surface_listener);

  setenv("DISPLAY", self->xwayland->display_name, true);
  zn_debug("DISPLAY=%s", self->xwayland->display_name);

  if (!wlr_backend_start(self->backend)) {
    zn_error("Failed to start backend");
    return EXIT_FAILURE;
  }

  wl_display_run(self->display);
  return self->exit_code;
}

void
zn_server_terminate(struct zn_server *self, int exit_code)
{
  self->exit_code = exit_code;
  wl_display_terminate(self->display);
}

struct zn_server *
zn_server_create(struct wl_display *display)
{
  struct zn_server *self;
  char socket_name_candidate[16];
  char *xdg;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->display = display;
  self->exit_code = EXIT_FAILURE;
  self->loop = wl_display_get_event_loop(display);

  self->backend = wlr_backend_autocreate(self->display);
  if (self->backend == NULL) {
    zn_error("Failed to create a backend");
    goto err_free;
  }

  self->renderer = wlr_renderer_autocreate(self->backend);
  if (self->renderer == NULL) {
    zn_error("Failed to create renderer");
    goto err_backend;
  }

  if (wlr_renderer_init_wl_display(self->renderer, self->display) == false) {
    zn_error("Failed to initialize renderer with wl_display");
    goto err_renderer;
  }

  self->allocator = wlr_allocator_autocreate(self->backend, self->renderer);
  if (self->allocator == NULL) {
    zn_error("Failed to create allocator");
    goto err_renderer;
  }

  self->w_compositor = wlr_compositor_create(self->display, self->renderer);
  if (self->w_compositor == NULL) {
    zn_error("Failed to create wlr_compositor");
    goto err_allocator;
  }

  self->xdg_shell = wlr_xdg_shell_create(self->display);
  if (self->display == NULL) {
    zn_error("Failed to create wlr_xdg_shell");
    goto err_allocator;
  }

  self->scene = zn_scene_create();
  if (self->scene == NULL) {
    zn_error("Failed to create zn_scene");
    goto err_allocator;
  }

  self->display_system = zn_display_system_create(self->display);
  if (self->display_system == NULL) {
    zn_error("Failed to create display system");
    goto err_scene;
  }

  for (int i = 1; i <= 32; i++) {
    sprintf(socket_name_candidate, "wayland-%d", i);
    if (wl_display_add_socket(self->display, socket_name_candidate) >= 0) {
      self->socket = strdup(socket_name_candidate);
      break;
    }
  }

  if (self->socket == NULL) {
    zn_error("Failed to open wayland socket");
    goto err_display_system;
  }

  self->input_manager = zn_input_manager_create(self->display);
  if (self->input_manager == NULL) {
    zn_error("Failed to create input manager");
    goto err_socket;
  }

  setenv("WAYLAND_DISPLAY", self->socket, true);
  xdg = getenv("XDG_RUNTIME_DIR");
  zn_debug("WAYLAND_DISPLAY=%s", self->socket);
  zn_debug("XDG_RUNTIME_DIR=%s", xdg);

  self->new_input_listener.notify = zn_server_new_input_handler;
  wl_signal_add(&self->backend->events.new_input, &self->new_input_listener);

  self->new_output_listener.notify = zn_server_new_output_handler;
  wl_signal_add(&self->backend->events.new_output, &self->new_output_listener);

  self->xdg_shell_new_surface_listener.notify =
      zn_server_xdg_shell_new_surface_handler;
  wl_signal_add(&self->xdg_shell->events.new_surface,
      &self->xdg_shell_new_surface_listener);

  self->display_system_switch_listener.notify =
      zn_server_display_system_switch_handler;
  wl_signal_add(&self->display_system->switch_signal,
      &self->display_system_switch_listener);

  return self;

err_socket:
  free(self->socket);

err_display_system:
  zn_display_system_destroy(self->display_system);

err_scene:
  zn_scene_destroy(self->scene);

err_allocator:
  wlr_allocator_destroy(self->allocator);

err_renderer:
  wlr_renderer_destroy(self->renderer);

err_backend:
  wlr_backend_destroy(self->backend);

err_free:
  free(self);

err:
  return NULL;
}

void
zn_server_destroy(struct zn_server *self)
{
  if (self->xwayland) wlr_xwayland_destroy(self->xwayland);
  zn_input_manager_destroy(self->input_manager);
  zn_display_system_destroy(self->display_system);
  free(self->socket);
  zn_scene_destroy(self->scene);
  wlr_allocator_destroy(self->allocator);
  wlr_renderer_destroy(self->renderer);
  wlr_backend_destroy(self->backend);
  free(self);
}
