#ifndef ZEN_DRM_BACKEND_KMS_CRTC_H
#define ZEN_DRM_BACKEND_KMS_CRTC_H

#include <wayland-server.h>

struct zn_kms_crtc {
  struct wl_list link;  // used by zn_kms
};

struct zn_kms_crtc *zn_kms_crtc_create();

void zn_kms_crtc_destroy(struct zn_kms_crtc *self);

#endif  //  ZEN_DRM_BACKEND_KMS_CRTC_H
