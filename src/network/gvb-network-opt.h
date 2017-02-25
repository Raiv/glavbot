/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-network-opt.h
 * Author: Miha
 *
 * Created on September 10, 2016, 5:02 PM
 */

#ifndef GVB_NETWORK_OPT_H
#define GVB_NETWORK_OPT_H

#include <glib.h>
#include "gvb-network.h"

typedef struct GvbNetworkOptions {
    gchar          *addr;
    guint16         port;
    GvbNetworkMode  mode;
    guint32         rtt_from;
    guint32         rtt_to;
} GvbNetworkOptions;

GOptionGroup*
gvb_network_opt_get_group(GvbNetworkOptions *options);
GInetSocketAddress*
gvb_network_opt_get_inet_socket_addr(GvbNetworkOptions *options);

#endif /* GVB_NETWORK_OPT_H */

