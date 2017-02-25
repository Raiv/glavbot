/* 
 * File:   main.c
 * Author: Miha
 *
 * Created on August 28, 2016, 6:22 PM
 */

#include <stdio.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <ncurses.h>
#include <stdlib.h>
#include "gvb-camera.h"
#include "gvb-camera-opt.h"
#include "gvb-log.h"
#include "gvb-network-opt.h"
#include "gvb-client-opt.h"
#include "gvb-server-opt.h"
#include "gvb-server.h"
#include "gvb-client.h"
#include "gvb-ui.h"

static GvbNetworkOptions  nwk_opts = {0};
static GOptionContext    *opt_ctx = NULL;
static GMainLoop         *loop = NULL;
static GvbServer         *server = NULL;
static GvbClient         *client = NULL;
static GvbCamera         *camera = NULL;

/*
static
gboolean sigint_handler(gpointer user_data)
{
    GvbCamera* camera = GVB_CAMERA(user_data);
    gvb_camera_stop_capturing(camera, NULL);
    g_main_loop_quit(loop);
    gvb_ui_stop(NULL);
    gvb_log_message("SIGINT handler");
    return TRUE;
}

static int 
camera_ui_test(int argc, char** argv)
{
    // TODO: free after usage
    GvbCameraOptions options = {0};
    g_autoptr(GError) error = NULL;
    g_autoptr(GOptionContext) context = g_option_context_new("- Glavbot Camera application");
    g_autoptr(GOptionGroup) optgroup = gvb_camera_opt_get_group(&options);
    g_autofree gchar *str;
    
    g_option_context_add_group(context, optgroup);
    
    if(!g_option_context_parse(context, &argc, &argv, &error)) {
        gvb_log_error(&error);
        goto fail;
    }
    
    if(!gvb_ui_start(&camera, &server, &client, &error)) {
        gvb_log_error(&error);
        return 1;
    }
    
    gvb_log_message("device path = [%s]", options.dev_path);
    gvb_log_message("output path = [%s]", options.out_path);
    camera = gvb_camera_new(&options);
    gvb_camera_get_dev_path(camera, &str);
    gvb_log_message("camera instance [%p], dev_path [%s]", camera, str);
    
    if(!gvb_camera_start_capturing(camera, &error)) {
        gvb_log_error(&error);
        goto fail;
    }

    g_unix_signal_add(SIGINT, sigint_handler, camera);
    
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    
    return 0;
    
fail:
    
    if(!gvb_ui_stop(&error)) {
        gvb_log_error(&error);
        return 1;
    }
}

static void 
camera_cb(gpointer start, guint32 bytes)
{
    GError *error = NULL;
    GIOStream *io_stream = gvb_client_get_io_stream(client, NULL, &error);
    if(!io_stream) {
        gvb_log_error(&error);
        return;
    }
    GOutputStream *out_stream = g_io_stream_get_output_stream(io_stream);   // no need to unref
    gssize wsize = -1;
    if((wsize=g_output_stream_write(out_stream, start, bytes, NULL, &error))<0) {
        gvb_log_error(&error);
    }
}

static int
network_test(int argc, char** argv)
{
    GvbCameraOptions camera_opts = {0};
    GvbNetworkOptions network_opts = {0};
    GvbClientOptions client_opts;
    GvbServerOptions server_opts;
    //
    g_autoptr(GError) error = NULL;
    g_autoptr(GOptionContext) context = g_option_context_new("- Glavbot Camera application");
    g_autoptr(GOptionGroup) camera_optgroup = gvb_camera_opt_get_group(&camera_opts);
    g_autoptr(GOptionGroup) network_optgroup = gvb_network_opt_get_group(&network_opts);
    g_autoptr(GOptionGroup) nwkmode_optgroup;
    
    g_option_context_add_group(context, camera_optgroup);
    g_option_context_add_group(context, network_optgroup);
    
    // 1-st pass client or server?
    if(!g_option_context_parse(context, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return 1;
    }
    
    g_option_context_add_group(
            context
          , nwkmode_optgroup = (
                network_opts.mode==GVB_NETWORK_MODE_CLIENT
              ? gvb_client_opt_get_group(&client_opts)
              : gvb_server_opt_get_group(&server_opts)
            ));
    
    // 2-nd pass with client/server options parse
    if(!g_option_context_parse(context, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return 1;
    }
    
    switch(network_opts.mode) {
        case GVB_NETWORK_MODE_SERVER:
            gvb_log_message("Server will be started");
            if(!(server = gvb_server_new(&network_opts, &server_opts, NULL, &error))) {
                gvb_log_error(&error);
                return 1;
            }
            break;
        case GVB_NETWORK_MODE_CLIENT:
            gvb_log_message("Client will be started");
            if(!(client = gvb_client_new(&network_opts, &client_opts, &error))) {
                gvb_log_error(&error);
                return 1;
            }
            if(!gvb_client_attach(client, NULL, NULL, &error)) {
                gvb_log_error(&error);
                return 1;
            }
            // start camera capturing on client
            camera = gvb_camera_new(&camera_opts);
            gvb_camera_set_buffer_callback(camera, camera_cb);
            if(!gvb_camera_start_capturing(camera, &error)) {
                gvb_log_error(&error);
                return 1;
            }
            break;
    }
    
    g_unix_signal_add(SIGINT, sigint_handler, NULL);
    
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}
*/
//------------------------------------------------------------------------------

