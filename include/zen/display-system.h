#ifndef ZEN_DISPLAY_SYSTEM_H
#define ZEN_DISPLAY_SYSTEM_H

#include <wayland-server-core.h>
#include <zen-desktop-protocol.h>

struct zn_display_system_switch_event {
  enum zen_display_system_type type;
};

struct zn_display_system {
  struct wl_global* global;

  struct wl_list resources;

  struct wl_signal switch_signal;  // (struct zn_display_system_switch_event *)
};

void zn_display_system_send_applied(
    struct zn_display_system* self, enum zen_display_system_type type);

struct zn_display_system* zn_display_system_create(struct wl_display* display);

void zn_display_system_destroy(struct zn_display_system* self);

#endif  //  ZEN_DISPLAY_SYSTEM_H
