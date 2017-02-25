#ifndef GVB_CAMERA_OPT_H
#define GVB_CAMERA_OPT_H

#include <glib.h>

typedef struct GvbCameraOptions {
    gchar    *dev_path;
    guint32   width;
    guint32   height;
    guint32   fps;
    gchar    *out_path;
} GvbCameraOptions;

GOptionGroup*
gvb_camera_opt_get_group(GvbCameraOptions *options);

#endif /* GVB_CAMERA_OPT_H */

