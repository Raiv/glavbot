#ifndef GVB_SERVER_H
#define GVB_SERVER_H

#include <glib.h>
#include <glib-object.h>
#include "gvb-server-opt.h"
#include "gvb-network-opt.h"

/**
 * GOBJECT defines
 ******************************************************************************/

#define GVB_TYPE_SERVER             (gvb_server_get_type ())
#define GVB_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVB_TYPE_SERVER, GvbServer))
#define GVB_IS_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVB_TYPE_SERVER))
#define GVB_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GVB_TYPE_SERVER, GvbServerClass))
#define GVB_IS_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GVB_TYPE_SERVER))
#define GVB_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVB_TYPE_SERVER, GvbServerClass))

GType gvb_server_get_type(void);

/**
 * TYPEDEFs
 ******************************************************************************/
    
// Структуры класса
typedef struct _GvbServerClass GvbServerClass;
typedef struct _GvbServer GvbServer;
typedef struct _GvbServerPrivate GvbServerPrivate;

typedef void (*GvbInputDataCb)(gchar *data, gsize size, gpointer user_data);
typedef void (*GvbClientConnectCb)(gboolean connected, gpointer user_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GvbServer, g_object_unref)

GvbServer*
gvb_server_new(
        GvbNetworkOptions *nwkopts, GvbServerOptions *options
      , GMainContext *main_ctx, GError **error
      );

gboolean
gvb_server_set_input_data_cb(
        GvbServer *self, GvbInputDataCb callback
      , gpointer user_data, GError **error
      );

gboolean
gvb_server_set_client_connect_cb(
        GvbServer *self, GvbClientConnectCb callback
      , gpointer user_data, GError **error
      );

gboolean
gvb_server_get_client_connection(
        GvbServer *self, GSocketConnection **connection, GError **error);

gboolean
gvb_server_get_rtt(GvbServer *self
    , guint32 *snd_rtt, guint32 *rcv_rtt
    , guint32 *snd_mss, guint32 *rcv_mss
    , GError **error);

#endif /* GVB_SERVER_H */
