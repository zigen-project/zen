#ifndef LIBZEN_COMPOSIOR_HELPERS_H
#define LIBZEN_COMPOSIOR_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cglm/cglm.h>
#include <stdlib.h>
#include <wayland-server.h>

#define UNUSED(x) ((void)x)

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef container_of
#define container_of(ptr, type, member)                    \
  ({                                                       \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member));     \
  })
#endif

static inline void *
zalloc(size_t size)
{
  return calloc(1, size);
}

int glm_vec3_from_wl_array(vec3 v, struct wl_array *array);

void glm_vec3_to_wl_array(vec3 v, struct wl_array *array);

int zen_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif

#endif  //  LIBZEN_COMPOSIOR_HELPERS_H
