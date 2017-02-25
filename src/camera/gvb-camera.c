/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gvb-camera.h"
#include "gvb-error.h"
#include "gvb-log.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

struct _GvbCamera {
    GObject parent_instance;
};

struct _GvbCameraClass {
    GObjectClass parent_class;
};

typedef struct {
    gpointer    start;
    gsize       length;
    gboolean    queued;
} CaptureBuffer;

struct _GvbCameraPrivate {
    GRecMutex            lock;
    GvbCameraState       state;
    // device control
    gchar               *dev_path;
    gint                 dev_fd;
    // capture control
    guint32              width;
    guint32              height;
    CaptureBuffer       *capt_bufs;
    guint32              capt_bufs_count;
    GThread             *capt_mill_thread;
    volatile gint        capt_mill_stop;
    GvbCameraBufferCbFn  capt_mill_cb;
    gpointer             capt_mill_cb_data;
    GDestroyNotify       capt_mill_cb_destroy_cb;
    // output
    gchar               *out_path;
    gint                 out_fd;
    // statistics
    GvbCameraStatistics  stats;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvbCamera, gvb_camera, G_TYPE_OBJECT);

#define P(self)       G_TYPE_INSTANCE_GET_PRIVATE(self, GVB_TYPE_CAMERA, GvbCameraPrivate)
#define LOCK(self)    g_rec_mutex_lock(&P(self)->lock)
#define UNLOCK(self)  g_rec_mutex_unlock(&P(self)->lock)

#define SET_IOCTL_ERROR(error, msg)                                             \
    g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, " [%s]", strerror(errno));   \
    g_prefix_error(error, msg);
    

/*
 * GOBJECT functions
 ******************************************************************************/

static void 
gvb_camera_finalize(GObject* object);

static void
gvb_camera_class_init(GvbCameraClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = gvb_camera_finalize;
}

static void
gvb_camera_init(GvbCamera *self)
{
    g_rec_mutex_init(&P(self)->lock);
    P(self)->state = GVB_CAMERA_STATE_IDLE;
    P(self)->dev_path = NULL;
    P(self)->dev_fd = -1;
    P(self)->width = 0;
    P(self)->height = 0;
    P(self)->capt_bufs = NULL;
    P(self)->capt_bufs_count = 0;
    P(self)->capt_mill_thread = NULL;
    g_atomic_int_set(&P(self)->capt_mill_stop, 0);
    P(self)->capt_mill_cb = NULL;
    P(self)->capt_mill_cb_data = NULL;
    P(self)->capt_mill_cb_destroy_cb = NULL;
    P(self)->out_path = NULL;
    P(self)->out_fd = -1;
    memset(&P(self)->stats, 0, sizeof(GvbCameraStatistics));
}

static void 
gvb_camera_finalize(GObject* object)
{
    GvbCamera *self = GVB_CAMERA(object);
    g_autoptr(GError) error = NULL;
    if(!gvb_camera_stop_capturing(self, &error)) {
        gvb_log_error(&error);
    }
    if(P(self)->capt_mill_cb_destroy_cb) {
        (P(self)->capt_mill_cb_destroy_cb)(P(self)->capt_mill_cb_data);
    }
    g_rec_mutex_clear(&P(self)->lock);
    g_free(P(self)->dev_path);
    g_free(P(self)->out_path);
}

/*
 * PUBLIC functions
 ******************************************************************************/

GvbCamera*
gvb_camera_new(GvbCameraOptions *options)
{
    g_return_val_if_fail(options!=NULL, NULL);
    g_return_val_if_fail(options->dev_path!=NULL, NULL);
    GvbCamera *self = g_object_new(GVB_TYPE_CAMERA, NULL);
    P(self)->dev_path = g_strdup(options->dev_path);
    P(self)->out_path = g_strdup(options->out_path);
    if(options->width>0 && options->height>0) {
        P(self)->width = options->width;
        P(self)->height = options->height;
    }
    else {
        P(self)->width = 0;
        P(self)->height = 0;
    }
    return self;
}

gboolean
gvb_camera_get_dev_path(GvbCamera *self, gchar **path)
{
    g_return_val_if_fail(self!=NULL,FALSE);
    g_return_val_if_fail(path!=NULL,FALSE);
    LOCK(self);
    *path = g_strdup(P(self)->dev_path);
    UNLOCK(self);
    return TRUE;
}

