#include "gvb-client-opt.h"
#include "gvb-opt.h"
#include <glib.h>
#include <string.h>

static gboolean 
gvb_client_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error);

static const GOptionEntry gvb_client_entries[] = {
    { "log_timing", 0, G_OPTION_FLAG_NONE | G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (gpointer)gvb_client_opt_cb, "gvb log timing values", NULL }
  , { "log_parser", 0, G_OPTION_FLAG_NONE | G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (gpointer)gvb_client_opt_cb, "gvb log video stream parser", NULL }
  , { NULL }
};

static gboolean 
gvb_client_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error)
{
    g_return_val_if_fail(name!=NULL && strlen(name) > 0, FALSE);
    GvbClientOptions *options = (GvbClientOptions*)data;
    const gchar *clean_name = gvb_opt_find_long_name(G_N_ELEMENTS(gvb_client_entries), gvb_client_entries, name);
    
#define RAISE_IF_DUP(a ,b)                                          \
    if((a)!=(b)) {                                                  \
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED    \
            , "duplicate option: [%s]", clean_name);                \
        return FALSE;                                               \
    }
    
    if(g_strcmp0(clean_name, "log_timing")==0) {
        RAISE_IF_DUP(options->log_timing, FALSE);
        options->log_timing = TRUE;
    }
    else if(g_strcmp0(clean_name, "log_parser")==0) {
        RAISE_IF_DUP(options->log_parser, FALSE);
        options->log_parser = TRUE;
    }
    else {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED
                , "bug - unhandled option [%s:%d][%s:%s]", __FILE__, __LINE__, name, clean_name);
    }
    
#undef RAISE_IF_DUP
    
    return (*error==NULL);
}

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