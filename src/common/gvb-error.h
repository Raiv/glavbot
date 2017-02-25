/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-error.h
 * Author: Miha
 *
 * Created on September 3, 2016, 3:20 PM
 */

#ifndef GVB_ERROR_H
#define GVB_ERROR_H

#include <glib.h>

#define GVB_ERROR gvb_error_quark()

typedef enum {
    GVB_ERROR_ILLEGAL_STATE,
    GVB_ERROR_INVALID_ARG,
    GVB_ERROR_IOCTL,
    GVB_ERROR_MEMORY,
    GVB_ERROR_NETWORK,
    GVB_ERROR_OTHER
} GvbErrorEnum;

GQuark gvb_error_quark (void);

#endif /* GVB_ERROR_H */