gboolean
gvb_camera_get_state(GvbCamera *self, GvbCameraState *state)
{
    g_return_val_if_fail(self!=NULL,FALSE);
    g_return_val_if_fail(state!=NULL,FALSE);
    LOCK(self);
    *state = P(self)->state;
    UNLOCK(self);
    return TRUE;
}

gboolean
gvb_camera_set_buffer_callback(GvbCamera *self, GvbCameraBufferCbFn callback, gpointer user_data, GDestroyNotify destroy_cb)
{
    g_return_val_if_fail(self!=NULL,FALSE);
    LOCK(self);
    P(self)->capt_mill_cb = callback;
    P(self)->capt_mill_cb_data = user_data;
    P(self)->capt_mill_cb_destroy_cb = destroy_cb;
    UNLOCK(self);
    return TRUE;
}

//TODO: при пустом value сбрасывать в значение по умолчанию. Значение по умолчанию брать из QUERY
//      также и в целом по control-ам, уйти от хардкода
gboolean
gvb_camera_set_control(GvbCamera *self, GError **error, GvbCameraControl control, gpointer value)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    g_return_val_if_fail(value!=NULL, FALSE);
    
    switch(control) {
        case GVB_CAMERA_CONTROL_FRAME_RATE: {
            GvbCameraFrameRate *frame_rate = (GvbCameraFrameRate*)value;
            struct v4l2_streamparm parm;
            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            parm.parm.capture.timeperframe.denominator = frame_rate->numerator;
            parm.parm.capture.timeperframe.numerator = frame_rate->denominator;
            if(ioctl(P(self)->dev_fd, VIDIOC_S_PARM, &parm)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_PARM");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_EXPOSURE_TIME: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_EXPOSURE_AUTO;
            ctrl.value = V4L2_EXPOSURE_MANUAL;
            if(ioctl(P(self)->dev_fd, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_EXPOSURE_AUTO)");
                return FALSE;
            }
            ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
            ctrl.value = *((guint32*)value);
            if(ioctl(P(self)->dev_fd, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_EXPOSURE_ABSOLUTE)");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_BITRATE: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
            //драйвер не хочет работать в этом режиме
            //ctrl.value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
            ctrl.value = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR;
            if(ioctl(P(self)->dev_fd, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE_MODE)");
                return FALSE;
            }
            ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            ctrl.value = *((guint32*)value);
            if(ioctl(P(self)->dev_fd, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE)");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
            ctrl.value = *((guint32*)value);
            if(ioctl(P(self)->dev_fd, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_H264_I_PERIOD)");
                return FALSE;
            }
        }
        break;
        default:
            return FALSE;
    }
    
    return TRUE;
}

gboolean
gvb_camera_get_control(GvbCamera *self, GError **error, GvbCameraControl control, gpointer value)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    g_return_val_if_fail(value!=NULL, FALSE);
    
    switch(control) {
        case GVB_CAMERA_CONTROL_FRAME_RATE: {
            struct v4l2_streamparm parm;
            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if(ioctl(P(self)->dev_fd, VIDIOC_G_PARM, &parm)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_PARM");
                return FALSE;
            }
            GvbCameraFrameRate *frame_rate = (GvbCameraFrameRate*)value;
            // frame_rate = 1/time_per_frame
            frame_rate->numerator = parm.parm.capture.timeperframe.denominator;
            frame_rate->denominator = parm.parm.capture.timeperframe.numerator;
        }
        break;
        case GVB_CAMERA_CONTROL_EXPOSURE_TIME: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
            if(ioctl(P(self)->dev_fd, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_CTRL (V4L2_CID_EXPOSURE_ABSOLUTE)");
                return FALSE;
            }
            *((guint32*)value) = ctrl.value;
        }
        break;
        case GVB_CAMERA_CONTROL_BITRATE: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            if(ioctl(P(self)->dev_fd, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE)");
                return FALSE;
            }
            *((guint32*)value) = ctrl.value;
        }
        break;
        case GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
            if(ioctl(P(self)->dev_fd, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_H264_I_PERIOD)");
                return FALSE;
            }
            *((guint32*)value) = ctrl.value;
        }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

gboolean
gvb_camera_get_statistics(GvbCamera *self, GError **error, GvbCameraStatistics *statistics)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    g_return_val_if_fail(statistics!=NULL, FALSE);
    
    LOCK(self);
    memcpy(statistics, &P(self)->stats, sizeof(GvbCameraStatistics));
    UNLOCK(self);
    
    return TRUE;
}

