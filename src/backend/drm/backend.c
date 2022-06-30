#include <assert.h>
#include <fcntl.h>
#include <libudev.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <zen-backend.h>
#include <zen-libinput.h>
#include <zen-session.h>
#include <zen-util.h>

struct zn_drm_backend {
  struct zn_backend base;

  struct zn_session *session;
  struct udev *udev;
  struct zn_libinput *input;

  struct {
    int id;
    int fd;
    char *filename;
    dev_t devnum;
  } drm;
};

static const char default_seat[] = "seat0";

static int
zn_drm_backend_check_kms_and_set_drm(
    struct zn_drm_backend *self, struct udev_device *device)
{
  const char *filename = udev_device_get_devnode(device);
  const char *sysnum = udev_device_get_sysnum(device);
  dev_t devnum = udev_device_get_devnum(device);
  drmModeRes *resource;
  int id = -1, fd;

  if (!filename) goto err;

  fd = zn_session_open_file(self->session, filename, O_RDWR);
  if (fd < 0) goto err;

  resource = drmModeGetResources(fd);
  if (resource == NULL) goto err_fd;

  if (resource->count_crtcs <= 0 || resource->count_connectors <= 0 ||
      resource->count_encoders <= 0)
    goto err_res;

  if (sysnum) id = atoi(sysnum);
  if (sysnum == NULL || id < 0) {
    zn_log("drm backend: failed to get sysnum for device %s\n", filename);
    goto err_res;
  }

  /* if called multiple times, delete the old entry if it exists */
  if (self->drm.fd >= 0) zn_session_close_file(self->session, self->drm.fd);
  free(self->drm.filename);

  self->drm.fd = fd;
  self->drm.id = id;
  self->drm.filename = strdup(filename);
  self->drm.devnum = devnum;

  drmModeFreeResources(resource);

  return 0;

err_res:
  drmModeFreeResources(resource);

err_fd:
  zn_session_close_file(self->session, fd);

err:
  return -1;
}

static struct udev_device *
zn_drm_backend_find_primary_gpu(struct zn_drm_backend *self, const char *seat)
{
  struct udev_enumerate *e;
  struct udev_list_entry *entry;
  const char *path, *device_seat, *id;
  struct udev_device *device, *drm_device, *pci;

  e = udev_enumerate_new(self->udev);
  udev_enumerate_add_match_subsystem(e, "drm");
  udev_enumerate_add_match_sysname(e, "card[0-9]*");

  udev_enumerate_scan_devices(e);

  drm_device = NULL;

  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
  {
    bool is_boot_vga = false;

    path = udev_list_entry_get_name(entry);
    device = udev_device_new_from_syspath(self->udev, path);
    if (device == NULL) continue;

    device_seat = udev_device_get_property_value(device, "ID_SEAT");
    if (device_seat == NULL) device_seat = default_seat;

    if (strcmp(device_seat, seat) != 0) {
      udev_device_unref(device);
      continue;
    }

    pci = udev_device_get_parent_with_subsystem_devtype(device, "pci", NULL);
    if (pci) {
      id = udev_device_get_sysattr_value(pci, "boot_vga");
      if (id && strcmp(id, "1") == 0) is_boot_vga = true;
    }

    if (!is_boot_vga && drm_device) {
      udev_device_unref(device);
      continue;
    }

    if (zn_drm_backend_check_kms_and_set_drm(self, device) != 0) {
      udev_device_unref(device);
      continue;
    }

    if (is_boot_vga) {
      if (drm_device) udev_device_unref(device);
      drm_device = device;
      break;
    }

    assert(drm_device == NULL);
    drm_device = device;
  }

  assert(!!drm_device == (self->drm.fd >= 0));

  udev_enumerate_unref(e);
  return drm_device;
}

ZN_EXPORT struct zn_backend *
zn_backend_create(struct wl_display *display)
{
  struct zn_drm_backend *self;
  struct udev_device *drm_device;
  const char *seat_id = default_seat;  // FIXME: enable to configure

  self = zalloc(sizeof *self);
  if (self == NULL) goto err;

  self->drm.fd = -1;

  self->session = zn_session_create(display);
  if (self->session == NULL) {
    zn_log("drm backend: failed to create a session\n");
    goto err_free;
  }

  zn_log("Press Q to exit...\n");  // FIXME: for dev only, remove this later...

  if (zn_session_connect(self->session, seat_id) != 0) {
    zn_log("drm backend: session connection failed\n");
    goto err_session;
  }

  self->udev = udev_new();
  if (self->udev == NULL) {
    zn_log("drm backend: failed to initialize udev context\n");
    goto err_session;
  }

  /* if this call succeeds, self->drm will be filled*/
  drm_device = zn_drm_backend_find_primary_gpu(self, seat_id);
  if (drm_device == NULL) {
    zn_log("drm backend: failed to find valid drm device\n");
    goto err_udev;
  }

  self->input = zn_libinput_create(self->udev, self->session, display, seat_id);
  if (self->input == NULL) {
    zn_log("drm backend: failed to create input devices\n");
    goto err_drm_device;
  }

  udev_device_unref(drm_device);

  return &self->base;

err_drm_device:
  udev_device_unref(drm_device);

err_udev:
  udev_unref(self->udev);

err_session:
  zn_session_destroy(self->session);

err_free:
  free(self);

err:
  return NULL;
}

ZN_EXPORT void
zn_backend_destroy(struct zn_backend *parent)
{
  struct zn_drm_backend *self = zn_container_of(parent, self, base);

  zn_libinput_destroy(self->input);
  udev_unref(self->udev);
  zn_session_close_file(self->session, self->drm.fd);
  zn_session_destroy(self->session);
  free(self->drm.filename);
  free(self);
}
