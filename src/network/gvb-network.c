#include "gvb-network.h"
#include <glib.h>
#include <glib-object.h>
#include <gio/gnetworking.h>
#include <string.h>
#include <errno.h>
#include "gvb-log.h"
#include "gvb-error.h"

GType
gvb_network_mode_get_type(void)
{
    static volatile gsize network_mode_type_id = 0;
    if (g_once_init_enter (&network_mode_type_id))
    {
        static const GEnumValue values[] = {
            { GVB_NETWORK_MODE_SERVER, "GVB_NETWORK_MODE_SERVER", "server" },
            { GVB_NETWORK_MODE_CLIENT, "GVB_NETWORK_MODE_SERVER", "client" },
            { 0, NULL, NULL }
        };
        GType type_id = g_enum_register_static ("GvbNetworkMode", values);
        g_once_init_leave (&network_mode_type_id, type_id);
    }
    return network_mode_type_id;
}

gchar*
gvb_network_sockaddr_to_str(GSocketAddress *sock_addr)
{
    g_return_val_if_fail(sock_addr!=NULL, NULL);
    GInetSocketAddress *inet_sock_addr = G_INET_SOCKET_ADDRESS(sock_addr);
    
    // !!!
    // from docs:
    // the GInetAddress for address , which ____must be g_object_ref()'d___ if it will be stored. 
    GInetAddress *inet_addr = g_inet_socket_address_get_address(inet_sock_addr);
    g_autofree gchar *inet_addr_str = g_inet_address_to_string(inet_addr);
    guint16 inet_addr_port = g_inet_socket_address_get_port(inet_sock_addr);
    
    GString *str = g_string_new(NULL);
    g_string_printf(str, "[%s:%d]", inet_addr_str, inet_addr_port);
    gchar *ret_chr = g_string_free(str, FALSE);
    
    return ret_chr;
}

gchar*
gvb_network_socket_connection_to_str(GSocketConnection *connection)
{
    g_return_val_if_fail(connection!=NULL, NULL);
    GError *error = NULL;
    g_autofree gchar *local_addr_str = NULL;
    g_autofree gchar *remote_addr_str = NULL;
    GSocketAddress *local_addr = g_socket_connection_get_local_address(connection, &error);
    if(local_addr) {
        local_addr_str = gvb_network_sockaddr_to_str(local_addr);
        g_object_unref(local_addr);
    }
    else {
        gvb_log_error(&error);
    }
    GSocketAddress *remote_addr = g_socket_connection_get_remote_address(connection, &error);
    if(remote_addr) {
        remote_addr_str = gvb_network_sockaddr_to_str(remote_addr);
        g_object_unref(remote_addr);
    }
    else {
        gvb_log_error(&error);
    }
    return g_strconcat("(", local_addr_str, ")-->(", remote_addr_str, ")", NULL);
}

gboolean
gvb_network_socket_get_info(GSocket *socket
    , guint32 *snd_rtt, guint32 *rcv_rtt
    , guint32 *snd_mss, guint32 *rcv_mss
    , GError **error
)
{
    if(!socket) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "null pointer: socket=[%p]", socket);
        return FALSE;
    }
    
    struct tcp_info socket_info = {0}; //WARN: big structure on stack
    socklen_t opt_len = sizeof(struct tcp_info);

    gint socket_fd = g_socket_get_fd(socket);
    if(getsockopt(socket_fd, SOL_TCP, TCP_INFO, &socket_info, &opt_len)<0
            || opt_len!=sizeof(struct tcp_info)) 
    {
        g_set_error(error, GVB_ERROR, GVB_ERROR_OTHER, "getsockopt error: [%s]", strerror(errno));
        return FALSE;
    }
    if(snd_rtt)
        *snd_rtt = socket_info.tcpi_rtt;
    if(rcv_rtt)
        *rcv_rtt = socket_info.tcpi_rcv_rtt;
    if(snd_mss)
        *snd_mss = socket_info.tcpi_snd_mss;
    if(rcv_mss)
        *rcv_mss = socket_info.tcpi_rcv_mss;
    
    return TRUE;
}