static
gboolean sigint_handler(gpointer user_data)
{
    (void*)user_data;
    if(loop) {
        g_main_loop_quit(loop);
    }
    return TRUE;
}

static void 
client_camera_cb(gpointer user_data, gpointer start, guint32 bytes)
{
    GError *error = NULL;
    GvbClient *client = GVB_CLIENT(user_data);
    
    GOutputStream *ostream = NULL;
    if(!gvb_client_get_output_stream(client, &ostream, &error)) {
        gvb_log_error(&error);
        return;
    }
    
    gssize wsize = -1;
    if((wsize=g_output_stream_write(ostream, start, bytes, NULL, &error))<0) {
        gvb_log_error(&error);
        return;
    }
    
    guint32 rcv_rtt=0, snd_rtt=0;
    if(!gvb_client_get_rtt(client, &snd_rtt, &rcv_rtt, &error)) {
        gvb_log_error(&error);
    }
    else {
        gvb_log_message("[rcv:%d] - [snd:%d]", rcv_rtt, snd_rtt);
    }
}

static void 
client_camera_destroy_cb(gpointer data)
{
    g_object_unref(GVB_CLIENT(data));
}

static int
client_test(int argc, char** argv)
{
    static GvbClientOptions cli_opts = {};
    static GvbCameraOptions cam_opts = {0};
    
    g_option_context_add_group(opt_ctx, gvb_client_opt_get_group(&cli_opts));
    g_option_context_add_group(opt_ctx, gvb_camera_opt_get_group(&cam_opts));
    
    GError *error = NULL;
    if(!g_option_context_parse(opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GMainContext) main_ctx = NULL; //g_main_context_new();
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    loop = g_main_loop_new(main_ctx, TRUE);
    
    g_autoptr(GvbClient) client = gvb_client_new(&nwk_opts, &cli_opts, main_ctx, &error);
    if(!client) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    if(!gvb_client_connect(client, cancellable, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GvbCamera) camera = gvb_camera_new(&cam_opts);
    if(!gvb_camera_set_buffer_callback(camera, client_camera_cb, g_object_ref(client), client_camera_destroy_cb)) {
        gvb_log_critical("gvb_camera_set_buffer_callback fails");
        return EXIT_FAILURE;
    }
    
    if(!gvb_camera_start_capturing(camera, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
//    if(!gvb_ui_start(camera, NULL, client, &error)) {
//        gvb_log_error(&error);
//        return EXIT_FAILURE;
//    }
    
    g_main_loop_run(loop);
    return EXIT_SUCCESS;
}

static int
server_test(int argc, char** argv)
{
    static GvbServerOptions srv_opts = {};
    
    g_option_context_add_group(opt_ctx, gvb_server_opt_get_group(&srv_opts));
    
    GError *error = NULL;
    if(!g_option_context_parse(opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GMainContext) main_ctx = NULL; //g_main_context_new();
    //g_autoptr(GCancellable) cancellable = g_cancellable_new();
    loop = g_main_loop_new(main_ctx, TRUE);
    
    g_autoptr(GvbServer) server = gvb_server_new(&nwk_opts, &srv_opts, main_ctx, &error);
    if(!server) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
//    if(!gvb_ui_start(NULL, server, NULL, &error)) {
//        gvb_log_error(&error);
//        return EXIT_FAILURE;
//    }
    
    g_main_loop_run(loop);
    return EXIT_SUCCESS;
}

static int
camera_and_network_test(int argc, char** argv)
{
    opt_ctx = g_option_context_new("- Glavbot Camera application");
    g_option_context_add_group(opt_ctx, gvb_network_opt_get_group(&nwk_opts));  // option group will be freed with context
    g_option_context_set_ignore_unknown_options(opt_ctx, TRUE);
    
    GError *error = NULL;
    if(!g_option_context_parse(opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_unix_signal_add(SIGINT, sigint_handler, NULL);
    
    gint retval = EXIT_FAILURE;
    switch(nwk_opts.mode) {
        case GVB_NETWORK_MODE_CLIENT:
            retval = client_test(argc, argv);
            break;
        case GVB_NETWORK_MODE_SERVER:
            retval = server_test(argc, argv);
            break;
    }
    
    g_option_context_free(opt_ctx);
    opt_ctx = NULL;
    
    return retval;
}

int 
main(int argc, char** argv)
{
    return camera_and_network_test(argc, argv);
}