/*
 * PRIVATE functions
 ******************************************************************************/

static gboolean
gvb_camera_init_memory(GvbCamera *self, GError **error);
static gboolean
gvb_camera_free_memory(GvbCamera *self, GError **error);
static gboolean
gvb_camera_turn_streaming_on(GvbCamera *self, GError **error);
static gboolean
gvb_camera_turn_streaming_off(GvbCamera *self, GError **error);
static gboolean
gvb_camera_initial_buf_queue(GvbCamera *self, GError **error);
static gboolean
gvb_camera_subscribe_events(GvbCamera *self, GError **error);
static gpointer
gvb_camera_capt_mill_fn(gpointer user_data);

gboolean
gvb_camera_start_capturing(GvbCamera *self, GError **error)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    
    LOCK(self);
    
    if(P(self)->state==GVB_CAMERA_STATE_CAPTURING) {
        UNLOCK(self);
        return TRUE;
    }
    if(!P(self)->dev_path) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE, "no dev path");
        goto fail0;
    }
    if(P(self)->dev_fd>=0) {
        g_assert_not_reached();
        g_close(P(self)->dev_fd, NULL);
        P(self)->dev_fd = -1;
    }
    // open device file
    if((P(self)->dev_fd = g_open(P(self)->dev_path, O_RDWR))<0) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, strerror(errno));
        goto fail0;
    }
    // open output file
    if(P(self)->out_path) 
    {
        if((P(self)->out_fd = g_open(P(self)->out_path, O_RDWR | O_CREAT, S_IRWXU, S_IRGRP, S_IROTH))<0) {
            g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, strerror(errno));
            goto fail1;
        }
    }
    
    // initialize current format
    struct v4l2_format format;
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(P(self)->dev_fd, VIDIOC_G_FMT, &format)<0) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_G_FMT [%s]", strerror(errno));
        goto fail1;
    }
    // set format
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    if(P(self)->width>0 && P(self)->height>0) {
        format.fmt.pix.width = P(self)->width;
        format.fmt.pix.height = P(self)->height;
    }
    gvb_log_message("default format: [%dx%d]", format.fmt.pix.width, format.fmt.pix.height);
    if(ioctl(P(self)->dev_fd, VIDIOC_S_FMT, &format)<0) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_S_FMT [%s]", strerror(errno));
        goto fail1;
    }
    
    // framerate
    GvbCameraFrameRate frame_rate;
    frame_rate.numerator = 30000;
    frame_rate.denominator = 1000;
    if(!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_FRAME_RATE, &frame_rate)) {
        goto fail1;
    }
    gvb_log_message("default framerate: [%d/%d]", frame_rate.numerator, frame_rate.denominator);
    
    // exposure time
    guint32 exp_time = 1000;
    if(!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &exp_time)) {
        goto fail1;
    }
    gvb_log_message("default exposure time: [%d]", exp_time);
    
    // bitrate
    guint32 bitrate = 10000000;
    if(!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_BITRATE, &bitrate)) {
        goto fail1;
    }
    gvb_log_message("default bitrate: [%d]", bitrate);
    
    // inter frame period
    guint32 iframe_period = 60;
    if(!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &iframe_period)) {
        goto fail1;
    }
    gvb_log_message("default inter frame period: [%d]", iframe_period);
    
    // initialize memory
    if(!gvb_camera_init_memory(self, error)) {
        goto fail1;
    }
    // events subscription
    if(!gvb_camera_subscribe_events(self, error)) {
        goto fail2;
    }
    // here we start buffers handling
    if(!gvb_camera_turn_streaming_on(self, error)) {
        goto fail3;
    }
    P(self)->state = GVB_CAMERA_STATE_CAPTURING;
    UNLOCK(self);
    return TRUE;
fail3:
    gvb_camera_turn_streaming_off(self, error);
fail2:
    gvb_camera_free_memory(self, error);
fail1:
    g_close(P(self)->dev_fd,NULL);
    P(self)->dev_fd = -1;
fail0:
    UNLOCK(self);
    return FALSE;
}

