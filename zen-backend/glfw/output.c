#include "output.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <libzen-compositor/libzen-compositor.h>
#include <time.h>
#include <wayland-server.h>
#include <zen-renderer/opengl-renderer.h>

// clang-format off

// Distance unit: meter
// IPD 62 mm
// eye height: 1500 mm
#define LEFT_EYE_VIEW {           \
  {   1.0f,  0.0f, 0.0f, 0.0f},   \
  {   0.0f,  1.0f, 0.0f, 0.0f},   \
  {   0.0f,  0.0f, 1.0f, 0.0f},   \
  { 0.031f, -1.5f, 0.0f, 1.0f}}

#define RIGHT_EYE_VIEW {          \
  {   1.0f,  0.0f, 0.0f, 0.0f},   \
  {   0.0f,  1.0f, 0.0f, 0.0f},   \
  {   0.0f,  0.0f, 1.0f, 0.0f},   \
  {-0.031f, -1.5f, 0.0f, 1.0f}}

// far clip: 1000, near clip: 0.001, field of view: 120° (60° each side), right handed
#define EYE_PROJECTION {                          \
  {0.577f,   0.0f, 0.0f,                 0.0f},   \
  {  0.0f, 0.577f, 0.0f,                 0.0f},   \
  {  0.0f,   0.0f, -1.000002000002f,    -1.0f},   \
  {  0.0f,   0.0f, -0.002000002000002f,  0.0f}}
// clang-format on

static void
glfw_output_set_window_size(
    struct glfw_output* output, uint32_t width, uint32_t height)
{
  uint32_t x, y, l;
  if (output->width == width && output->height == height) return;
  output->width = width;
  output->height = height;
  l = MIN(width / 2, height);
  x = width / 2 - l;
  y = (height - l) / 2;

  output->eyes[0].viewport.height = l;
  output->eyes[0].viewport.width = l;
  output->eyes[0].viewport.x = x;
  output->eyes[0].viewport.y = y;
  output->eyes[1].viewport.height = l;
  output->eyes[1].viewport.width = l;
  output->eyes[1].viewport.x = x + l;
  output->eyes[1].viewport.y = y;
}

static void
glfw_output_repaint(struct zen_output* zen_output)
{
  struct glfw_output* output = (struct glfw_output*)zen_output;
  zen_opengl_renderer_set_cameras(
      output->renderer, output->eyes, ARRAY_LENGTH(output->eyes));
  zen_opengl_renderer_render(output->renderer);
  // TODO: check if the rendering is in time.
}

static int
swap_timer_loop(void* data)
{
  struct glfw_output* output = data;
  struct timespec next_repaint;
  int64_t refresh_msec;
  int width, height;

  glfwSwapBuffers(output->window);

  if (timespec_get(&output->base.frame_time, TIME_UTC) < 0)
    zen_log("glfw output: [WARNING] failed to get current time\n");

  glfwPollEvents();
  if (glfwWindowShouldClose(output->window))
    wl_display_terminate(output->compositor->display);

  glfwGetWindowSize(output->window, &width, &height);
  glfw_output_set_window_size(output, width, height);

  refresh_msec = millihz_to_nsec(output->refresh) / 1000000;
  timespec_add_msec(&next_repaint, &output->base.frame_time, refresh_msec);

  wl_event_source_timer_update(output->swap_timer,
      refresh_msec - output->compositor->repaint_window_msec + 1);

  zen_compositor_finish_frame(output->compositor, next_repaint);

  return 0;
}

static uint32_t
get_refresh_rate()
{
  // primary_monitor & mode will be freed by glfw
  GLFWmonitor* primary_monitor;
  const GLFWvidmode* mode;

  primary_monitor = glfwGetPrimaryMonitor();
  if (primary_monitor == NULL) {
    zen_log("glfw output: failed to get primary monitor\n");
    return 0;
  }

  mode = glfwGetVideoMode(primary_monitor);
  if (mode == NULL) {
    zen_log("glfw output: failed to get mode of primary monitor\n");
    return 0;
  }

  return mode->refreshRate * 1000;
}

