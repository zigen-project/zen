#include "zen/scene/screen-layout.h"

#include "zen-common.h"

struct zn_screen_layout *
zn_screen_layout_create(void)
{
  struct zn_screen_layout *self;
  self = zalloc(sizeof *self);

  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  wl_list_init(&self->screens);

  return self;

err:
  return NULL;
}

void
zn_screen_layout_destroy(struct zn_screen_layout *self)
{
  wl_list_remove(&self->screens);
  free(self);
}
