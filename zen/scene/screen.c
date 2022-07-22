#include "zen/scene/screen.h"

#include "zen-common.h"

struct zn_screen *
zn_screen_create(
    struct zn_screen_layout *screen_layout, struct zn_output *output)
{
  struct zn_screen *self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->output = output;
  wl_list_init(&self->views);
  wl_list_insert(&screen_layout->screens, &self->link);

  return self;

err:
  return NULL;
}

void
zn_screen_destroy(struct zn_screen *self)
{
  wl_list_remove(&self->views);
  wl_list_remove(&self->link);
  free(self);
}
