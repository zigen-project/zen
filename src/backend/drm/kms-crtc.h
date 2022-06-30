#ifndef ZEN_DRM_BACKEND_KMS_CRTC_H
#define ZEN_DRM_BACKEND_KMS_CRTC_H

#include <wayland-server.h>

#include "drm-props.h"

enum zn_kms_crtc_property {
  ZN_KMS_CRTC_MODE_ID = 0,
  ZN_KMS_CRTC_ACTIVE,
  ZN_KMS_CRTC__COUNT,
};

struct zn_kms_crtc {
  struct wl_list link;  // used by zn_kms

  int drm_fd;
  uint32_t id;
  int idx;

  struct zn_drm_property_info prop_info_list[ZN_KMS_CRTC__COUNT];
};

struct zn_kms_crtc *zn_kms_crtc_create(int drm_fd, uint32_t id, int idx);

void zn_kms_crtc_destroy(struct zn_kms_crtc *self);

#endif  //  ZEN_DRM_BACKEND_KMS_CRTC_H
