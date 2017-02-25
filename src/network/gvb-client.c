#include "gvb-client.h"
#include "gvb-client-opt.h"
#include "gvb-log.h"
#include "gvb-error.h"
// glib
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gnetworking.h>
// system
#include <linux/net_tstamp.h>
#include <errno.h>
#include <sys/socket.h>
// standard
#include <string.h>
#include <unistd.h>

struct _GvbClient {
    GObject parent_instance;
};

struct _GvbClientClass {
    GObjectClass parent_class;
};

struct _GvbClientPrivate {
    GRecMutex            lock;
    GInetSocketAddress  *saddr;
    GSocketClient       *client;
    GSocketConnection   *sconn;
    GMainContext        *main_ctx;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvbClient, gvb_client, G_TYPE_OBJECT);

#define P(self)       G_TYPE_INSTANCE_GET_PRIVATE(self, GVB_TYPE_CLIENT, GvbClientPrivate)
#define LOCK(self)    g_rec_mutex_lock(&P(self)->lock)
#define UNLOCK(self)  g_rec_mutex_unlock(&P(self)->lock)

// -----------------------------------------------------------------------------
// PROTOTYPES
// -----------------------------------------------------------------------------
static void 
gvb_client_finalize(GObject* object);
static void
gvb_client_connection_event (
        GSocketClient      *self
      , GSocketClientEvent  event
      , GSocketConnectable *connectable
      , GIOStream          *connection
      , gpointer            user_data);

//------------------------------------------------------------------------------

static void
gvb_client_class_init(GvbClientClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = gvb_client_finalize;
}

static void
gvb_client_init(GvbClient *self)
{
    g_rec_mutex_init(&P(self)->lock);
    P(self)->saddr = NULL;
    P(self)->client = NULL;
    P(self)->sconn = NULL;
    P(self)->main_ctx = NULL;
}

static void 
gvb_client_finalize(GObject* object)
{
    GvbClient *self = GVB_CLIENT(object);
    GError *error = NULL;
    LOCK(self);
    // close connection
    if(P(self)->sconn) {
        if(!g_io_stream_close(G_IO_STREAM(P(self)->sconn), NULL, &error)) {
            gvb_log_error(&error);
        }
        g_clear_object(&P(self)->sconn);
    }
    // close client
    g_clear_object(&P(self)->client);
    // free address
    g_clear_object(&P(self)->saddr);
    // free main_ctx
    g_clear_object(&P(self)->main_ctx);
    UNLOCK(self);
    g_rec_mutex_clear(&P(self)->lock);
}

GvbClient*
gvb_client_new(GvbNetworkOptions *nwkopts, GvbClientOptions *options, GMainContext *context, GError **error)
{
    if(!nwkopts || !options) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: nwkopts=[%p], options=[%p]", nwkopts, options);
        return NULL;
    }
    GvbClient *self = g_object_new(GVB_TYPE_CLIENT, NULL);
    P(self)->saddr = gvb_network_opt_get_inet_socket_addr(nwkopts);
    P(self)->client = g_socket_client_new();
    P(self)->main_ctx = context;
    //
    g_socket_client_set_family(P(self)->client, G_SOCKET_FAMILY_IPV4);
    g_socket_client_set_socket_type(P(self)->client, G_SOCKET_TYPE_STREAM);
    g_socket_client_set_protocol(P(self)->client, G_SOCKET_PROTOCOL_TCP);
    g_signal_connect(P(self)->client, "event", (GCallback)gvb_client_connection_event, self);
    //
    return self;
}

static void
gvb_client_connection_event (
        GSocketClient      *self
      , GSocketClientEvent  event
      , GSocketConnectable *connectable
      , GIOStream          *connection
      , gpointer            user_data)
{
    GEnumClass *enum_cls = g_type_class_ref(G_TYPE_SOCKET_CLIENT_EVENT);
    GEnumValue *enum_val = g_enum_get_value(enum_cls, event);
    g_type_class_unref(enum_cls);
    if(enum_val) {
        gvb_log_message("client_connection_event: %s", enum_val->value_name);
    }
    else {
        gvb_log_message("client_connection_event: %d", event);
    }
}

gboolean
gvb_client_connect(GvbClient *self, GCancellable *cancellable, GError **error)
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: self=[%p]", self);
        return FALSE;
    }
    LOCK(self);
    
    if(P(self)->sconn && g_socket_connection_is_connected(P(self)->sconn)) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE, "already connected: self=[%p]", self);
        UNLOCK(self);
        return FALSE;
    }
    
    // clear old connection
    if(P(self)->sconn) {
        g_clear_object(&P(self)->sconn);
    }
    
    gvb_log_message("connectiong client...");
    if((P(self)->sconn = g_socket_client_connect(
                P(self)->client
              , G_SOCKET_CONNECTABLE(P(self)->saddr)
              , cancellable
              , error)))
    {
        gchar *str = gvb_network_socket_connection_to_str(P(self)->sconn);
        gvb_log_message("client connected: [%s]", str);
        
//        GSocket *socket = g_socket_connection_get_socket(P(self)->sconn);
//        g_socket_set_keepalive()
        
        g_free(str);
    }
    else
    {
        if(*error) {
            g_prefix_error(error, "client connect fails: ");
        }
        else {
            g_set_error_literal(error, GVB_ERROR, GVB_ERROR_NETWORK, "client connect fails");
        }
        UNLOCK(self);
        return FALSE;
    }
    
    UNLOCK(self);
    return TRUE;
}

gboolean
gvb_client_get_output_stream(GvbClient *self, GOutputStream **ostream, GError **error)
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: self=[%p]", self);
        return FALSE;
    }
    // if pointer is null or target pointer is not null
    if(!ostream || *ostream) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "bad stream pointer: ostream=[%p]", ostream);
        return FALSE;
    }
    GIOStream *stream = NULL;
    LOCK(self);
    if(!P(self)->sconn) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE, "client not connected: self=[%p]", self);
        UNLOCK(self);
        return FALSE;
    }
    stream = G_IO_STREAM(P(self)->sconn);
    *ostream = g_io_stream_get_output_stream(stream);
    UNLOCK(self);
    return TRUE;
}

gboolean
gvb_client_get_input_stream(GvbClient *self, GInputStream **istream, GError **error)
{
    g_set_error(error, GVB_ERROR, GVB_ERROR_OTHER, "not implemented yet");
    return FALSE;
}

gboolean
gvb_client_get_rtt(GvbClient *self
    , guint32 *snd_rtt, guint32 *rcv_rtt
    , guint32 *snd_mss, guint32 *rcv_mss
    , GError **error)
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: self=[%p]", self);
        return FALSE;
    }
    
    LOCK(self);
    if(!P(self)->sconn) {
        UNLOCK(self);
        g_set_error(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE, "no client connection");
        return FALSE;
    }
    if(!gvb_network_socket_get_info(
            g_socket_connection_get_socket(P(self)->sconn)
          , snd_rtt
          , rcv_rtt
          , snd_mss
          , rcv_mss
          , error )) {
        UNLOCK(self);
        return FALSE;
    }
    UNLOCK(self);
    return TRUE;
}
