/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-log.h
 * Author: Miha
 *
 * Created on September 5, 2016, 4:27 PM
 */

#ifndef GVB_LOG_H
#define GVB_LOG_H

#include <glib.h>

#define GVB_LOG_DOMAIN "gvb-log"

#define gvb_log_fatal(...)                                      \
    G_STMT_START {                                              \
        g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_ERROR, __VA_ARGS__);  \
        for (;;) ;                                              \
    } G_STMT_END
#define gvb_log_error(error)                                    \
    G_STMT_START {                                              \
        if G_LIKELY(error && *error) {                          \
            g_log(g_quark_to_string((*error)->domain)           \
                , G_LOG_LEVEL_ERROR, "(%d) %s"                  \
                , (*error)->code, (*error)->message);           \
            g_clear_error(error);                               \
        }                                                       \
        else {                                                  \
            g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_ERROR             \
                , "error with no object");                      \
        }                                                       \
    } G_STMT_END
#define gvb_log_critical(...) g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define gvb_log_warning(...)  g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_WARNING, __VA_ARGS__)
#define gvb_log_message(...)  g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define gvb_log_info(...)     g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_INFO, __VA_ARGS__)
#define gvb_log_debug(...)    g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)

#endif /* GVB_LOG_H */