gboolean
gvb_camera_stop_capturing(GvbCamera *self, GError **error)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    
    LOCK(self);
    
    if(P(self)->state==GVB_CAMERA_STATE_IDLE
        || P(self)->dev_fd<0 ) 
    {
        UNLOCK(self);
        return TRUE;
    }
    
    if(!gvb_camera_turn_streaming_off(self, error)) {
        goto fail0;
    }
    
    if(!gvb_camera_free_memory(self, error)) {
        goto fail0;
    }
    
    if(!g_close(P(self)->dev_fd, error)) {
        goto fail0;
    }
    P(self)->dev_fd = -1;
    
    if(!g_close(P(self)->out_fd, error)) {
        goto fail0;
    }
    P(self)->out_fd = -1;
    
    P(self)->state = GVB_CAMERA_STATE_CAPTURING;
    UNLOCK(self);
    return TRUE;
fail0:
    UNLOCK(self);
    return FALSE;
}

static gboolean
gvb_camera_init_memory(GvbCamera *self, GError **error)
{
    // request driver to allocate buffers
    struct v4l2_requestbuffers v4l2_reqbufs;
    v4l2_reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbufs.memory = V4L2_MEMORY_MMAP;
    v4l2_reqbufs.count = 50;
    if(ioctl(P(self)->dev_fd, VIDIOC_REQBUFS, &v4l2_reqbufs)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_REQBUFS");
        goto fail0;
    }
    //gvb_log_message("%s: allocated [%d] buffers", __func__, v4l2_reqbufs.count);
    
    if(P(self)->capt_bufs) {
        g_assert_not_reached();
        g_slice_free1((P(self)->capt_bufs_count)*sizeof(CaptureBuffer), P(self)->capt_bufs);
        P(self)->capt_bufs = NULL;
        P(self)->capt_bufs_count = 0;
    }

    P(self)->capt_bufs_count = v4l2_reqbufs.count;
    if(!(P(self)->capt_bufs = g_slice_alloc0(v4l2_reqbufs.count*sizeof(CaptureBuffer)))) {
        P(self)->capt_bufs_count = 0;
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_MEMORY, "fail to allocate memory");
        goto fail0;
    }
    
    // map buffers to userspace
    gint idx;
    struct v4l2_buffer v4l2_buf;
    CaptureBuffer *capt_buf;
    for(idx=0; idx<P(self)->capt_bufs_count; idx++) 
    {
        // get allocated buffer info
        v4l2_buf.type = v4l2_reqbufs.type;
        v4l2_buf.memory = v4l2_reqbufs.memory;
        v4l2_buf.index = idx;
        if(ioctl(P(self)->dev_fd, VIDIOC_QUERYBUF, &v4l2_buf)<0) {
            SET_IOCTL_ERROR(error, "VIDIOC_QUERYBUF");
            goto fail0;
        }
        
        // map buffers to userspace
        capt_buf = P(self)->capt_bufs+idx;
        capt_buf->queued = FALSE;
        capt_buf->length = v4l2_buf.length;
        capt_buf->start = mmap(NULL, v4l2_buf.length
                , PROT_READ|PROT_WRITE, MAP_SHARED
                , P(self)->dev_fd, v4l2_buf.m.offset
                );
        if(capt_buf->start==MAP_FAILED) {
            g_set_error(error, GVB_ERROR, GVB_ERROR_MEMORY, "mmap failed [%s]", strerror(errno));
            goto fail0;
        }
        //gvb_log_message("buffer mapped: [%p]", capt_buf->start);
    }
    return TRUE;
fail0:
    gvb_camera_free_memory(self, error);
    return FALSE;
}

