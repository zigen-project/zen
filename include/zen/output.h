#ifndef ZEN_OUTPUT_H
#define ZEN_OUTPUT_H

#include <wlr/types/wlr_output.h>

#include "zen/server.h"

/** this destroys itself when the given wlr_output is destroyed */
struct zn_output;

void zn_output_update_global(struct zn_output *self);

struct zn_output *zn_output_create(
    struct wlr_output *wlr_output, struct zn_server *server);

#endif  //  ZEN_OUTPUT_H
