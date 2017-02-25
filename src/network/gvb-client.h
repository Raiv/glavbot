#ifndef GVB_CLIENT_H
#define GVB_CLIENT_H

#include <glib.h>
#include <glib-object.h>
#include "gvb-client-opt.h"
#include "gvb-network-opt.h"

/**
 * GOBJECT defines
 ******************************************************************************/

#define GVB_TYPE_CLIENT             (gvb_client_get_type ())
#define GVB_CLIENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVB_TYPE_CLIENT, GvbClient))
#define GVB_IS_CLIENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVB_TYPE_CLIENT))
#define GVB_CLIENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GVB_TYPE_CLIENT, GvbClientClass))
#define GVB_IS_CLIENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GVB_TYPE_CLIENT))
#define GVB_CLIENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVB_TYPE_CLIENT, GvbClientClass))

/**
 * TYPEDEFs
 ******************************************************************************/
    
// Структуры класса
typedef struct _GvbClientClass GvbClientClass;
typedef struct _GvbClient GvbClient;
typedef struct _GvbClientPrivate GvbClientPrivate;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GvbClient, g_object_unref)
        
GvbClient*
gvb_client_new(GvbNetworkOptions *nwkopts, GvbClientOptions *options, GMainContext *context, GError **error);

gboolean
gvb_client_connect(GvbClient *self, GCancellable *cancellable, GError **error);

gboolean
gvb_client_get_output_stream(GvbClient *self, GOutputStream **ostream, GError **error);

gboolean
gvb_client_get_input_stream(GvbClient *self, GInputStream **istream, GError **error);

gboolean
gvb_client_get_rtt(GvbClient *self, guint32 *snd_rtt, guint32 *rcv_rtt, GError **error);

#endif /* GVB_CLIENT_H */

