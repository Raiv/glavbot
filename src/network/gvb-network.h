/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-network.h
 * Author: Miha
 *
 * Created on September 11, 2016, 1:58 PM
 */

#ifndef GVB_NETWORK_H
#define GVB_NETWORK_H

#include <glib-object.h>
#include <gio/gio.h>

typedef enum {
    GVB_NETWORK_MODE_INVALD
  , GVB_NETWORK_MODE_SERVER
  , GVB_NETWORK_MODE_CLIENT
} GvbNetworkMode;

#define GVB_TYPE_NETWORK_MODE (gvb_network_mode_get_type())

GType
gvb_network_mode_get_type(void);

gchar*
gvb_network_sockaddr_to_str(GSocketAddress *sock_addr);

gchar*
gvb_network_socket_connection_to_str(GSocketConnection *connection);

gboolean
gvb_network_socket_get_info(GSocket *socket
    , guint32 *snd_rtt, guint32 *rcv_rtt
    , guint32 *snd_mss, guint32 *rcv_mss
    , GError **error );

#endif /* GVB_NETWORK_H */

