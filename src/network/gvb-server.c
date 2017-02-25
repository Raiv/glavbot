#include "gvb-server.h"
#include "gvb-server-opt.h"
#include "gvb-network-opt.h"
#include "gvb-log.h"
#include "gvb-error.h"
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gnetworking.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/socket.h>

typedef enum {
    GVB_SERVER_STATE_NULL
  , GVB_SERVER_STATE_LISTEN
  , GVB_SERVER_STATE_SERVE
} GvbServerState;

typedef enum {
    GVB_SERVER_MESSAGE_ID_START_LISTEN
  , GVB_SERVER_MESSAGE_ID_HANDLE_CLIENT_CONNECTION
  , GVB_SERVER_MESSAGE_ID_CLIENT_IN_DATA
  , GVB_SERVER_MESSAGE_ID_FINALIZE
} GvbServerMessageId;

struct _GvbServer {
    GObject parent_instance;
};

struct _GvbServerClass {
    GObjectClass parent_class;
};

// TODO: вынести данные, нужные для поддержки клиентского соединения в отдельную
//   структуру, чтобы можно было поддерживать несколько клиентов
struct _GvbServerPrivate {
    GRecMutex            lock;    // !!! recursive is important
    GvbServerState       state;
    GThread             *server_thread;
    GAsyncQueue         *msg_queue;
    GMainContext        *main_ctx;
    GInetSocketAddress  *saddr;
    GSocketService      *service;
    GSocketConnection   *sconn;
    guint                ssource_id;
    gboolean             input_data_pend;   // флаг нужен, чтобы избавиться от множества повторяющихся сообщений GVB_SERVER_MESSAGE_ID_CLIENT_IN_DATA
    GvbInputDataCb       input_data_cb;
    gpointer             input_data_cb_data;
    gchar               *input_data_buf;
    gsize                input_data_buf_sz;
    guint32              snd_rtt;
    guint32              rcv_rtt;
};

typedef struct {
    GvbServerMessageId  id;
    GValue             *data;
} GvbServerMessage;

G_DEFINE_TYPE_WITH_PRIVATE(GvbServer, gvb_server, G_TYPE_OBJECT);

#define P(self)       G_TYPE_INSTANCE_GET_PRIVATE(self, GVB_TYPE_SERVER, GvbServerPrivate)
#define LOCK(self)    g_rec_mutex_lock(&P(self)->lock)
#define UNLOCK(self)  g_rec_mutex_unlock(&P(self)->lock)

// -----------------------------------------------------------------------------
// PROTOTYPES
// -----------------------------------------------------------------------------
static void
gvb_server_message_new(GvbServerMessageId id, GValue *data, GvbServerMessage **message);
static void
gvb_server_message_destroy(GvbServerMessage *message);
static void
gvb_server_message_push(GvbServer *self, GvbServerMessage *message);
static void
gvb_server_message_pop(GvbServer *self, guint64 wait_us, GvbServerMessage **message);
//------------------------------------------------------------------------------
static void
gvb_server_finalize(GObject* object);
static gpointer
gvb_server_thread_fn(gpointer user_data);
static void
gvb_server_start_listen(GvbServer *self);
static void
gvb_server_handle_client_connection(GvbServer *self, GSocketConnection *connection);
static void
gvb_server_handle_client_data(GvbServer *self);
static gboolean
gvb_server_client_connect_cb(GSocketService *service, GSocketConnection *connection, GObject *source_object, GvbServer *self);
static gboolean
gvb_server_client_socket_source_cb(GSocket *socket, GIOCondition condition, gpointer user_data);
static void
gvb_server_client_socket_source_destroy_cb(gpointer user_data);
static void
gvb_server_input_data_dummy_cb(gchar *data, gsize size, gpointer user_data);
static gboolean
gvb_server_set_input_data_pend_get_old(GvbServer *self, gboolean value);
// -----------------------------------------------------------------------------
static gboolean
gvb_server_timeout_cb(gpointer user_data);

