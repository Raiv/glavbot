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

static inline
void gvb_error_stacked(GError **err0pp, GError **err1pp)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
    if((err1pp) && *(err1pp)) {
#pragma GCC diagnostic pop
        g_prefix_error(err0pp, "[%s:%d] %s"
          , g_quark_to_string((*(err1pp))->domain)
          , (*(err1pp))->code
          , (*(err1pp))->message
        );
    }
    g_clear_error(err1pp);
}

//#define gvb_error_set1(error_pp,message,domain,errnum...)                   
//    if(error_pp) {                                                          
//        if(*(error_pp)) {                                                   
//            g_prefix_error(error_pp, message "\n", __VA_ARGS__);            
//        }                                                                   
//        else {                                                             
//            g_set_error(error_pp, domain, errnum, message, __VA_ARGS__);    
//        }                                                                   
//    }
//
//#define gvb_error_set2(error_pp,message,...) 
//    gvb_error_set1(error_pp, message, GVB_ERROR, GVB_ERROR_OTHER, __VA_ARGS__);
//
//#define gvb_error_return_ifnull(ptr, error_pp)                                              
//    if(!ptr) {                                                                              
//        gvb_error_set1(error_pp, #ptr " is null", GVB_ERROR, GVB_ERROR_INVALID_ARG);        
//        return;                                                                             
//    }
//
//#define gvb_error_return_val_ifnull(ptr, error_pp, value)                                   
//    if(!ptr) {                                                                              
//        gvb_error_set1(error_pp, #ptr " is null", GVB_ERROR, GVB_ERROR_INVALID_ARG);        
//        return value;                                                                       
//    }

#endif /* GVB_ERROR_H */

