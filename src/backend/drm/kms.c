#include "kms.h"

#include <zen-util.h>

struct zn_kms *
zn_kms_create(int drm_fd)
{
  struct zn_kms *self;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  self->drm_fd = drm_fd;

  return self;

err:
  return NULL;
}

void
zn_kms_destroy(struct zn_kms *self)
{
  free(self);
}
