//#include <linux/netlink.h>
//#include <linux/rtnetlink.h>
//#include <sys/socket.h>
//#include <string.h>
//#include <errno.h>
//#include <unistd.h>
//#include <stdio.h>

//#include "gvb-camera.h"
//#include "gvb-log.h"

#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>

int test1(int argc, char** argv)
{
//    int sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
//    if(sockfd<0) {
//        printf("socket error: %s", strerror(errno));
//        return -1;
//    }
//    
//    struct sockaddr_nl sockaddr;
//    memset(&sockaddr, 0, sizeof(struct sockaddr_nl));
//    sockaddr.nl_family = AF_NETLINK;
//    
//    if(bind(sockfd, &sockaddr, sizeof(struct sockaddr_nl))<0) {
//        printf("bind error: %s", strerror(errno));
//        close(sockfd);
//        return -1;
//    }
    return EXIT_SUCCESS;
}

int test2(int argc, char** argv)
{
//    GError *error = NULL;
//    GvbCameraOptions options;
//    options.dev_path = "/dev/video0";
//    options.fps = 30;
//    options.width = 1024;
//    options.height = 768;
//    options.out_path = NULL;
//    
//    GvbCamera *camera = gvb_camera_new(&options);
//    if(!camera) {
//        return -1;
//    }
//    
//    if(!gvb_camera_open(camera, &error)) {
//        gvb_log_error(&error);
//        return -1;
//    }
//    
//    gint32 min = 0, max = 0, step = 0, deflt = 0;
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("EXPOSURE_TIME: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("FRAME_RATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("BITRATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("INTER_FRAME_PERIOD: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    gvb_camera_close(camera, NULL);
//    return 0;
//    
//ret_err:
//    gvb_camera_close(camera, NULL);
//    return -1;    
//    
//    GError *error = NULL;
//    GvbCameraOptions options;
//    options.dev_path = "/dev/video0";
//    options.fps = 30;
//    options.width = 1024;
//    options.height = 768;
//    options.out_path = NULL;
//    
//    GvbCamera *camera = gvb_camera_new(&options);
//    if(!camera) {
//        return -1;
//    }
//    
//    if(!gvb_camera_open(camera, &error)) {
//        gvb_log_error(&error);
//        return -1;
//    }
//    
//    gint32 min = 0, max = 0, step = 0, deflt = 0;
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("EXPOSURE_TIME: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("FRAME_RATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("BITRATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("INTER_FRAME_PERIOD: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    gvb_camera_close(camera, NULL);
//    return 0;
//    
//ret_err:
//    gvb_camera_close(camera, NULL);
//    return -1;    
//    
//    GError *error = NULL;
//    GvbCameraOptions options;
//    options.dev_path = "/dev/video0";
//    options.fps = 30;
//    options.width = 1024;
//    options.height = 768;
//    options.out_path = NULL;
//    
//    GvbCamera *camera = gvb_camera_new(&options);
//    if(!camera) {
//        return -1;
//    }
//    
//    if(!gvb_camera_open(camera, &error)) {
//        gvb_log_error(&error);
//        return -1;
//    }
//    
//    gint32 min = 0, max = 0, step = 0, deflt = 0;
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("EXPOSURE_TIME: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("FRAME_RATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("BITRATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("INTER_FRAME_PERIOD: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    gvb_camera_close(camera, NULL);
//    return 0;
//    
//ret_err:
//    gvb_camera_close(camera, NULL);
//    return -1;    
//    
//    GError *error = NULL;
//    GvbCameraOptions options;
//    options.dev_path = "/dev/video0";
//    options.fps = 30;
//    options.width = 1024;
//    options.height = 768;
//    options.out_path = NULL;
//    
//    GvbCamera *camera = gvb_camera_new(&options);
//    if(!camera) {
//        return -1;
//    }
//    
//    if(!gvb_camera_open(camera, &error)) {
//        gvb_log_error(&error);
//        return -1;
//    }
//    
//    gint32 min = 0, max = 0, step = 0, deflt = 0;
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("EXPOSURE_TIME: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("FRAME_RATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("BITRATE: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
//            , &min, &max, &step, &deflt, &error)) {
//        gvb_log_error(&error);
//        goto ret_err;
//    }
//    gvb_log_message("INTER_FRAME_PERIOD: min=[%d], max=[%d], step=[%d], deflt=[%d]", min, max, step, deflt);
//    
//    gvb_camera_close(camera, NULL);
//    return 0;
//    
//ret_err:
//    gvb_camera_close(camera, NULL);
//    return -1;
    return EXIT_SUCCESS;
}

int test3(int argc, char** argv)
{
    GInputStream *base_stream = g_memory_input_stream_new();
    GDataInputStream *data_stream = g_data_input_stream_new(G_INPUT_STREAM(base_stream));
    
    g_print("data_stream size: %zu\n", g_buffered_input_stream_get_available(G_BUFFERED_INPUT_STREAM(data_stream)));
    gchar data[10] = {1,2,3,4,5,6,7,8,9,0};
    g_memory_input_stream_add_data(G_MEMORY_INPUT_STREAM(base_stream), data, 10, NULL);
    
    gsize i = 0;
    gsize cnt = 0;
    const gchar* buf = g_buffered_input_stream_peek_buffer(G_BUFFERED_INPUT_STREAM(data_stream), &cnt);
    for(i=0; i<cnt; i++) {
        g_print("%d", buf[i]);
    }
    
    return EXIT_SUCCESS;
}

int gvb_misc_test(int argc, char** argv)
{
    return test3(argc, argv);
}