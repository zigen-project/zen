#ifndef ZEN_DRM_BACKEND_KMS_H
#define ZEN_DRM_BACKEND_KMS_H

#include <stdbool.h>
#include <stdint.h>

/* zn_drm_backend owns zn_kms */
struct zn_kms {
  int drm_fd;  // managed by zn_drm_backend
  struct {
    int32_t cursor_width;
    int32_t cursor_height;
    bool timestamp_monotonic;
    bool atomic_modeset;
  } caps;
};

struct zn_kms *zn_kms_create(int drm_fd);

void zn_kms_destroy(struct zn_kms *self);

#endif  //  ZEN_DRM_BACKEND_KMS_H