static void
gvb_server_class_init(GvbServerClass *klass)
{
    GObjectClass *go_class = G_OBJECT_CLASS(klass);
    go_class->finalize = gvb_server_finalize;
}

static void
gvb_server_init(GvbServer *self)
{
    g_rec_mutex_init(&P(self)->lock);
    P(self)->state = GVB_SERVER_STATE_NULL;
    P(self)->msg_queue = g_async_queue_new_full((GDestroyNotify)gvb_server_message_destroy);
    P(self)->main_ctx = NULL;
    P(self)->saddr = NULL;
    P(self)->service = NULL;
    P(self)->sconn = NULL;
    P(self)->ssource_id = 0;
    P(self)->input_data_pend = FALSE;
    P(self)->input_data_cb = gvb_server_input_data_dummy_cb;
    P(self)->input_data_cb_data = self;
    P(self)->input_data_buf_sz = 200000; // 200K
    P(self)->input_data_buf = g_slice_alloc(P(self)->input_data_buf_sz);
    P(self)->snd_rtt = 0;
    P(self)->rcv_rtt = 0;
}

static void 
gvb_server_finalize(GObject* object)
{
    GvbServer *self = GVB_SERVER(object);
    GError *error = NULL;
    LOCK(self);
    
    // stop thread
    GvbServerMessage *fin_msg = NULL;
    gvb_server_message_new(GVB_SERVER_MESSAGE_ID_FINALIZE, NULL, &fin_msg);
    gvb_server_message_push(self, fin_msg);
    g_thread_join(P(self)->server_thread);
    
    // stop listening service
    if(P(self)->service) {
        // stop accepting new connections
        g_socket_service_stop(P(self)->service);
        // close all sockets
        g_socket_listener_close(G_SOCKET_LISTENER(P(self)->service));
        // unref service
        g_clear_object(&P(self)->service);
    }
    // close connection
    if(P(self)->sconn) {
        if(!g_io_stream_close(G_IO_STREAM(P(self)->sconn), NULL, &error)) {
            gvb_log_error(&error);
        }
        g_clear_object(&P(self)->sconn);
    }
    // remove socket source from main context
    if(P(self)->ssource_id>0) {
        g_source_destroy(g_main_context_find_source_by_id(P(self)->main_ctx, P(self)->ssource_id));
    }
    // unref main context
    g_main_context_unref(P(self)->main_ctx);
    // free address
    g_clear_object(&P(self)->saddr);
    // free queue
    g_async_queue_unref(P(self)->msg_queue);
    P(self)->msg_queue = NULL;
    // free buffer
    g_slice_free1(P(self)->input_data_buf_sz, P(self)->input_data_buf);
    
    UNLOCK(self);
    g_rec_mutex_clear(&P(self)->lock);
}

static void
gvb_server_message_destroy(GvbServerMessage *message)
{
    g_return_if_fail(message!=NULL);
    if(G_VALUE_HOLDS_OBJECT(message->data)) {
        gpointer value_obj = g_value_get_object(message->data);
        g_object_unref(value_obj);
    }
    // TODO: для остальных объектов тоже прописать, так как не все типы соответствуют объеткам
    g_free(message->data);
    g_free(message);
}

static void
gvb_server_message_new(GvbServerMessageId id, GValue *data, GvbServerMessage **message)
{
    g_return_if_fail(message!=NULL);
    g_return_if_fail(*message==NULL);
    GvbServerMessage *new_message = g_new(GvbServerMessage, 1);
    new_message->id = id;
    new_message->data = NULL;
    if(data) {
        new_message->data = g_new0(GValue,1);
        g_value_init(new_message->data, G_VALUE_TYPE(data));
        g_value_copy(data, new_message->data);
    }
    *message = new_message;
}

static void
gvb_server_message_push(GvbServer *self, GvbServerMessage *message)
{
    g_async_queue_push(P(self)->msg_queue, message);
}

