#ifndef GVB_UI_H
#define GVB_UI_H

#include <glib.h>
#include "gvb-camera.h"
#include "gvb-server.h"
#include "gvb-client.h"

gboolean
gvb_ui_start(GMainContext *main_ctx, GError **error);

gboolean
gvb_ui_stop(GError **error);

gboolean
gvb_ui_set_camera(GvbCamera *camera, GError **error);

gboolean
gvb_ui_set_client(GvbClient *client, GError **error);

gboolean
gvb_ui_set_server(GvbServer *server, GError **error);

void
gvb_ui_refresh();

#endif /* GVB_UI_H */

