#ifndef ZEN_KMS_HEAD_H
#define ZEN_KMS_HEAD_H

#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct zn_kms_head {
  struct wl_list link;  // used by zn_kms
};

struct zn_kms_head *zn_kms_head_create(drmModeConnector *drm_connector);

void zn_kms_head_destroy(struct zn_kms_head *self);

#endif  //  ZEN_KMS_HEAD_H
