#include "input-manager.h"

#include <wayland-server.h>

#include "seat.h"
#include "zen-common.h"

struct zn_input_manager {
  struct wl_list devices;  // zn_input_device::link
  struct zn_seat* seat;
};

struct zn_input_manager*
zn_input_manager_create(struct wl_display* display)
{
  struct zn_input_manager* self;
  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  wl_list_init(&self->devices);

  self->seat = zn_seat_create(display, ZEN_DEFAULT_SEAT);
  if (self->seat == NULL) {
    zn_error("Failed to create zn_seat");
    goto err_free;
  }

  return self;

err_free:
  free(self);

err:
  return NULL;
}

void
zn_input_manager_destroy(struct zn_input_manager* self)
{
  zn_seat_destroy(self->seat);
  wl_list_remove(&self->devices);
  free(self);
}
