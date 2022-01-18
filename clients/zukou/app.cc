#include <errno.h>
#include <string.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zukou.h>

namespace zukou {
App::App() {}

App::~App()
{
  if (display_) wl_display_disconnect(display_);
}

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version)
{
  App *app = (App *)data;
  app->GlobalRegistryHandler(registry, id, interface, version);
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
  App *app = (App *)data;
  app->GlobalRegistryRemover(registry, id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover,
};

static void
seat_capabilities(void *data, struct zgn_seat *seat, uint32_t capability)
{
  App *app = (App *)data;
  app->Capabilities(seat, capability);
}

static const struct zgn_seat_listener seat_listener = {
    seat_capabilities,
};

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
  App *app = (App *)data;
  app->ShmFormat(wl_shm, format);
}

static const struct wl_shm_listener shm_listener = {
    shm_format,
};

static void
ray_enter(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *origin,
    struct wl_array *direction)
{
  App *app = (App *)data;
  glm::vec3 origin_vec = glm_vec3_from_wl_array(origin);
  glm::vec3 direction_vec = glm_vec3_from_wl_array(direction);
  app->RayEnter(ray, serial, virtual_object, origin_vec, direction_vec);
}

static void
ray_leave(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  App *app = (App *)data;
  app->RayLeave(ray, serial, virtual_object);
}

static void
ray_motion(void *data, struct zgn_ray *ray, uint32_t time,
    struct wl_array *origin, struct wl_array *direction)
{
  App *app = (App *)data;
  glm::vec3 origin_vec = glm_vec3_from_wl_array(origin);
  glm::vec3 direction_vec = glm_vec3_from_wl_array(direction);
  app->RayMotion(ray, time, origin_vec, direction_vec);
}

static void
ray_button(void *data, struct zgn_ray *ray, uint32_t serial, uint32_t time,
    uint32_t button, uint32_t state)
{
  App *app = (App *)data;
  app->RayButton(ray, serial, time, button, (enum zgn_ray_button_state)state);
}

static const struct zgn_ray_listener ray_listener = {
    ray_enter,
    ray_leave,
    ray_motion,
    ray_button,
};

static void
keyboard_keymap(void *data, struct zgn_keyboard *keyboard, uint32_t format,
    int32_t fd, uint32_t size)
{
  App *app = (App *)data;
  app->KeyboardKeymap(
      keyboard, (enum zgn_keyboard_keymap_format)format, fd, size);
}

static void
keyboard_enter(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *keys)
{
  App *app = (App *)data;
  app->KeyboardEnter(keyboard, serial, virtual_object, keys);
}

static void
keyboard_leave(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  App *app = (App *)data;
  app->KeyboardLeave(keyboard, serial, virtual_object);
}

static void
keyboard_key(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    uint32_t time, uint32_t key, uint32_t state)
{
  App *app = (App *)data;
  app->KeyboardKey(
      keyboard, serial, time, key, (enum zgn_keyboard_key_state)state);
}

static void
keyboard_modifiers(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
  (void)data;
  (void)keyboard;
  (void)serial;
  (void)mods_depressed;
  (void)mods_latched;
  (void)mods_locked;
  (void)group;
}

static const struct zgn_keyboard_listener keyboard_listener = {
    keyboard_keymap,
    keyboard_enter,
    keyboard_leave,
    keyboard_key,
    keyboard_modifiers,
};

bool
App::Connect(const char *socket)
{
  display_ = wl_display_connect(socket);
  if (display_ == nullptr) goto err;

  registry_ = wl_display_get_registry(display_);
  if (registry_ == nullptr) goto err_registry;

  wl_registry_add_listener(registry_, &registry_listener, this);

  wl_display_dispatch(display_);
  wl_display_roundtrip(display_);

  if (compositor_ == nullptr || shm_ == nullptr || opengl_ == nullptr ||
      shell_ == nullptr)
    goto err_globals;

  return true;

err_globals:
  wl_registry_destroy(registry_);

err_registry:
  wl_display_disconnect(display_);
  display_ = NULL;

err:
  return false;
}

void
App::GlobalRegistryHandler(struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version)
{
  if (strcmp(interface, "zgn_compositor") == 0) {
    compositor_ = (zgn_compositor *)wl_registry_bind(
        registry, id, &zgn_compositor_interface, version);
  } else if (strcmp(interface, "zgn_seat") == 0) {
    seat_ = (zgn_seat *)wl_registry_bind(
        registry, id, &zgn_seat_interface, version);
    zgn_seat_add_listener(seat_, &seat_listener, this);
  } else if (strcmp(interface, "wl_shm") == 0) {
    shm_ = (wl_shm *)wl_registry_bind(registry, id, &wl_shm_interface, version);
    wl_shm_add_listener(shm_, &shm_listener, this);
  } else if (strcmp(interface, "zgn_opengl") == 0) {
    opengl_ = (zgn_opengl *)wl_registry_bind(
        registry, id, &zgn_opengl_interface, version);
  } else if (strcmp(interface, "zgn_shell") == 0) {
    shell_ = (zgn_shell *)wl_registry_bind(
        registry, id, &zgn_shell_interface, version);
  }
}

