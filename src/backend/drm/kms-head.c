#include "kms-head.h"

#include <zen-util.h>

struct zn_kms_head *
zn_kms_head_create(drmModeConnector *drm_connector)
{
  struct zn_kms_head *self;
  UNUSED(drm_connector);

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  // TODO: implement here

  return self;

err:
  return NULL;
}

void
zn_kms_head_destroy(struct zn_kms_head *self)
{
  free(self);
}