static void
gvb_server_message_pop(GvbServer *self, guint64 wait_us, GvbServerMessage **message)
{
    *message = g_async_queue_timeout_pop(P(self)->msg_queue, wait_us);
}

GvbServer*
gvb_server_new(GvbNetworkOptions *nwkopts, GvbServerOptions *options, GMainContext *main_ctx, GError **error)
{
    g_return_val_if_fail(nwkopts!=NULL, NULL);
    g_return_val_if_fail(options!=NULL, NULL);
    
    GvbServer *self = g_object_new(GVB_TYPE_SERVER, NULL);
    P(self)->saddr = gvb_network_opt_get_inet_socket_addr(nwkopts);
    P(self)->service = g_threaded_socket_service_new(2);
    // ref to main context
    P(self)->main_ctx = (main_ctx ? g_main_context_ref(main_ctx) : g_main_context_ref_thread_default());
    // start server thread
    P(self)->server_thread = g_thread_new("server-thread", gvb_server_thread_fn, self);
    if(!P(self)->server_thread) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, "create server thread fails");
        g_object_unref(self);
        return NULL;
    }
    // move initial state - LISTEN
    GvbServerMessage *msg = NULL;
    gvb_server_message_new(GVB_SERVER_MESSAGE_ID_START_LISTEN, NULL, &msg);
    gvb_server_message_push(self, msg);
    
    return self;
}

static gpointer
gvb_server_thread_fn(gpointer user_data)
{
    GvbServer *self = GVB_SERVER(user_data);
    GvbServerMessage *msg = NULL;
    gboolean illegal_type = FALSE;
    gboolean exit_flag = FALSE;
    while(!exit_flag) 
    {
        gvb_server_message_pop(self, 2*G_TIME_SPAN_SECOND, &msg);
        if(!msg) {
            continue;
        }
        switch(msg->id) {
            case GVB_SERVER_MESSAGE_ID_START_LISTEN:
                gvb_log_message("GVB_SERVER_MESSAGE_START_LISTEN");
                gvb_server_start_listen(self);
                break;
            case GVB_SERVER_MESSAGE_ID_HANDLE_CLIENT_CONNECTION:
                gvb_log_message("GVB_SERVER_MESSAGE_HANDLE_CLIENT_CONNECTION");
                if(G_VALUE_HOLDS(msg->data, G_TYPE_SOCKET_CONNECTION)) {
                    GSocketConnection *connection = (GSocketConnection *)g_value_get_object(msg->data);
                    gvb_server_handle_client_connection(self, connection);
                }
                else {
                    illegal_type = TRUE;
                }
                break;
            case GVB_SERVER_MESSAGE_ID_CLIENT_IN_DATA:
                gvb_server_handle_client_data(self);
                break;
            case GVB_SERVER_MESSAGE_ID_FINALIZE:
                exit_flag = TRUE;
                break;
            default:
                gvb_log_critical("undefined message id: [%d]", msg->id);
        }
        if(illegal_type) {
            gvb_log_critical("illegal message [%d] data type [%s]", msg->id, G_VALUE_TYPE_NAME(msg->data));
            illegal_type = FALSE;
        }
        gvb_server_message_destroy(msg);
    }
    gvb_log_message("server thread stopped");
}

static void
gvb_server_start_listen(GvbServer *self)
{
    LOCK(self);
    
    GError *error = NULL;
    g_autoptr(GSocketAddress) eff_isock_addr = NULL;
    if(!g_socket_listener_add_address(
              G_SOCKET_LISTENER(P(self)->service)
            , G_SOCKET_ADDRESS(P(self)->saddr)
            , G_SOCKET_TYPE_STREAM
            , G_SOCKET_PROTOCOL_TCP
            , NULL             // source object
            , &eff_isock_addr  // effective address
            , &error
            ))
    {
        gvb_log_error(&error);
    }
    else {
        P(self)->state = GVB_SERVER_STATE_LISTEN;
        g_autofree gchar *sock_addr_str = gvb_network_sockaddr_to_str(eff_isock_addr);
        gvb_log_message("server start to listen at %s", sock_addr_str);
        g_signal_connect(P(self)->service, "incoming", (GCallback)gvb_server_client_connect_cb, self);
    }

    UNLOCK(self);
}

