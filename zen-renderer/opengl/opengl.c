#include "opengl.h"

#include <libzen-compositor/libzen-compositor.h>
#include <zigen-opengl-server-protocol.h>

#include "opengl-component.h"
#include "opengl-element-array-buffer.h"
#include "opengl-shader-program.h"
#include "opengl-texture.h"
#include "opengl-vertex-buffer.h"

WL_EXPORT void zen_opengl_destroy(struct zen_opengl* opengl);

static void
zen_opengl_protocol_destroy(
    struct wl_client* client, struct wl_resource* resource)
{
  UNUSED(client);
  UNUSED(resource);
}

static void
zen_opengl_protocol_create_opengl_component(struct wl_client* client,
    struct wl_resource* resource, uint32_t id,
    struct wl_resource* virtual_object_resource)
{
  struct zen_opengl* opengl;
  struct zen_virtual_object* virtual_object;
  struct zen_opengl_component* component;

  opengl = wl_resource_get_user_data(resource);
  virtual_object = wl_resource_get_user_data(virtual_object_resource);

  component =
      zen_opengl_component_create(client, id, opengl->renderer, virtual_object);
  if (component == NULL) {
    zen_log("zen opengl: failed to create a opengl component\n");
    return;
  }
}

static void
zen_opengl_protocol_create_vertex_buffer(
    struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
  UNUSED(resource);
  struct zen_opengl_vertex_buffer* vertex_buffer;

  vertex_buffer = zen_opengl_vertex_buffer_create(client, id);
  if (vertex_buffer == NULL) {
    zen_log("zen opengl: failed to create a vertex buffer\n");
  }
}

static void
zen_opengl_protocol_create_element_array_buffer(
    struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
  UNUSED(resource);
  struct zen_opengl_element_array_buffer* element_array_burfer;

  element_array_burfer = zen_opengl_element_array_buffer_create(client, id);
  if (element_array_burfer == NULL) {
    zen_log("zen opengl: failed to create a element array buffer\n");
  }
}

static void
zen_opengl_protocol_create_shader_program(
    struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
  UNUSED(resource);
  struct zen_opengl_shader_program* shader;

  shader = zen_opengl_shader_program_create(client, id);
  if (shader == NULL) {
    zen_log("zen opengl: failed to create a shader program\n");
  }
}

static void
zen_opengl_protocol_create_texture(
    struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
  UNUSED(resource);
  struct zen_opengl_texture* texture;

  texture = zen_opengl_texture_create(client, id);
  if (texture == NULL) {
    zen_log("zen opengl: failed to create a texture\n");
  }
}

static const struct zgn_opengl_interface opengl_interface = {
    .destroy = zen_opengl_protocol_destroy,
    .create_opengl_component = zen_opengl_protocol_create_opengl_component,
    .create_vertex_buffer = zen_opengl_protocol_create_vertex_buffer,
    .create_element_array_buffer =
        zen_opengl_protocol_create_element_array_buffer,
    .create_shader_program = zen_opengl_protocol_create_shader_program,
    .create_texture = zen_opengl_protocol_create_texture,
};

static void
zen_opengl_bind(
    struct wl_client* client, void* data, uint32_t version, uint32_t id)
{
  UNUSED(client);
  UNUSED(data);
  UNUSED(version);
  UNUSED(id);
  struct zen_opengl* opengl = data;
  struct wl_resource* resource;

  resource = wl_resource_create(client, &zgn_opengl_interface, version, id);
  if (resource == NULL) {
    wl_client_post_no_memory(client);
    zen_log("zen opengl: failed to create a resource\n");
    return;
  }

  wl_resource_set_implementation(resource, &opengl_interface, opengl, NULL);
}

WL_EXPORT struct zen_opengl*
zen_opengl_create(
    struct zen_compositor* compositor, struct zen_opengl_renderer* renderer)
{
  struct zen_opengl* opengl;
  struct wl_global* global;

  opengl = zalloc(sizeof *opengl);
  if (opengl == NULL) {
    zen_log("zen opengl: failed to allocate memory\n");
    goto err;
  }

  global = wl_global_create(
      compositor->display, &zgn_opengl_interface, 1, opengl, zen_opengl_bind);
  if (global == NULL) {
    zen_log("zen opengl: failed to create a zen opengl global\n");
    goto err_global;
  }

  opengl->global = global;
  opengl->compositor = compositor;
  opengl->renderer = renderer;

  return opengl;

err_global:
  free(opengl);

err:
  return NULL;
}

WL_EXPORT void
zen_opengl_destroy(struct zen_opengl* opengl)
{
  wl_global_destroy(opengl->global);
  free(opengl);
}
