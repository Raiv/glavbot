/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <string.h>

#include "gvb-camera-opt.h"
#include "gvb-error.h"
#include "gvb-log.h"
#include "gvb-opt.h"

static gboolean
gvb_camera_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error);

static const GOptionEntry gvb_camera_entries[] = {
    { "dev-path", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_camera_opt_cb, "gvb camera device path", NULL }
  , { "out-path", 'o', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_camera_opt_cb, "out file path", NULL }
  , { "width", 'w', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_camera_opt_cb, "capture width", NULL }
  , { "height", 'g', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, (gpointer)gvb_camera_opt_cb, "capture height", NULL }
  , { NULL }
};

static gboolean
gvb_camera_opt_cb(const gchar *name, const gchar *value, gpointer data, GError **error)
{
    g_return_val_if_fail(name!=NULL && strlen(name) > 0, FALSE);
    GvbCameraOptions *options = (GvbCameraOptions*)data;
    const gchar *clean_name = gvb_opt_find_long_name(G_N_ELEMENTS(gvb_camera_entries), gvb_camera_entries, name);
    
#define RAISE_IF_DUP(a ,b)                                          \
    if((a)!=(b)) {                                                  \
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED    \
            , "duplicate option: [%s]", clean_name);                \
        return FALSE;                                               \
    }
    
    if(g_strcmp0(clean_name, "dev-path")==0) {
        RAISE_IF_DUP(options->dev_path, NULL);
        options->dev_path = g_strdup(value);
    }
    else if(g_strcmp0(clean_name, "out-path")==0) {
        RAISE_IF_DUP(options->out_path, NULL);
        options->out_path = g_strdup(value);
    }
    else if(g_strcmp0(clean_name, "width")==0) {
        RAISE_IF_DUP(options->width, 0);
        gvb_log_message("width = [%d]", options->width);
        options->width = g_ascii_strtoull(value, NULL, 10);
    }
    else if(g_strcmp0(clean_name, "height")==0) {
        RAISE_IF_DUP(options->height, 0);
        options->height = g_ascii_strtoull(value, NULL, 10);
    }
    else {
        g_set_error(error, GVB_ERROR, GVB_ERROR_OTHER, "bug - unhandled option [%s:%d][%s:%s]", __FILE__, __LINE__, name, clean_name);
        return FALSE;
    }
    
#undef RAISE_IF_DUP
    
    return TRUE;
}

GOptionGroup*
gvb_camera_opt_get_group(GvbCameraOptions *options)
{
    g_return_val_if_fail(options!=NULL,NULL);
    GOptionGroup *gvb_camera_group = g_option_group_new(
        "gvb-camera"
      , "Glavbot Camera Options"
      , "Show Glavbot Camera options"
      , (gpointer)options
      , NULL
      );
    g_option_group_add_entries(gvb_camera_group, gvb_camera_entries);
    return gvb_camera_group;
}