static gboolean
gvb_camera_free_memory(GvbCamera *self, GError **error)
{
    // clean userspace memory
    gint idx;
    CaptureBuffer *capt_buf;
    if(P(self)->capt_bufs) {
        for(idx=0; idx<P(self)->capt_bufs_count; idx++) {
            capt_buf = P(self)->capt_bufs+idx;
            if(capt_buf->start) {
                if(munmap(capt_buf->start, capt_buf->length)<0) {
                    if(error && *error) {
                        g_prefix_error(error, "unmap failed [%s]\n", strerror(errno));
                    }
                    else {
                        g_set_error(error, GVB_ERROR, GVB_ERROR_MEMORY, "unmap failed [%s]", strerror(errno));
                    }
                }
                capt_buf->start = NULL;
                capt_buf->length = 0;
                capt_buf->queued = FALSE;
            }
        }
        g_slice_free1(P(self)->capt_bufs_count*sizeof(CaptureBuffer), P(self)->capt_bufs);
        P(self)->capt_bufs = NULL;
        P(self)->capt_bufs_count = 0;
    }
    // clean driver memory
    struct v4l2_requestbuffers v4l2_reqbufs;
    memset(&v4l2_reqbufs, 0, sizeof(struct v4l2_requestbuffers));
    v4l2_reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbufs.memory = V4L2_MEMORY_MMAP;
    v4l2_reqbufs.count = 0;
    if(ioctl(P(self)->dev_fd, VIDIOC_REQBUFS, &v4l2_reqbufs)<0) {
        if(error && *error) {
            g_prefix_error(error, "VIDIOC_REQBUFS [%s]\n", strerror(errno));
        }
        else {
            g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_REQBUFS [%s]", strerror(errno));
        }
        gvb_log_warning("VIDIOC_REQBUFS error set to zero [%s]", strerror(errno));
    }
    
    if(error && *error) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

static gboolean
gvb_camera_turn_streaming_on(GvbCamera *self, GError **error)
{
    /*
     * Before VIDIOC_STREAMON we must queue buffers. 
     * Otherwise we get "VIDIOC_STREAMON error [Operation not permitted]"
     */
    if(!gvb_camera_initial_buf_queue(self, error)) {
        return FALSE;
    }
    // streaming on
    enum v4l2_buf_type v4l2_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(P(self)->dev_fd, VIDIOC_STREAMON, &v4l2_buf_type)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_STREAMON");
        return FALSE;
    }
    GThread **capt_mill_thread = &P(self)->capt_mill_thread;
    // start worker thread
    if(*capt_mill_thread) {
        g_assert_not_reached();
        g_thread_unref(*capt_mill_thread);
        *capt_mill_thread = NULL;
    }
    // вопрос - как остановить поток, висящий на вызове VIDIOC_DQBUF ???
    g_atomic_int_set(&P(self)->capt_mill_stop, 0);
    *capt_mill_thread = g_thread_new("gvb-camera worker thread", gvb_camera_capt_mill_fn, self);
    if(!*capt_mill_thread) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, "fail to create thread");
        return FALSE;
    }
    return TRUE;
}

static gboolean
gvb_camera_turn_streaming_off(GvbCamera *self, GError **error)
{
    // stop thread
    g_atomic_int_set(&P(self)->capt_mill_stop, 1);
    if(P(self)->capt_mill_thread) {
        UNLOCK(self);
        g_thread_join(P(self)->capt_mill_thread);
        LOCK(self);
        P(self)->capt_mill_thread = NULL;
    }
    // streaming off
    enum v4l2_buf_type v4l2_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(P(self)->dev_fd, VIDIOC_STREAMOFF, &v4l2_buf_type)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_STREAMOFF");
        return FALSE;
    }
    return TRUE;
}

static gboolean
gvb_camera_initial_buf_queue(GvbCamera *self, GError **error)
{
    guint32 idx;
    struct v4l2_buffer v4l2_buf;
    CaptureBuffer *capt_buf;
    for(idx=0; idx<P(self)->capt_bufs_count; idx++) {
        memset(&v4l2_buf,0,sizeof(struct v4l2_buffer));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        capt_buf = P(self)->capt_bufs+idx;
        if(!capt_buf->queued) {
            v4l2_buf.index = idx;
            if(ioctl(P(self)->dev_fd, VIDIOC_QBUF, &v4l2_buf)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_QBUF");
                return FALSE;
            }
            capt_buf->queued = TRUE;
            //gvb_log_message("queued: [%d]", idx);
        }
    }
    return TRUE;
}

