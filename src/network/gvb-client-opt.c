/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gvb-client-opt.h"
#include <glib.h>

//static gboolean 
//gvb_client_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error);

static const GOptionEntry gvb_client_entries[] = {
    { NULL }
};

//static gboolean 
//gvb_client_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error)
//{
//    GvbClientOptions *options = (GvbClientOptions*)data;
//    return (*error==NULL);
//}

GOptionGroup*
gvb_client_opt_get_group(GvbClientOptions *options)
{
    g_return_val_if_fail(options!=NULL,NULL);
    GOptionGroup *gvb_client_group = g_option_group_new(
        "gvb_client"
      , "Glavbot Client Options"
      , "Show Glavbot Client options"
      , (gpointer)options
      , NULL
      );
    g_option_group_add_entries(gvb_client_group, gvb_client_entries);
    return gvb_client_group;
}