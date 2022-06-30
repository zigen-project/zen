#include "kms-crtc.h"

#include <zen-util.h>

#include "drm-props.h"

static const struct zn_drm_property_info crtc_prop_info_list[] = {
    [ZN_KMS_CRTC_MODE_ID] = {.name = "MODE_ID"},
    [ZN_KMS_CRTC_ACTIVE] = {.name = "ACTIVE"},
};

struct zn_kms_crtc *
zn_kms_crtc_create(int drm_fd, uint32_t id, int idx)
{
  struct zn_kms_crtc *self;
  drmModeObjectProperties *props;
  int ret;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  props = drmModeObjectGetProperties(drm_fd, id, DRM_MODE_OBJECT_CRTC);
  if (props == NULL) {
    zn_log("DRM: failed to get CRTC properties\n");
    goto err_free;
  }

  ret = zn_drm_property_info_populate(drm_fd, crtc_prop_info_list,
      self->prop_info_list, ZN_KMS_CRTC__COUNT, props);
  if (ret != 0) {
    zn_log("DRM: failed to populate KMS crtc properties\n");
    goto err_props;
  }

  self->drm_fd = drm_fd;
  self->id = id;
  self->idx = idx;

  drmModeFreeObjectProperties(props);

  return self;

err_props:
  drmModeFreeObjectProperties(props);

err_free:
  free(self);

err:
  return NULL;
}

void
zn_kms_crtc_destroy(struct zn_kms_crtc *self)
{
  zn_drm_property_info_clear(self->prop_info_list, ZN_KMS_CRTC__COUNT);
  free(self);
}
