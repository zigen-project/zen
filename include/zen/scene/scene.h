#ifndef ZEN_SCENE_H
#define ZEN_SCENE_H

#include <wayland-server-core.h>

#include "zen/scene/screen-layout.h"

struct zn_scene {
  struct zn_screen_layout* screen_layout;
};

struct zn_scene* zn_scene_create();

void zn_scene_destroy(struct zn_scene* self);

#endif  //  ZEN_SCENE_H
