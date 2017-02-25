/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-camera-opt.h
 * Author: Miha
 *
 * Created on September 3, 2016, 2:40 PM
 */

#ifndef GVB_CAMERA_OPT_H
#define GVB_CAMERA_OPT_H

#include <glib.h>

typedef struct GvbCameraOptions {
    gchar    *dev_path;
    guint32   width;
    guint32   height;
    gchar    *out_path;
} GvbCameraOptions;

GOptionGroup*
gvb_camera_opt_get_group(GvbCameraOptions *options);

#endif /* GVB_CAMERA_OPT_H */

