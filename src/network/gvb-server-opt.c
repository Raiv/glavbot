#include "gvb-server-opt.h"
#include <glib.h>

static const GOptionEntry gvb_server_entries[] = {
    { NULL }
};

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