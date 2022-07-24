#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wayland-client.h>
#include <zen-common.h>
#include <zen-desktop-client-protocol.h>

/**
 * handle command input, echo inputting command
 *
 * inputting command string are buffer from buf[0] to buf[cur-1]
 */
struct command {
  char buf[32];
  unsigned int cur;
};

struct client {
  struct wl_display *display;
  struct wl_registry *registry;
  struct zen_display_system *display_system;
  char *socket;

  struct command cmd;
};

static bool
char_is_alphanumeric(char c)
{
  if ('a' <= c && c <= 'z') return true;
  if ('A' <= c && c <= 'Z') return true;
  if ('0' <= c && c <= '9') return true;
  return false;
}

static void
cursor_erase_line()
{
  printf("\x1b[2K");  // clear line
  printf("\x1b[0G");  // move cursor to the head of the line
}

/** n can be negative */
static void
cursor_move_right(int n)
{
  if (n > 0)
    printf("\x1b[%dC", n);  // move right
  else if (n < 0)
    printf("\x1b[%dD", -n);  // move left
}

/** remove chars after the cursor */
static void
cursor_erase_after()
{
  printf("\x1b[0J");
}

static void
command_add_char(struct command *cmd, char c)
{
  if (cmd->cur >= ARRAY_LENGTH(cmd->buf)) return;

  cmd->buf[cmd->cur++] = c;
  fputc(c, stdout);
}

static void
command_remove_char(struct command *cmd, unsigned int n)
{
  unsigned int remove_len = cmd->cur < n ? cmd->cur : n;
  cmd->cur -= remove_len;
  cursor_move_right(-remove_len);
  cursor_erase_after();
}

/** clear line and re-print inputting command */
static void
command_print_buf(struct command *cmd)
{
  cursor_erase_line();
  printf("> %.*s", cmd->cur, cmd->buf);
}

/**
 * clear buffer and return command string with trailing '\0'
 *
 * @return caller needs to free this pointer
 */
static char *
command_commit(struct command *cmd)
{
  char *str = malloc(sizeof(char) * (cmd->cur + 1));
  sprintf(str, "%.*s", cmd->cur, cmd->buf);
  cmd->cur = 0;
  command_print_buf(cmd);
  return str;
}

/**
 * print formatted string above the inputting command
 *
 * formatted string should include trailing \n
 */
static void
command_printf(struct command *cmd, const char *fmt, ...)
{
  va_list args;

  cursor_erase_line();

  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  command_print_buf(cmd);
  fflush(stdout);
}

static void
command_init(struct command *cmd)
{
  cmd->cur = 0;
}

static void
display_system_applied(
    void *data, struct zen_display_system *zen_display_system, uint32_t type)
{
  struct client *cui = data;
  UNUSED(zen_display_system);

  command_printf(&cui->cmd, "<<< new display system type applied: %s\n",
      type == ZEN_DISPLAY_SYSTEM_TYPE_SCREEN ? "screen" : "immersive");
}

static const struct zen_display_system_listener display_system_listener = {
    .applied = display_system_applied,
};

static void
registry_global(void *data, struct wl_registry *wl_registry, uint32_t name,
    const char *interface, uint32_t version)
{
  struct client *cui = data;
  if (strcmp(interface, "zen_display_system") == 0) {
    cui->display_system = wl_registry_bind(
        wl_registry, name, &zen_display_system_interface, version);
    zen_display_system_add_listener(
        cui->display_system, &display_system_listener, cui);
  }
}

