#include "gvb-client-opt.h"
#include <glib.h>

static const GOptionEntry gvb_client_entries[] = {
    { NULL }
};

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