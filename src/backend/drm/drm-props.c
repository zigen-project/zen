#include "drm-props.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zen-util.h>

/**
 * This method doesn't fully support all types of props.
 * See the linux kernel documentation for detail.
 */
int
zn_drm_property_info_populate(int drm_fd,
    const struct zn_drm_property_info *src, struct zn_drm_property_info *dest,
    unsigned int num_dests, drmModeObjectProperties *props)
{
  drmModePropertyRes *prop;

  for (unsigned i = 0; i < num_dests; i++) {
    dest[i].name = src[i].name;
    dest[i].prop_id = 0;
    dest[i].num_enum_values = src[i].num_enum_values;

    if (src[i].enum_values == 0) continue;

    dest[i].enum_values =
        malloc(src[i].num_enum_values * sizeof(*dest[i].enum_values));

    if (dest[i].enum_values == NULL) {
      for (unsigned j = 0; j < i; j++) free(dest[j].enum_values);
      return -1;
    }

    for (unsigned j = 0; j < dest[i].num_enum_values; j++) {
      dest[i].enum_values[j].name = src[i].enum_values[j].name;
      dest[i].enum_values[j].valid = false;
    }
  }

  /* iterate props first because the cost of drmModeGetProperty is unknown */
  for (unsigned i = 0; i < props->count_props; i++) {
    unsigned j;
    prop = drmModeGetProperty(drm_fd, props->props[i]);
    if (prop == NULL) continue;

    for (j = 0; j < num_dests; j++) {
      if (strcmp(prop->name, dest[j].name) == 0) break;
    }

    if (j == num_dests) continue;

    if (dest[j].num_enum_values == 0 && (prop->flags & DRM_MODE_PROP_ENUM)) {
      zn_log(
          "DRM Warning: expected property %s not to be an enum, but it is; "
          "ignoring\n",
          prop->name);
      continue;
    }

    dest[j].prop_id = props->props[i];
    dest[j].flags = prop->flags;

    if (prop->flags & DRM_MODE_PROP_RANGE ||
        prop->flags & DRM_MODE_PROP_SIGNED_RANGE) {
      dest[j].num_range_values = prop->count_values;
      for (int k = 0; k < prop->count_values; k++)
        dest[j].range_values[k] = prop->values[k];
    }

    if (dest[j].num_enum_values == 0) {
      drmModeFreeProperty(prop);
      continue;
    }

    for (unsigned k = 0; k < dest[j].num_enum_values; k++) {
      int l;

      for (l = 0; l < prop->count_enums; l++) {
        if (strcmp(prop->enums[l].name, dest[j].enum_values[k].name) == 0)
          break;
      }

      if (l == prop->count_enums) continue;

      dest[j].enum_values[k].valid = true;
      dest[j].enum_values[k].value = prop->enums[l].value;
    }

    drmModeFreeProperty(prop);
  }

  return 0;
}

void
zn_drm_property_info_clear(struct zn_drm_property_info *info, int num_props)
{
  for (int i = 0; i < num_props; i++) free(info[i].enum_values);

  memset(info, 0, sizeof(*info) * num_props);
}