static void
gvb_server_handle_client_connection(GvbServer *self, GSocketConnection *connection)
{
    if(!connection) {
        gvb_log_critical("null connection");
        return;
    }
    
    // no need to unref socket
    GSocket *socket = g_socket_connection_get_socket(connection);
    // set non blocking mode & keep-alive
    g_socket_set_blocking(socket, FALSE);
    g_socket_set_keepalive(socket, TRUE);
    // 3-ий параметр GCancellable не совсем понятно, как с ним работать
    GSource *ssource = g_socket_create_source(socket, G_IO_IN | G_IO_PRI | G_IO_NVAL, NULL);
    if(!ssource) {
        gvb_log_critical("fails to create socket source");
    }
    g_source_set_callback(ssource, (GSourceFunc)gvb_server_client_socket_source_cb
            , self, (GDestroyNotify)gvb_server_client_socket_source_destroy_cb);
    
    LOCK(self);
    P(self)->ssource_id = g_source_attach(ssource, P(self)->main_ctx);
    if(P(self)->ssource_id<=0) {
        gvb_log_critical("fails to attach socket source");
        g_source_unref(ssource);
    }
    P(self)->sconn = g_object_ref(connection);
    P(self)->state = GVB_SERVER_STATE_SERVE;
    UNLOCK(self);
    gvb_log_message("client connected");
}

static gboolean
gvb_server_client_connect_cb(GSocketService *service, GSocketConnection *connection, GObject *source_object, GvbServer *self)
{
    gvb_log_message("gvb_server_client_connect_cb");
    GValue msg_data = G_VALUE_INIT;
    g_value_init(&msg_data, G_TYPE_SOCKET_CONNECTION);
    // value holds ref to connection
    g_value_set_object(&msg_data, connection);
    GvbServerMessage *msg = NULL;
    gvb_server_message_new(GVB_SERVER_MESSAGE_ID_HANDLE_CLIENT_CONNECTION, &msg_data, &msg);
    gvb_server_message_push(self, msg);
    return TRUE;
}

static gboolean
gvb_server_client_socket_source_cb(GSocket *socket, GIOCondition condition, gpointer user_data)
{
    GvbServer *self = GVB_SERVER(user_data);
    if(condition & G_IO_ERR) {
        gvb_log_message("gvb_server_client_socket_source_cb [%p]: G_IO_ERR", self);
    }
    if(condition & G_IO_HUP) {
        gvb_log_message("gvb_server_client_socket_source_cb [%p]: G_IO_HUP", self);
    }
    if(condition & G_IO_IN) {
        // old was false
        if(!gvb_server_set_input_data_pend_get_old(self, TRUE)) {
//            gvb_log_message("gvb_server_client_socket_source_cb [%p]: G_IO_IN", self);
            GvbServerMessage *msg = NULL;
            gvb_server_message_new(GVB_SERVER_MESSAGE_ID_CLIENT_IN_DATA, NULL, &msg);
            gvb_server_message_push(self, msg);
        }
    }
    if(condition & G_IO_PRI) {
        gvb_log_message("gvb_server_client_socket_source_cb [%p]: G_IO_PRI", self);
    }
    if(condition & G_IO_NVAL) {
        gvb_log_message("gvb_server_client_socket_source_cb [%p]: G_IO_NVAL", self);
    }
    return G_SOURCE_CONTINUE;
}

static void
gvb_server_client_socket_source_destroy_cb(gpointer user_data)
{
    GvbServer *self = GVB_SERVER(user_data);
    gvb_log_message("gvb_server_client_socket_source_destroy_cb [%p]", self);
}

