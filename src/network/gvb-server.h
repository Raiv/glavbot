/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-server.h
 * Author: Miha
 *
 * Created on September 10, 2016, 3:58 PM
 */

#ifndef GVB_SERVER_H
#define GVB_SERVER_H

#include <glib.h>
#include <glib-object.h>
#include "gvb-server-opt.h"
#include "gvb-network-opt.h"

/**
 * GOBJECT defines
 ******************************************************************************/

#define GVB_TYPE_SERVER             (gvb_server_get_type ())
#define GVB_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVB_TYPE_SERVER, GvbServer))
#define GVB_IS_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVB_TYPE_SERVER))
#define GVB_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GVB_TYPE_SERVER, GvbServerClass))
#define GVB_IS_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GVB_TYPE_SERVER))
#define GVB_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVB_TYPE_SERVER, GvbServerClass))

/**
 * TYPEDEFs
 ******************************************************************************/
    
// Структуры класса
typedef struct _GvbServerClass GvbServerClass;
typedef struct _GvbServer GvbServer;
typedef struct _GvbServerPrivate GvbServerPrivate;

typedef void (*GvbInputDataCb)(gchar *data, gsize size, gpointer user_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GvbServer, g_object_unref)

GvbServer*
gvb_server_new(GvbNetworkOptions *nwkopts, GvbServerOptions *options, GMainContext *main_ctx, GError **error);

gboolean
gvb_server_set_input_data_cb(GvbServer *self, GvbInputDataCb callback, GError **error);

gboolean
gvb_server_get_rtt(GvbServer *self, guint32 *snd_rtt, guint32 *rcv_rtt, GError **error);

#endif /* GVB_SERVER_H */

