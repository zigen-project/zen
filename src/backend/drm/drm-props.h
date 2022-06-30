#ifndef ZEN_DRM_BACKEND_DRM_PROPS__H
#define ZEN_DRM_BACKEND_DRM_PROPS__H

#include <stdbool.h>
#include <stdint.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct zn_drm_property_enum_info {
  const char *name;
  bool valid;
  uint64_t value;
};

struct zn_drm_property_info {
  const char *name;
  uint32_t prop_id;
  uint32_t flags;
  unsigned int num_enum_values;
  struct zn_drm_property_enum_info *enum_values;
  unsigned int num_range_values;
  uint64_t range_values[2];
};

int zn_drm_property_info_populate(int drm_fd,
    const struct zn_drm_property_info *src, struct zn_drm_property_info *dest,
    unsigned int num_dests, drmModeObjectProperties *props);

void zn_drm_property_info_clear(
    struct zn_drm_property_info *info, int num_props);

#endif  //  ZEN_DRM_BACKEND_DRM_PROPS__H
