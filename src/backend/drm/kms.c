#include "kms.h"

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <zen-util.h>

static int
zn_kms_init_caps(struct zn_kms *self)
{
  uint64_t cap;
  int ret;

  ret = drmGetCap(self->drm_fd, DRM_CAP_TIMESTAMP_MONOTONIC, &cap);
  self->caps.timestamp_monotonic = ((ret == 0) && (cap == 1));

  if (!self->caps.timestamp_monotonic) {
    zn_log(
        "DRM Error: kernel DRM KMS does not support "
        "DRM_CAP_TIMESTAMP_MONOTONIC\n");
    return -1;
  }

  // TODO: set CLOCK_MONOTONIC for presentation clock

  ret = drmGetCap(self->drm_fd, DRM_CAP_CURSOR_WIDTH, &cap);
  if (ret == 0)
    self->caps.cursor_width = cap;
  else
    self->caps.cursor_width = 64;

  ret = drmGetCap(self->drm_fd, DRM_CAP_CURSOR_HEIGHT, &cap);
  if (ret == 0)
    self->caps.cursor_height = cap;
  else
    self->caps.cursor_height = 64;

  ret = drmSetClientCap(self->drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  if (ret != 0) {
    zn_log("DRM: failed to enable DRM_CLIENT_CAP_UNIVERSAL_PLANES\n");
    return -1;
  }

  ret = drmGetCap(self->drm_fd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap);
  if (ret != 0) cap = 0;
  ret = drmSetClientCap(self->drm_fd, DRM_CLIENT_CAP_ATOMIC, 1);
  self->caps.atomic_modeset = ((ret == 0) && (cap == 1));

  zn_log("DRM: %s atomic modesetting\n",
      self->caps.atomic_modeset ? "supports" : "does not support");

  return 0;
}

struct zn_kms *
zn_kms_create(int drm_fd)
{
  struct zn_kms *self;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  self->drm_fd = drm_fd;

  if (zn_kms_init_caps(self) != 0) goto err_free;

  return self;

err_free:
  free(self);

err:
  return NULL;
}

void
zn_kms_destroy(struct zn_kms *self)
{
  free(self);
}
