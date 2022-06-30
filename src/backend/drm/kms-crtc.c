#include "kms-crtc.h"

#include <zen-util.h>

struct zn_kms_crtc *
zn_kms_crtc_create()
{
  struct zn_kms_crtc *self;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  return self;

err:
  return NULL;
}

void
zn_kms_crtc_destroy(struct zn_kms_crtc *self)
{
  free(self);
}
