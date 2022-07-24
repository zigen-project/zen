#include "zen/display-system.h"

#include "zen-common.h"

static void
zn_display_system_handle_destroy(struct wl_resource *resource)
{
  wl_list_remove(&resource->link);
}

static void
zn_display_system_protocol_switch_type(
    struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
  struct zn_display_system *self = wl_resource_get_user_data(resource);
  struct zn_display_system_switch_event event;
  UNUSED(client);

  event.type = type;
  wl_signal_emit(&self->switch_signal, &event);
}

static void
zn_display_system_protocol_release(
    struct wl_client *client, struct wl_resource *resource)
{
  UNUSED(client);
  wl_resource_destroy(resource);
}

static const struct zen_display_system_interface zn_display_system_interface = {
    .switch_type = zn_display_system_protocol_switch_type,
    .release = zn_display_system_protocol_release,
};

static void
zn_display_system_bind(
    struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
  struct zn_display_system *self = data;
  struct wl_resource *resource;

  resource =
      wl_resource_create(client, &zen_display_system_interface, version, id);
  if (resource == NULL) {
    wl_client_post_no_memory(client);
    zn_error("Failed to create a wl_resource");
    return;
  }

  wl_resource_set_implementation(resource, &zn_display_system_interface, self,
      zn_display_system_handle_destroy);

  wl_list_insert(&self->resources, &resource->link);
}

void
zn_display_system_send_applied(
    struct zn_display_system *self, enum zen_display_system_type type)
{
  struct wl_resource *resource;
  wl_resource_for_each(resource, &self->resources)
      zen_display_system_send_applied(resource, type);
}

struct zn_display_system *
zn_display_system_create(struct wl_display *display)
{
  struct zn_display_system *self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->global = wl_global_create(
      display, &zen_display_system_interface, 1, self, zn_display_system_bind);
  if (self->global == NULL) {
    zn_error("Failed to create wl_global");
    goto err_free;
  }

  wl_list_init(&self->resources);
  wl_signal_init(&self->switch_signal);

  return self;

err_free:
  free(self);

err:
  return NULL;
}

void
zn_display_system_destroy(struct zn_display_system *self)
{
  struct wl_resource *resource, *tmp;

  wl_resource_for_each_safe(resource, tmp, &self->resources)
      wl_resource_destroy(resource);

  wl_global_destroy(self->global);
  free(self);
}