static void
registry_global_remove(
    void *data, struct wl_registry *wl_registry, uint32_t name)
{
  UNUSED(data);
  UNUSED(wl_registry);
  UNUSED(name);
  zn_abort("Fatal: zen_display_system global has removed");
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static bool
connect(struct client *cui, char *socket_name)
{
  zn_debug("Trying to connect to %s", socket_name);

  cui->display = wl_display_connect(socket_name);
  if (cui->display == NULL) return false;

  cui->registry = wl_display_get_registry(cui->display);
  wl_registry_add_listener(cui->registry, &registry_listener, cui);

  wl_display_dispatch(cui->display);
  wl_display_roundtrip(cui->display);

  if (cui->display_system == NULL) {
    wl_registry_destroy(cui->registry);
    cui->registry = NULL;
    wl_display_disconnect(cui->display);
    cui->display = NULL;
    return false;
  }

  cui->socket = strdup(socket_name);
  return true;
}

static bool
connect_auto(struct client *cui)
{
  char socket_name_candidate[16];

  for (int i = 0; i <= 32; i++) {
    snprintf(socket_name_candidate, ARRAY_LENGTH(socket_name_candidate),
        "wayland-%d", i);
    if (connect(cui, socket_name_candidate)) return true;
  }

  return false;
}

static void
disconnect(struct client *cui)
{
  wl_registry_destroy(cui->registry);
  cui->registry = NULL;
  wl_display_disconnect(cui->display);
  cui->display = NULL;
  free(cui->socket);
  cui->socket = NULL;
}

/** return true when exit */
static bool
handle_command(struct client *cui, char *command)
{
  enum zen_display_system_type type;

  if (strcmp(command, "screen") == 0) {
    type = ZEN_DISPLAY_SYSTEM_TYPE_SCREEN;
  } else if (strcmp(command, "immersive") == 0) {
    type = ZEN_DISPLAY_SYSTEM_TYPE_IMMERSIVE;
  } else if (strcmp(command, "exit") == 0) {
    cursor_erase_line();
    printf("exit...\n");
    return true;
  } else {
    command_printf(&cui->cmd, "xxx %s (unexpected command)\n", command);
    return false;
  }

  zen_display_system_switch_type(cui->display_system, type);
  wl_display_flush(cui->display);
  command_printf(&cui->cmd, ">>> %s\n", command);
  return false;
}

/**
 * @return int 0: continue, -1: error, 1: exit
 */
static int
handle_stdin(struct client *cui)
{
  char c = fgetc(stdin);
  if (c == EOF) return -1;

  if (c == '\n') {
    char *command = command_commit(&cui->cmd);
    if (handle_command(cui, command)) return 1;
    command_print_buf(&cui->cmd);
    free(command);
  } else if (c == '\b' || c == 127) {  // BackSpace / Delete
    command_remove_char(&cui->cmd, 1);
  } else if (char_is_alphanumeric(c)) {
    command_add_char(&cui->cmd, c);
  }
  fflush(stdout);
  return 0;
}

static void
set_stdin_non_canonical_and_no_echo()
{
  struct termios opt;
  tcgetattr(STDIN_FILENO, &opt);
  opt.c_lflag &= ~ICANON;
  opt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &opt);
}

int
main()
{
  struct client cui;
  char *socket;
  int fd, status = EXIT_FAILURE;
  fd_set fds;
  bool ret;

  zn_log_init(ZEN_DEBUG, NULL);
  command_init(&cui.cmd);

  socket = getenv("WAYLAND_DISPLAY");
  if (socket)
    ret = connect(&cui, socket);
  else
    ret = connect_auto(&cui);

  if (ret == false) {
    zn_error("Failed to connect to server");
    goto err;
  }

  zn_info("Connected to %s", cui.socket);

  fd = wl_display_get_fd(cui.display);

  set_stdin_non_canonical_and_no_echo();

  fprintf(stdout,
      "Interactive shell started\n"
      "The only available commands are\n"
      "\n"
      "\"screen\"   : request server to start screen type of display system\n"
      "\"immersive\": request server to start immersive type of display "
      "system\n"
      "\"exit\"     : exit\n"
      "\n");

  command_print_buf(&cui.cmd);

  fflush(stdout);

  while (1) {
    int ret;
    int max_fd;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_SET(STDIN_FILENO, &fds);
    max_fd = fd > STDIN_FILENO ? fd : STDIN_FILENO;

    while (wl_display_prepare_read(cui.display) != 0) {
      if (errno != EAGAIN) return false;
      if (wl_display_dispatch_pending(cui.display) == -1) goto err_disconnect;
    }

    if (wl_display_flush(cui.display) == -1) goto err_disconnect;

    ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

    if (ret < 0 && errno == EINTR) continue;
    if (ret < 0) goto err_disconnect;
    if (ret == 0) continue;

    if (FD_ISSET(fd, &fds)) {
      if (wl_display_read_events(cui.display) == -1) goto err_disconnect;
      if (wl_display_dispatch_pending(cui.display) < 0) goto err_disconnect;
    } else {
      wl_display_cancel_read(cui.display);
    }

    if (FD_ISSET(STDIN_FILENO, &fds)) {
      ret = handle_stdin(&cui);
      if (ret == 1) break;
      if (ret == -1) goto err_disconnect;
    }
  }

  status = EXIT_SUCCESS;

err_disconnect:
  disconnect(&cui);

err:
  return status;
}
