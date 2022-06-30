#include "kms.h"

#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <zen-util.h>

#include "kms-crtc.h"

/* zn_drm_backend owns zn_kms */
struct zn_kms {
  int drm_fd;  // managed by zn_drm_backend

  struct wl_list crtc_list;
  struct wl_list plane_list;

  struct {
    int32_t cursor_width;
    int32_t cursor_height;
    bool timestamp_monotonic;
    bool atomic_modeset;
  } caps;
};

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

static int
zn_kms_init_crtc_list(struct zn_kms *self, drmModeRes *resources)
{
  struct zn_kms_crtc *crtc, *crtc_tmp;
  int i;

  wl_list_init(&self->crtc_list);

  for (i = 0; i < resources->count_crtcs; i++) {
    crtc = zn_kms_crtc_create();
    if (crtc == NULL) goto err;
    wl_list_insert(self->crtc_list.prev, &crtc->link);
  }

  return 0;

err:
  wl_list_for_each_safe(crtc, crtc_tmp, &self->crtc_list, link)
  {
    wl_list_remove(&crtc->link);
    zn_kms_crtc_destroy(crtc);
  }

  return -1;
}

static void
zn_kms_deinit_crtc_list(struct zn_kms *self)
{
  struct zn_kms_crtc *crtc, *crtc_tmp;

  wl_list_for_each_safe(crtc, crtc_tmp, &self->crtc_list, link)
  {
    wl_list_remove(&crtc->link);
    zn_kms_crtc_destroy(crtc);
  }
}

struct zn_kms *
zn_kms_create(int drm_fd)
{
  struct zn_kms *self;
  drmModeRes *resources;

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  self->drm_fd = drm_fd;

  if (zn_kms_init_caps(self) != 0) goto err_free;

  resources = drmModeGetResources(self->drm_fd);
  if (resources == NULL) goto err_free;

  if (zn_kms_init_crtc_list(self, resources) != 0) goto err_resource;

  drmModeFreeResources(resources);

  return self;

err_resource:
  drmModeFreeResources(resources);

err_free:
  free(self);

err:
  return NULL;
}

void
zn_kms_destroy(struct zn_kms *self)
{
  zn_kms_deinit_crtc_list(self);
  free(self);
}