static void
gvb_server_client_close_connection(GvbServer *self)
{
    LOCK(self);
    g_source_destroy(g_main_context_find_source_by_id(P(self)->main_ctx, P(self)->ssource_id));
    P(self)->ssource_id = 0;
    g_clear_object(&P(self)->sconn);
    P(self)->input_data_pend = FALSE;
    P(self)->state = GVB_SERVER_STATE_LISTEN;
    gvb_log_message("client disconnected");
    UNLOCK(self);
}

static void
gvb_server_handle_client_data(GvbServer *self)
{
    LOCK(self);
    
    gvb_server_set_input_data_pend_get_old(self, FALSE);
    
    if(P(self)->state!=GVB_SERVER_STATE_SERVE) {
        UNLOCK(self);
        gvb_log_warning("illegal state != GVB_SERVER_STATE_SERVE");
        return;
    }
    
    GError *error = NULL;
    gssize rec_size = 0;
    
    // we don't use input stream because it uses blocking model
    // GIOStream *stream = G_IO_STREAM(P(self)->sconn);
    // GInputStream *in_stream = g_io_stream_get_input_stream(stream);  // no need to unref
    
    GSocket *socket = g_socket_connection_get_socket(P(self)->sconn);   // no need to unref
    for(;;) {
        rec_size = g_socket_receive(socket, P(self)->input_data_buf, P(self)->input_data_buf_sz, NULL, &error);
        //gvb_log_message("rec_size == [%d]", rec_size);
        // connection was closed
        if(rec_size==0) {
            gvb_server_client_close_connection(self);
            break;
        }
        // error occured
        else if(rec_size<0) {
            if(error->code==G_IO_ERROR_WOULD_BLOCK) {
                break; // no more data available
            }
        }
        else {
            GvbInputDataCb data_cb = P(self)->input_data_cb;
            if(data_cb) {
                data_cb(P(self)->input_data_buf, rec_size, self);
            }
        }
    }
    
    UNLOCK(self);
}

static void
gvb_server_input_data_dummy_cb(gchar *data, gsize size, gpointer user_data)
{
    static guint32 missed_bytes = 0, missed_calls = 0;
    missed_bytes += size;
    missed_calls++;
    //
    GvbServer *self = GVB_SERVER(user_data);
    guint32 snd_rtt=0, rcv_rtt=0;
    GError *error = NULL;
    if(!gvb_server_get_rtt(self, &snd_rtt, &rcv_rtt, &error)) {
        gvb_log_error(&error);
        return;
    }
    if(missed_bytes%2000==0 || missed_calls%15==0) {
        gvb_log_message("missed [%d] bytes, \t[rcv:%d] - [snd:%d]", missed_bytes, rcv_rtt, snd_rtt);
    }
}

gboolean
gvb_server_set_input_data_cb(GvbServer *self, GvbInputDataCb callback, GError **error)
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    if(!callback) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "callback is null");
        return FALSE;
    }
    gvb_log_message("gvb_server_set_input_data_cb: [%p]", callback);
    LOCK(self);
    P(self)->input_data_cb = callback;
    UNLOCK(self);
    return TRUE;
}

// return old value
static gboolean
gvb_server_set_input_data_pend_get_old(GvbServer *self, gboolean value)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    gboolean ret_val = FALSE;
    LOCK(self);
    ret_val = P(self)->input_data_pend;
    if(P(self)->input_data_pend!=value) {
        P(self)->input_data_pend = value;
    }
    UNLOCK(self);
    return ret_val;
}

gboolean
gvb_server_get_rtt(GvbServer *self, guint32 *snd_rtt, guint32 *rcv_rtt, GError **error)
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: self=[%p]", self);
        return FALSE;
    }
    LOCK(self);
    gvb_network_socket_get_info(
            g_socket_connection_get_socket(P(self)->sconn)
          , snd_rtt
          , rcv_rtt
          , error );
    UNLOCK(self);
    if(*error) {
        return FALSE;
    }
    return TRUE;
}