void
App::GlobalRegistryRemover(struct wl_registry *registry, uint32_t id)
{
  (void)registry;
  (void)id;
}

void
App::ShmFormat(struct wl_shm *wl_shm, uint32_t format)
{
  (void)wl_shm;
  (void)format;
}

void
App::Capabilities(struct zgn_seat *seat, uint32_t capability)
{
  if (capability & ZGN_SEAT_CAPABILITY_RAY && ray_ == NULL) {
    ray_ = zgn_seat_get_ray(seat);
    zgn_ray_add_listener(ray_, &ray_listener, this);
  }

  if (!(capability & ZGN_SEAT_CAPABILITY_RAY) && ray_) {
    zgn_ray_release(ray_);
    ray_ = NULL;
  }

  if (capability & ZGN_SEAT_CAPABILITY_KEYBOARD && keyboard_ == NULL) {
    keyboard_ = zgn_seat_get_keyboard(seat);
    zgn_keyboard_add_listener(keyboard_, &keyboard_listener, this);
  }

  if (!(capability & ZGN_SEAT_CAPABILITY_KEYBOARD) && keyboard_) {
    zgn_keyboard_release(keyboard_);
    keyboard_ = NULL;
  }
}

void
App::RayEnter(struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object, glm::vec3 origin,
    glm::vec3 direction)
{
  (void)ray;
  ray_focus_virtual_object_ = virtual_object;

  VirtualObject *v =
      (VirtualObject *)wl_proxy_get_user_data((wl_proxy *)virtual_object);

  v->RayEnter(serial, origin, direction);
}

void
App::RayLeave(struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  (void)ray;
  ray_focus_virtual_object_ = NULL;

  VirtualObject *v =
      (VirtualObject *)wl_proxy_get_user_data((wl_proxy *)virtual_object);

  v->RayLeave(serial);
}

void
App::RayMotion(
    struct zgn_ray *ray, uint32_t time, glm::vec3 origin, glm::vec3 direction)
{
  (void)ray;
  if (ray_focus_virtual_object_ == NULL) return;

  VirtualObject *v = (VirtualObject *)wl_proxy_get_user_data(
      (wl_proxy *)ray_focus_virtual_object_);

  v->RayMotion(time, origin, direction);
}

void
App::RayButton(struct zgn_ray *ray, uint32_t serial, uint32_t time,
    uint32_t button, enum zgn_ray_button_state state)
{
  (void)ray;
  if (ray_focus_virtual_object_ == NULL) return;

  VirtualObject *v = (VirtualObject *)wl_proxy_get_user_data(
      (wl_proxy *)ray_focus_virtual_object_);

  v->RayButton(serial, time, button, state);
}

void
App::KeyboardKeymap(struct zgn_keyboard *keyboard,
    enum zgn_keyboard_keymap_format format, int32_t fd, uint32_t size)
{
  (void)keyboard;
  if (keyboard_focus_virtual_object_ == NULL) return;

  VirtualObject *v = (VirtualObject *)wl_proxy_get_user_data(
      (wl_proxy *)keyboard_focus_virtual_object_);

  v->KeyboardKeymap(format, fd, size);
}

void
App::KeyboardEnter(struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *keys)
{
  (void)keyboard;
  keyboard_focus_virtual_object_ = virtual_object;

  VirtualObject *v =
      (VirtualObject *)wl_proxy_get_user_data((wl_proxy *)virtual_object);

  v->KeyboardEnter(serial, virtual_object, keys);
}

void
App::KeyboardLeave(struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  (void)keyboard;
  keyboard_focus_virtual_object_ = NULL;

  VirtualObject *v =
      (VirtualObject *)wl_proxy_get_user_data((wl_proxy *)virtual_object);

  v->KeyboardLeave(serial, virtual_object);
}

void
App::KeyboardKey(struct zgn_keyboard *keyboard, uint32_t serial, uint32_t time,
    uint32_t key, enum zgn_keyboard_key_state state)
{
  (void)keyboard;
  if (keyboard_focus_virtual_object_ == NULL) return;

  VirtualObject *v = (VirtualObject *)wl_proxy_get_user_data(
      (wl_proxy *)keyboard_focus_virtual_object_);

  v->KeyboardKey(serial, time, key, state);
}

bool
App::Run()
{
  int ret;
  running_ = true;
  while (running_) {
    while (wl_display_prepare_read(display()) != 0) {
      if (errno != EAGAIN) return false;
      wl_display_dispatch_pending(display());
    }
    ret = wl_display_flush(display());
    if (ret == -1) return false;
    wl_display_read_events(display());
    wl_display_dispatch_pending(display());
  }
  return true;
}

void
App::Terminate()
{
  running_ = false;
}

}  // namespace zukou
