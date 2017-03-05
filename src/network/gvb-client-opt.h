/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-client-opt.h
 * Author: Miha
 *
 * Created on September 10, 2016, 4:45 PM
 */

#ifndef GVB_CLIENT_OPT_H
#define GVB_CLIENT_OPT_H

#include <glib.h>

typedef struct GvbClientOptions {
    gboolean log_timing;
    gboolean log_parser;
} GvbClientOptions;

GOptionGroup*
gvb_client_opt_get_group(GvbClientOptions *options);

#endif /* GVB_CLIENT_OPT_H */

