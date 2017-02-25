#ifndef GVB_LOG_H
#define GVB_LOG_H

#include <glib.h>

#define GVB_LOG_DOMAIN "gvb-log"

#define gvb_log_fatal(...)                                      \
    G_STMT_START {                                              \
        g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_ERROR, __VA_ARGS__);  \
        for (;;) ;                                              \
    } G_STMT_END

static inline void 
gvb_log_error_l(GError **error, GLogLevelFlags level)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
        if(G_LIKELY((error) && *(error))) {
#pragma GCC diagnostic pop
            g_log(g_quark_to_string((*error)->domain)
                , level, "%s:%d (%d) %s"
                ,__FILE__, __LINE__
                ,  (*error)->code, (*error)->message);
            g_clear_error(error);
        }
        else {
            g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_ERROR
                , "error with no object");
        }
}

#define gvb_log_error(error)                                    \
    gvb_log_error_l(error, G_LOG_LEVEL_CRITICAL)

#define gvb_log_critical(...) g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define gvb_log_warning(...)  g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_WARNING, __VA_ARGS__)
#define gvb_log_message(...)  g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define gvb_log_info(...)     g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_INFO, __VA_ARGS__)
#define gvb_log_debug(...)    g_log(GVB_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)

#endif /* GVB_LOG_H */

