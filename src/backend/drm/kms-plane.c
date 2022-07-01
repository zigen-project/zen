#include "kms-plane.h"

#include <zen-util.h>

struct zn_kms_plane *
zn_kms_plane_create(drmModePlane *drm_plane, int idx)
{
  struct zn_kms_plane *self;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  self->id = drm_plane->plane_id;
  self->idx = idx;

  return self;

err:
  return NULL;
}

void
zn_kms_plane_destroy(struct zn_kms_plane *self)
{
  free(self);
}
