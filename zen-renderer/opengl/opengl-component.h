#ifndef ZEN_RENDERER_OPENGL_COMPONENT_H
#define ZEN_RENDERER_OPENGL_COMPONENT_H

#include <GL/glew.h>
#include <libzen-compositor/libzen-compositor.h>
#include <wayland-server.h>
#include <zen-renderer/opengl-renderer.h>
#include <zigen-opengl-server-protocol.h>
#include <zigen-server-protocol.h>

struct zen_opengl_vertex_attribute {
  uint32_t index;
  uint32_t size;
  enum zgn_opengl_vertex_attribute_type type;
  uint32_t normalized;
  uint32_t stride;
  uint32_t pointer;
};

struct zen_opengl_component {
  struct wl_resource *resource;
  struct zen_virtual_object *virtual_object;
  struct wl_list link;

  GLuint vertex_array_id;

  struct {
    struct zen_weak_link vertex_buffer_link;
    struct zen_weak_link element_array_buffer_link;
    struct zen_weak_link shader_program_link;
    struct zen_weak_link texture_link;
    uint32_t min;
    uint32_t count;
    enum zgn_opengl_topology topology;
    struct wl_array vertex_attributes;
  } current;

  struct {
    struct zen_weak_link vertex_buffer_link;
    struct zen_weak_link element_array_buffer_link;
    struct zen_weak_link shader_program_link;
    struct zen_weak_link texture_link;
    uint32_t min;
    uint32_t count;
    enum zgn_opengl_topology topology;
    struct wl_array vertex_attributes;
    bool min_changed;
    bool count_changed;
    bool topology_changed;
    bool vertex_attributes_changed;
  } pending;

  struct wl_listener virtual_object_commit_listener;
  struct wl_listener virtual_object_destroy_listener;
};

struct zen_opengl_component *zen_opengl_component_create(
    struct wl_client *client, uint32_t id, struct zen_opengl_renderer *renderer,
    struct zen_virtual_object *virtual_object);

void zen_opengl_component_destroy(struct zen_opengl_component *component);

void zen_opengl_component_render(struct zen_opengl_component *component,
    struct zen_opengl_renderer_camera *camera);

#endif  //  ZEN_RENDERER_OPENGL_COMPONENT_H
