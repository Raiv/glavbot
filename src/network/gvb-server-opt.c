/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gvb-server-opt.h"
#include <glib.h>

static gboolean 
gvb_server_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error);

static const GOptionEntry gvb_server_entries[] = {
    { NULL }
};

static gboolean 
gvb_server_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error)
{
    GvbServerOptions *options = (GvbServerOptions*)data;
    return (*error==NULL);
}

GOptionGroup*
gvb_server_opt_get_group(GvbServerOptions *options)
{
    g_return_val_if_fail(options!=NULL,NULL);
    GOptionGroup *gvb_server_group = g_option_group_new(
        "gvb_server"
      , "Glavbot Server Options"
      , "Show Glavbot Server options"
      , (gpointer)options
      , NULL
      );
    g_option_group_add_entries(gvb_server_group, gvb_server_entries);
    return gvb_server_group;
}