static gboolean
gvb_camera_subscribe_events(GvbCamera *self, GError **error)
{
//    struct v4l2_event_subscription subs;
//    subs.type = V4L2_EVENT_CTRL;
//    if(ioctl(PRIVATE(self)->dev_fd, VIDIOC_SUBSCRIBE_EVENT, &subs)<0) {
//        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_SUBSCRIBE_EVENT (V4L2_EVENT_CTRL) [%s]", strerror(errno));
//        return FALSE;
//    }
//    subs.type = V4L2_EVENT_FRAME_SYNC;
//    if(ioctl(PRIVATE(self)->dev_fd, VIDIOC_SUBSCRIBE_EVENT, &subs)<0) {
//        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_SUBSCRIBE_EVENT (V4L2_EVENT_FRAME_SYNC) [%s]", strerror(errno));
//        return FALSE;
//    }
//    subs.type = V4L2_EVENT_SOURCE_CHANGE;
//    if(ioctl(PRIVATE(self)->dev_fd, VIDIOC_SUBSCRIBE_EVENT, &subs)<0) {
//        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_SUBSCRIBE_EVENT (V4L2_EVENT_SOURCE_CHANGE) [%s]", strerror(errno));
//        return FALSE;
//    }
    return TRUE;
}

static gpointer
gvb_camera_capt_mill_fn(gpointer user_data)
{
    GvbCamera *self = GVB_CAMERA(user_data);
    
    gint *dev_fd = NULL;
    gint *out_fd = NULL;
    CaptureBuffer **capt_bufs = NULL;
    guint32 *capt_bufs_count = NULL;
    GvbCameraBufferCbFn cb = NULL;
    gpointer cb_user_data = NULL;
    
    gint idx;
    guint16 print_slow = 0;
    guint32 allbytes = 0;
    CaptureBuffer *capt_buf = NULL;
    struct v4l2_buffer v4l2_buf;
    
#define CHECK_EXIT()                                        \
    if(g_atomic_int_get(&P(self)->capt_mill_stop)) {  \
        g_thread_exit(NULL);                                \
    }
    
    for(;;) {
        CHECK_EXIT();
        LOCK(self);
        dev_fd = &P(self)->dev_fd;
        out_fd = &P(self)->out_fd;
        capt_bufs = &P(self)->capt_bufs;
        capt_bufs_count = &P(self)->capt_bufs_count;
        cb = P(self)->capt_mill_cb;
        cb_user_data = P(self)->capt_mill_cb_data;
        UNLOCK(self);
        
        for(idx=0;idx<*capt_bufs_count;idx++) 
        {
            CHECK_EXIT();
            
            capt_buf = *capt_bufs+idx;
            
            memset(&v4l2_buf,0,sizeof(struct v4l2_buffer));
            v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            v4l2_buf.memory = V4L2_MEMORY_MMAP;
            
            if(!capt_buf->queued) {
                v4l2_buf.index = idx;
                if(ioctl(*dev_fd, VIDIOC_QBUF, &v4l2_buf)<0) {
                    gvb_log_critical("VIDIOC_QBUF [%s]", strerror(errno));
                }
                else {
                    capt_buf->queued = TRUE;
                }
                continue;
            }
            
            if(ioctl(*dev_fd, VIDIOC_DQBUF, &v4l2_buf)<0) {
                gvb_log_critical("VIDIOC_DQBUF [%s]", strerror(errno));
                continue;
            }
            
            capt_buf->queued = FALSE;
            
            if(v4l2_buf.flags & V4L2_BUF_FLAG_ERROR) {
                gvb_log_critical("stream buffer error [%d, %s]", idx, strerror(errno));
            }
            else {
                if(cb) {
                    cb(cb_user_data, capt_buf->start, v4l2_buf.bytesused);
                }
                if(*out_fd>=0) {
                    write(*out_fd, capt_buf->start, v4l2_buf.bytesused);
                }
                allbytes += v4l2_buf.bytesused;
                if(print_slow++%30==0) {
                    gvb_log_info("[%d]\tprocessing buffer [%p]: bytesused=%d \tallbytes=%d \tfield=%d \tsequence=%d \ttimestamp=%d:%d"
                        , print_slow
                        , capt_buf->start, v4l2_buf.bytesused, allbytes, v4l2_buf.field, v4l2_buf.sequence
                        , v4l2_buf.timestamp.tv_sec, v4l2_buf.timestamp.tv_usec
                        );
                }
            }
            
            // reuse
            if(ioctl(*dev_fd, VIDIOC_QBUF, &v4l2_buf)<0) {
                gvb_log_critical("VIDIOC_QBUF [%s]", strerror(errno));
            }
            else {
                capt_buf->queued = TRUE;
            }
        }
        
    }
    
#undef CHECK_EXIT

    return NULL;
}
