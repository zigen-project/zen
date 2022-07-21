#include "zen/scene/scene.h"

#include "zen-common.h"

struct zn_scene*
zn_scene_create()
{
  struct zn_scene* self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->screen_layout = zn_screen_layout_create();
  if (self->screen_layout == NULL) {
    zn_error("Failed to create zn_screen_layout");
    goto err_free;
  }

  return self;

err_free:
  free(self);

err:
  return NULL;
}

void
zn_scene_destroy(struct zn_scene* self)
{
  zn_screen_layout_destroy(self->screen_layout);

  free(self);
}
