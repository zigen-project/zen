#include <assert.h>
#include <wayland-server.h>

#include "compositor.h"
#include "test-runner.h"

TEST(load_backend)
{
  struct wl_display *display;
  struct zen_compositor *compositor;

  display = wl_display_create();
  compositor = zen_compositor_create(display);
  assert(compositor->backend == NULL);

  zen_compositor_load_backend(compositor);
  assert(compositor->backend != NULL);
}