WL_EXPORT struct zen_output*
zen_output_create(struct zen_compositor* compositor)
{
  // primary_monitor & mode will be freed by glfw
  GLFWmonitor* primary_monitor;
  const GLFWvidmode* mode;
  struct glfw_output* output;
  GLFWwindow* window;
  uint32_t refresh;
  uint32_t initial_width;
  uint32_t initial_height;
  struct wl_event_loop* loop;
  struct wl_event_source* swap_timer;
  struct zen_opengl_renderer* renderer;
  mat4 right_eye_view = RIGHT_EYE_VIEW;
  mat4 left_eye_view = LEFT_EYE_VIEW;
  mat4 eye_projection = EYE_PROJECTION;
  uint32_t refresh_msec;

  primary_monitor = glfwGetPrimaryMonitor();
  if (primary_monitor == NULL) {
    zen_log("openvr backend - gl window: failed to get primary monitor\n");
    goto err_primary_monitor;
  }

  mode = glfwGetVideoMode(primary_monitor);
  if (mode == NULL) {
    zen_log(
        "openvr backend - gl window: failed to get mode of primary monitor\n");
    goto err_mode;
  }

  if (compositor->config->fullscreen_preview) {
    initial_width = mode->width;
    initial_height = mode->height;
    window = glfwCreateWindow(
        initial_width, initial_height, "zen compositor", primary_monitor, NULL);
  } else {
    initial_width = 640;
    initial_height = 320;
    window = glfwCreateWindow(
        initial_width, initial_height, "zen compositor", NULL, NULL);
  }

  if (window == NULL) {
    zen_log("glfw output: failed to create a window\n");
    goto err_window;
  }

  glfwMakeContextCurrent(window);

  if (compositor->config->hidden_cursor)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  GLenum glewError = glewInit();
  if (glewError != GLEW_OK) {
    zen_log("glfw output: failed to initialize glew: %s\n",
        glewGetErrorString(glewError));
    goto err_glew;
  }

  renderer = zen_opengl_renderer_get(compositor->renderer);
  if (renderer == NULL) {
    zen_log("glfw output: renderer type not supported: %s\n",
        compositor->renderer->type);
    goto err_renderer;
  }

  output = zalloc(sizeof *output);
  if (output == NULL) {
    zen_log("glfw output: failed to allocate memory\n");
    goto err_output;
  }

  refresh = get_refresh_rate();
  if (refresh == 0) {
    zen_log(
        "glfw output: [WARNING] failed to get refresh rate. use 60 fps "
        "instead.\n");
    refresh = 60000;
  }

  loop = wl_display_get_event_loop(compositor->display);
  swap_timer = wl_event_loop_add_timer(loop, swap_timer_loop, output);

  timespec_get(&output->base.frame_time, TIME_UTC);
  output->base.repaint = glfw_output_repaint;
  output->compositor = compositor;
  output->renderer = renderer;
  output->window = window;
  glfw_output_set_window_size(output, initial_width, initial_height);
  output->refresh = refresh;
  output->swap_timer = swap_timer;
  glm_mat4_copy(left_eye_view, output->eyes[0].view_matrix);
  glm_mat4_copy(eye_projection, output->eyes[0].projection_matrix);
  glm_mat4_copy(right_eye_view, output->eyes[1].view_matrix);
  glm_mat4_copy(eye_projection, output->eyes[1].projection_matrix);

  // start swap loop
  swap_timer_loop(output);

  refresh_msec = millihz_to_nsec(refresh) / 1000000;
  compositor->repaint_window_msec = refresh_msec - 1;

  return &output->base;

err_output:
err_renderer:
err_glew:
  glfwDestroyWindow(window);

err_window:
err_mode:
err_primary_monitor:
  return NULL;
}

WL_EXPORT void
zen_output_destroy(struct zen_output* output)
{
  struct glfw_output* o = (struct glfw_output*)output;

  wl_event_source_remove(o->swap_timer);
  glfwDestroyWindow(o->window);
  free(output);
}
