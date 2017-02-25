/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-server-opt.h
 * Author: Miha
 *
 * Created on September 10, 2016, 4:45 PM
 */

#ifndef GVB_SERVER_OPT_H
#define GVB_SERVER_OPT_H

#include <glib.h>

typedef struct GvbServerOptions {
    
} GvbServerOptions;

GOptionGroup*
gvb_server_opt_get_group(GvbServerOptions *options);

#endif /* GVB_SERVER_OPT_H */

