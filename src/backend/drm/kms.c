#include "kms.h"

#include <errno.h>
#include <string.h>
#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <zen-util.h>

#include "kms-crtc.h"
#include "kms-head.h"
#include "kms-plane.h"

/* zn_drm_backend owns zn_kms */
struct zn_kms {
  int drm_fd;  // managed by zn_drm_backend

  struct wl_list crtc_list;   // list of zn_kms_crtc
  struct wl_list plane_list;  // list of zn_kms_plane
  struct wl_list head_list;   // list of zn_kms_head

  struct {
    int32_t cursor_width;
    int32_t cursor_height;
    bool timestamp_monotonic;
    bool atomic_modeset;
  } caps;

  uint32_t fb_min_width, fb_max_width;
  uint32_t fb_min_height, fb_max_height;
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

  wl_list_init(&self->crtc_list);

  for (int i = 0; i < resources->count_crtcs; i++) {
    crtc = zn_kms_crtc_create(self->drm_fd, resources->crtcs[i], i);
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

static int
zn_kms_init_plane_list(struct zn_kms *self)
{
  struct zn_kms_plane *plane, *plane_tmp;
  drmModePlaneRes *drm_plane_resources;
  drmModePlane *drm_plane;

  wl_list_init(&self->plane_list);

  drm_plane_resources = drmModeGetPlaneResources(self->drm_fd);
  if (!drm_plane_resources) {
    zn_log("DRM: failed to get plane resources; %s\n", strerror(errno));
    return -1;
  }

  for (uint32_t i = 0; i < drm_plane_resources->count_planes; i++) {
    drm_plane = drmModeGetPlane(self->drm_fd, drm_plane_resources->planes[i]);
    if (drm_plane == NULL) goto err;

    plane = zn_kms_plane_create(drm_plane, i);
    drmModeFreePlane(drm_plane);
    if (plane == NULL) goto err;

    wl_list_insert(&self->plane_list, &plane->link);
  }

  drmModeFreePlaneResources(drm_plane_resources);

  return 0;

err:
  wl_list_for_each_safe(plane, plane_tmp, &self->plane_list, link)
  {
    wl_list_remove(&plane->link);
    zn_kms_plane_destroy(plane);
  }

  return -1;
}

static void
zn_kms_deinit_plane_list(struct zn_kms *self)
{
  struct zn_kms_plane *plane, *plane_tmp;

  wl_list_for_each_safe(plane, plane_tmp, &self->plane_list, link)
  {
    wl_list_remove(&plane->link);
    zn_kms_plane_destroy(plane);
  }
}

static int
zn_kms_init_head_list(struct zn_kms *self, drmModeRes *resources)
{
  drmModeConnector *connector;
  struct zn_kms_head *head, *head_tmp;

  wl_list_init(&self->head_list);

  for (int i = 0; i < resources->count_connectors; i++) {
    connector = drmModeGetConnector(self->drm_fd, resources->connectors[i]);
    if (connector == NULL) goto err;

    if (connector->connector_type == DRM_MODE_CONNECTOR_WRITEBACK) {
      drmModeFreeConnector(connector);
      continue;
    }

    head = zn_kms_head_create(connector);
    drmModeFreeConnector(connector);
    if (head == NULL) goto err;

    wl_list_insert(&self->head_list, &head->link);
  }

  return 0;

err:
  wl_list_for_each_safe(head, head_tmp, &self->head_list, link)
  {
    wl_list_remove(&head->link);
    zn_kms_head_destroy(head);
  }
  return -1;
}

static void
zn_kms_deinit_head_list(struct zn_kms *self)
{
  struct zn_kms_head *head, *head_tmp;

  wl_list_for_each_safe(head, head_tmp, &self->head_list, link)
  {
    wl_list_remove(&head->link);
    zn_kms_head_destroy(head);
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

  self->fb_min_width = resources->min_width;
  self->fb_max_width = resources->max_width;
  self->fb_min_height = resources->min_height;
  self->fb_max_height = resources->max_height;

  if (zn_kms_init_crtc_list(self, resources) != 0) goto err_resource;

  if (zn_kms_init_plane_list(self) != 0) goto err_crtc_list;

  if (zn_kms_init_head_list(self, resources) != 0) goto err_plane_list;

  drmModeFreeResources(resources);

  return self;

err_plane_list:
  zn_kms_deinit_plane_list(self);

err_crtc_list:
  zn_kms_deinit_crtc_list(self);

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
  zn_kms_deinit_head_list(self);
  zn_kms_deinit_plane_list(self);
  zn_kms_deinit_crtc_list(self);
  free(self);
}
