/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-ui.h
 * Author: Miha
 *
 * Created on September 4, 2016, 2:06 PM
 */

#ifndef GVB_UI_H
#define GVB_UI_H

#include <glib.h>
#include "gvb-camera.h"
#include "gvb-server.h"
#include "gvb-client.h"

gboolean
gvb_ui_start(GvbCamera *camera, GvbServer *server, GvbClient *client, GError **error);

gboolean
gvb_ui_stop(GError **error);

#endif /* GVB_UI_H */

