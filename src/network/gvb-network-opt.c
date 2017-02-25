/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gvb-network-opt.h"
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <errno.h>
#include <glib-2.0/glib/gerror.h>
#include <glib-2.0/glib/goption.h>
#include <glib-2.0/glib/gmessages.h>
#include "gvb-error.h"
#include "gvb-network.h"
#include "gvb-opt.h"
#include "gvb-log.h"

static gboolean 
gvb_network_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error);

static const GOptionEntry gvb_network_entries[] = {
    { "addr", 'a', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_network_opt_cb, "gvb network address", NULL }
  , { "port", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_network_opt_cb, "gvb network port", NULL }
  , { "mode", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_network_opt_cb, "gvb network mode(\"server\" or \"client\")", NULL }
  , { NULL }
};

static gboolean 
gvb_network_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error)
{
    g_return_val_if_fail(name!=NULL && strlen(name) > 0, FALSE);
    GvbNetworkOptions *options = (GvbNetworkOptions*)data;
    const gchar *clean_name = gvb_opt_find_long_name(G_N_ELEMENTS(gvb_network_entries), gvb_network_entries, name);
    
#define RAISE_IF_DUP(a ,b)                                          \
    if((a)!=(b)) {                                                  \
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED    \
            , "duplicate option: [%s]", clean_name);                \
        return FALSE;                                               \
    }
    
    if(g_strcmp0(clean_name, "addr")==0) {
        RAISE_IF_DUP(options->addr, NULL);
        options->addr = g_strdup(value);
    }
    else if(g_strcmp0(clean_name, "port")==0) {
        RAISE_IF_DUP(options->port, 0);
        gchar *endptr = NULL;
        options->port = (guint16)g_ascii_strtoull(value, &endptr, 10);
        if(errno<0 || endptr==value) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE
                    , "string conversion fails [%s] %s"
                    , value, strerror(errno));
        }
    }
    else if(g_strcmp0(clean_name, "mode")==0) {
        RAISE_IF_DUP(options->mode, GVB_NETWORK_MODE_INVALD);
        GEnumClass *network_mode_class = g_type_class_ref(GVB_TYPE_NETWORK_MODE);
        GEnumValue *network_mode = g_enum_get_value_by_nick(network_mode_class, value);
        g_type_class_unref(network_mode_class);
        if(!network_mode) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE
                    , "unknown network mode [%s]", value);
        }
        else {
            options->mode = network_mode->value;
        }
    }
    else {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED
                , "bug - unhandled option [%s:%d][%s:%s]", __FILE__, __LINE__, name, clean_name);
    }
    
#undef RAISE_IF_DUP
    
    return (*error==NULL);
}

GOptionGroup*
gvb_network_opt_get_group(GvbNetworkOptions *options)
{
    g_return_val_if_fail(options!=NULL,NULL);
    GOptionGroup *gvb_network_group = g_option_group_new(
        "gvb-network"
      , "Glavbot Network Options"
      , "Show Glavbot Network options"
      , (gpointer)options
      , NULL
      );
    g_option_group_add_entries(gvb_network_group, gvb_network_entries);
    return gvb_network_group;
}

GInetSocketAddress*
gvb_network_opt_get_inet_socket_addr(GvbNetworkOptions *options)
{
    g_return_val_if_fail(options!=NULL, NULL);
    g_autoptr(GInetAddress) iaadr = g_inet_address_new_from_string(options->addr);
    GInetSocketAddress* saddr = (GInetSocketAddress*)g_inet_socket_address_new(iaadr, options->port);
    if(!saddr) {
        gvb_log_warning("fail to create inet socket address for [%s:%d]", options->addr, options->port);
    }
    return saddr;
}
