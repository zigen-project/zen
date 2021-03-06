#include <GL/glew.h>
#include <libzen-compositor/libzen-compositor.h>
#include <string.h>
#include <wayland-server.h>
#include <zen-renderer/opengl-renderer.h>
#include <zen-shell/zen-shell.h>

#include "cuboid-window-render-item.h"
#include "opengl-component.h"
#include "opengl.h"
#include "ray-render-item.h"

char* zen_opengl_renderer_type = "zen_opengl_renderer";

WL_EXPORT struct zen_renderer*
zen_renderer_create(struct zen_compositor* compositor)
{
  struct zen_opengl_renderer* renderer;
  struct zen_opengl* opengl;

  if (compositor->shell_base->type != zen_shell_type) {
    zen_log("opengl renderer: unsupported shell type: %s\n",
        compositor->shell_base->type);
    goto err;
  }

  renderer = zalloc(sizeof *renderer);
  if (renderer == NULL) {
    zen_log("opengl renderer: failed to create opengl renderer\n");
    goto err;
  }

  opengl = zen_opengl_create(compositor, renderer);
  if (opengl == NULL) {
    zen_log("opengl renderer: failed to create zen opengl\n");
    goto err_opengl;
  }

  renderer->base.type = zen_opengl_renderer_type;
  renderer->compositor = compositor;
  renderer->opengl = opengl;
  renderer->cameras = NULL;
  renderer->camera_count = 0;
  renderer->camera_allocate = 0;
  wl_list_init(&renderer->ray_render_item_list);
  wl_list_init(&renderer->cuboid_window_render_item_list);
  wl_list_init(&renderer->component_list);

  return &renderer->base;

err_opengl:
  free(renderer);

err:
  return NULL;
}

WL_EXPORT void
zen_renderer_destroy(struct zen_renderer* renderer_base)
{
  struct zen_opengl_renderer* renderer = zen_opengl_renderer_get(renderer_base);
  if (renderer->camera_allocate > 0) free(renderer->cameras);
  zen_opengl_destroy(renderer->opengl);
  free(renderer);
}

WL_EXPORT struct zen_opengl_renderer*
zen_opengl_renderer_get(struct zen_renderer* renderer_base)
{
  struct zen_opengl_renderer* renderer;
  if (renderer_base->type != zen_opengl_renderer_type) return NULL;

  renderer = wl_container_of(renderer_base, renderer, base);

  return renderer;
}

WL_EXPORT void
zen_opengl_renderer_set_cameras(struct zen_opengl_renderer* renderer,
    struct zen_opengl_renderer_camera* cameras, uint32_t count)
{
  size_t size = sizeof(struct zen_opengl_renderer_camera) * count;
  if (count > renderer->camera_allocate) {
    free(renderer->cameras);
    renderer->cameras = malloc(size);
    renderer->camera_allocate = count;
  }
  memcpy(renderer->cameras, cameras, size);
  renderer->camera_count = count;
}

WL_EXPORT void
zen_opengl_renderer_render(struct zen_opengl_renderer* renderer)
{
  struct zen_opengl_renderer_camera* camera;

  if (renderer->camera_count == 0) {
    static bool warned = false;
    if (!warned)
      zen_log(
          "opengl renderer: [WARNING] tried to render without any camera\n");
    warned = true;
    return;
  }

  for (camera = renderer->cameras;
       camera < renderer->cameras + renderer->camera_count; camera++) {
    glBindFramebuffer(GL_FRAMEBUFFER, camera->framebuffer_id);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  glEnable(GL_MULTISAMPLE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_NOTEQUAL, 0);
  for (camera = renderer->cameras;
       camera < renderer->cameras + renderer->camera_count; camera++) {
    struct zen_opengl_ray_render_item* ray_render_item;
    struct zen_opengl_cuboid_window_render_item* cuboid_window_render_item;
    struct zen_opengl_component* component;

    glBindFramebuffer(GL_FRAMEBUFFER, camera->framebuffer_id);
    glViewport(camera->viewport.x, camera->viewport.y, camera->viewport.width,
        camera->viewport.height);

    // render rays
    wl_list_for_each(ray_render_item, &renderer->ray_render_item_list, link)
    {
      zen_opengl_ray_render_item_render(ray_render_item, camera);
    }

    // render cuboid windows
    wl_list_for_each(cuboid_window_render_item,
        &renderer->cuboid_window_render_item_list, link)
    {
      zen_opengl_cuboid_window_render_item_render(
          cuboid_window_render_item, camera);
    }

    // render opengl components
    wl_list_for_each(component, &renderer->component_list, link)
    {
      zen_opengl_component_render(component, camera);
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  renderer->camera_count = 0;
}
