#include <zukou.h>

namespace zukou {
VirtualObject::VirtualObject(App *app)
{
  app_ = app;
  virtual_object_ = zgn_compositor_create_virtual_object(app->compositor());
  frame_callback_ = nullptr;
  wl_proxy_set_user_data((wl_proxy *)virtual_object_, this);
}

VirtualObject::~VirtualObject()
{
  if (frame_callback_) wl_callback_destroy(frame_callback_);
  zgn_virtual_object_destroy(virtual_object_);
}

static void
frame_callback_handler(
    void *data, struct wl_callback *callback, uint32_t callback_data)
{
  VirtualObject *virtual_object = (VirtualObject *)data;
  wl_callback_destroy(callback);
  virtual_object->Frame(callback_data);
}

static const struct wl_callback_listener frame_callback_listener = {
    frame_callback_handler,
};

void
VirtualObject::NextFrame()
{
  frame_callback_ = zgn_virtual_object_frame(virtual_object_);
  wl_callback_add_listener(frame_callback_, &frame_callback_listener, this);
  zgn_virtual_object_commit(virtual_object_);
}

void
VirtualObject::Commit()
{
  zgn_virtual_object_commit(virtual_object_);
}

void
VirtualObject::Frame(uint32_t time)
{
  (void)time;
}

void
VirtualObject::DataDeviceEnter(uint32_t serial, glm::vec3 origin,
    glm::vec3 direction, struct zgn_data_offer *id)
{
  (void)serial;
  (void)origin;
  (void)direction;
  (void)id;
}

void
VirtualObject::DataDeviceLeave()
{}

void
VirtualObject::DataDeviceMotion(
    uint32_t time, glm::vec3 origin, glm::vec3 direction)
{
  (void)time;
  (void)origin;
  (void)direction;
}

void
VirtualObject::DataDeviceDrop()
{}

void
VirtualObject::RayEnter(uint32_t serial, glm::vec3 origin, glm::vec3 direction)
{
  (void)serial;
  (void)origin;
  (void)direction;
}

void
VirtualObject::RayLeave(uint32_t serial)
{
  (void)serial;
}

void
VirtualObject::RayMotion(uint32_t time, glm::vec3 origin, glm::vec3 direction)
{
  (void)time;
  (void)origin;
  (void)direction;
}

void
VirtualObject::RayButton(uint32_t serial, uint32_t time, uint32_t button,
    enum zgn_ray_button_state state)
{
  (void)serial;
  (void)time;
  (void)button;
  (void)state;
}

void
VirtualObject::KeyboardKeymap(
    enum zgn_keyboard_keymap_format format, int32_t fd, uint32_t size)
{
  (void)format;
  (void)fd;
  (void)size;
}

void
VirtualObject::KeyboardEnter(uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *keys)
{
  (void)serial;
  (void)virtual_object;
  (void)keys;
}

void
VirtualObject::KeyboardLeave(
    uint32_t serial, struct zgn_virtual_object *virtual_object)
{
  (void)serial;
  (void)virtual_object;
}

void
VirtualObject::KeyboardKey(uint32_t serial, uint32_t time, uint32_t key,
    enum zgn_keyboard_key_state state)
{
  (void)serial;
  (void)time;
  (void)key;
  (void)state;
}

}  // namespace zukou
