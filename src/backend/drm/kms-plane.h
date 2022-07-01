#ifndef ZEN_DRM_BACKEND_KMS_PLANE_H
#define ZEN_DRM_BACKEND_KMS_PLANE_H

#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct zn_kms_plane {
  struct wl_list link;  // used by zn_kms

  uint32_t id;
  int idx;
};

struct zn_kms_plane *zn_kms_plane_create(drmModePlane *drm_plane, int idx);

void zn_kms_plane_destroy(struct zn_kms_plane *self);

#endif  //  ZEN_DRM_BACKEND_KMS_PLANE_H
