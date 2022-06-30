#ifndef ZEN_DRM_BACKEND_KMS_H
#define ZEN_DRM_BACKEND_KMS_H

#include <stdbool.h>
#include <stdint.h>

/* zn_drm_backend owns zn_kms */
struct zn_kms;

struct zn_kms *zn_kms_create(int drm_fd);

void zn_kms_destroy(struct zn_kms *self);

#endif  //  ZEN_DRM_BACKEND_KMS_H
