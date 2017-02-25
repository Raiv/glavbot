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
#include <math.h>

#define MAX_SAMPLES_CNT 1000

#ifndef V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER
#define V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER (V4L2_CID_MPEG_BASE+226)
#endif

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

typedef struct {
    gint64  sample_time;  // время семлпирования
    guint32 buf_size;     // значение семпла
} GvbStatsSample;

typedef struct {
    GvbCameraStatistics  stats_pub;
    GRecMutex            lock;
    GQueue              *samples;
    gint64               start_time;   // изначальное время начала семплирования
    gint64               wrap_time;  // время последнего сворачания статистики
} GvbStatsPriv;

struct _GvbCameraPrivate {
    GRecMutex            lock;
    GvbCameraState       state;
    // device control
    gchar               *dev_path;
    gint                 dev_fd;
    // capture control
    guint32              width;
    guint32              height;
    guint32              fps;
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
    GvbStatsPriv         stats;
    gboolean             streaming_on;
    GMutex               streaming_mutex;
    GCond                streaming_cond;
};

G_DEFINE_QUARK(gvb-camera-error-quark,gvb_camera_error);
G_DEFINE_TYPE_WITH_PRIVATE(GvbCamera, gvb_camera, G_TYPE_OBJECT);

#define P(self)           G_TYPE_INSTANCE_GET_PRIVATE(self, GVB_TYPE_CAMERA, GvbCameraPrivate)
#define LOCK(self)        g_rec_mutex_lock(&P(self)->lock)
#define UNLOCK(self)      g_rec_mutex_unlock(&P(self)->lock)
#define STAT_LOCK(self)   g_rec_mutex_lock(&P(self)->stats.lock)
#define STAT_UNLOCK(self) g_rec_mutex_unlock(&P(self)->stats.lock)

#define SET_IOCTL_ERROR(error, msg)                                             \
    g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, " [%s]", strerror(errno));   \
    g_prefix_error(error, msg);

#define GVB_CAMERA_IOCTL(self, request, ...)                        \
    ({                                                              \
        LOCK(self);                                                 \
        gint ret = ioctl(P(self)->dev_fd, request, ##__VA_ARGS__);  \
        UNLOCK(self);                                               \
        ret;                                                        \
    })
    

/*
 * GOBJECT functions
 ******************************************************************************/

static void 
gvb_camera_finalize(GObject* object);
static GvbStatsSample*
gvb_stat_sample_new();
static void
gvb_stat_sample_free(GvbStatsSample *sample);
static gboolean
gvb_camera_capt_mill_start(GvbCamera *self, GError **error);
static gboolean
gvb_camera_capt_mill_stop(GvbCamera *self, GError **error);

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
    P(self)->fps = 0;
    P(self)->capt_bufs = NULL;
    P(self)->capt_bufs_count = 0;
    P(self)->capt_mill_thread = NULL;
    g_atomic_int_set(&P(self)->capt_mill_stop, 1);
    P(self)->capt_mill_cb = NULL;
    P(self)->capt_mill_cb_data = NULL;
    P(self)->capt_mill_cb_destroy_cb = NULL;
    P(self)->out_path = NULL;
    P(self)->out_fd = -1;
    memset(&P(self)->stats, 0, sizeof(GvbStatsPriv));
    P(self)->stats.stats_pub.bytes_per_second_avg0 = 0.0f;
    P(self)->stats.stats_pub.bytes_per_second_avg2 = 0.0f;
    P(self)->stats.stats_pub.bytes_per_second_avg5 = 0.0f;
    P(self)->stats.stats_pub.bytes_per_second_avg10 = 0.0f;
    P(self)->stats.stats_pub.bytes_per_second_avg20 = 0.0f;
    P(self)->stats.stats_pub.bytes_per_second_avg60 = 0.0f;
    g_rec_mutex_init(&P(self)->stats.lock);
    P(self)->stats.samples = g_queue_new();
    P(self)->streaming_on = FALSE;
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
    g_queue_free_full(P(self)->stats.samples, (GDestroyNotify)gvb_stat_sample_free);
    g_rec_mutex_clear(&P(self)->stats.lock);
}

static GvbStatsSample*
gvb_stat_sample_new()
{
    return g_slice_new0(GvbStatsSample);
}

static void
gvb_stat_sample_free(GvbStatsSample *sample)
{
    g_slice_free(GvbStatsSample, sample);
}

gboolean
gvb_camera_isopen(GvbCamera *self)
{
    gboolean ret = FALSE;
    LOCK(self);
    if(P(self)->dev_fd<0) {
        ret = FALSE;
    }
    else {
        ret = TRUE;
    }
    UNLOCK(self);
    return ret;
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
    P(self)->width = options->width;
    P(self)->height = options->height;
    P(self)->fps = options->fps;
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

static inline gboolean
validate_set_and_get_args(GvbCamera *self, GError **error, gpointer value)
{
    if(!self || !value) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "arg is null: [%p,%p]", self, value);
        return FALSE;
    }
    if(!gvb_camera_isopen(self)) {
        g_set_error(error, GVB_CAMERA_ERROR, GVB_CAMERA_ERROR_CLOSED, "camera closed");
        return FALSE;
    }
    return TRUE;
}

//gboolean
//gvb_camera_set_control2(GvbCamera *self, GError **error, GvbCameraControl control, gfloat *value)
//{
//    if(!validate_set_and_get_args(self, error, value)) {
//        return FALSE;
//    }
//    switch(control) {
////        case GVB_CAMERA_CONTROL_FRAME_RATE: {
////            struct v4l2_streamparm parm;
////            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
////            // TODO: плавающая точка...
////            parm.parm.capture.timeperframe.denominator = (guint32)*value;
////            parm.parm.capture.timeperframe.numerator = 1;
////            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_PARM, &parm)<0) {
////                SET_IOCTL_ERROR(error, "VIDIOC_S_PARM");
////                return FALSE;
////            }
////        }
////        break;
//        default:
//            g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "Unsupported control: [%d]", control);
//            return FALSE;
//    }
//    return TRUE;
//}
//
//gboolean
//gvb_camera_get_control2(GvbCamera *self, GError **error, GvbCameraControl control, gfloat *value)
//{
//    if(!validate_set_and_get_args(self, error, value)) {
//        return FALSE;
//    }
//    switch(control) {
////        case GVB_CAMERA_CONTROL_FRAME_RATE: {
////            struct v4l2_streamparm parm;
////            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
////            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_PARM, &parm)<0) {
////                SET_IOCTL_ERROR(error, "VIDIOC_G_PARM");
////                return FALSE;
////            }
////            GvbCameraFrameRate *frame_rate = (GvbCameraFrameRate*)value;
////            // frame_rate = 1/time_per_frame
////            *value = ((gdouble)parm.parm.capture.timeperframe.denominator) 
////                        / ((gdouble)parm.parm.capture.timeperframe.numerator);
////        }
////        break;
//        default:
//            g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "Unsupported control: [%d]", control);
//            return FALSE;
//    }
//    return TRUE;
//}

gboolean
gvb_camera_set_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value)
{
    if(!validate_set_and_get_args(self, error, value)) {
        return FALSE;
    }
    if(*value<0) {
        g_set_error(error, GVB_CAMERA_ERROR, GVB_CAMERA_BAD_CONTROL, "bad value for control [%d]", *value);
        return FALSE;
    }
    switch(control) {
        case GVB_CAMERA_CONTROL_FRAME_RATE: {
            struct v4l2_streamparm parm;
            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            parm.parm.capture.timeperframe.denominator = (guint32)*value;
            parm.parm.capture.timeperframe.numerator = 1;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_PARM, &parm)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_PARM");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_EXPOSURE_TIME: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
            ctrl.value = *((gint32*)value);
            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_EXPOSURE_ABSOLUTE)");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_BITRATE: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            ctrl.value = *((gint32*)value);
            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE)");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
            ctrl.value = *((gint32*)value);
            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_H264_I_PERIOD)");
                return FALSE;
            }
        }
        break;
        case GVB_CAMERA_CONTROL_INLINE_HEADER: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
            ctrl.value = *((gint32*)value);
            if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER)");
                return FALSE;
            }
        }
        break;
        default:
            g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "Unsupported control: [%d]", control);
            return FALSE;
    }
    return TRUE;
}

gboolean
gvb_camera_get_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value)
{
    if(!validate_set_and_get_args(self, error, value)) {
        return FALSE;
    }   
    switch(control) {
        case GVB_CAMERA_CONTROL_FRAME_RATE: {
            struct v4l2_streamparm parm;
            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_PARM, &parm)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_PARM");
                return FALSE;
            }
            //GvbCameraFrameRate *frame_rate = (GvbCameraFrameRate*)value;
            // frame_rate = 1/time_per_frame
            //gvb_log_message("fps: [%d]/[%d]", parm.parm.capture.timeperframe.denominator, parm.parm.capture.timeperframe.numerator);
            *value = parm.parm.capture.timeperframe.denominator / parm.parm.capture.timeperframe.numerator;
        }
        break;
        case GVB_CAMERA_CONTROL_EXPOSURE_TIME: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_CTRL (V4L2_CID_EXPOSURE_ABSOLUTE)");
                return FALSE;
            }
            *((gint32*)value) = ctrl.value;
        }
        break;
        case GVB_CAMERA_CONTROL_BITRATE: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_G_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE)");
                return FALSE;
            }
            *((gint32*)value) = ctrl.value;
        }
        break;
        case GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_H264_I_PERIOD)");
                return FALSE;
            }
            *((gint32*)value) = ctrl.value;
        }
        break;
        case GVB_CAMERA_CONTROL_INLINE_HEADER: {
            struct v4l2_control ctrl;
            ctrl.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_G_CTRL, &ctrl)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER)");
                return FALSE;
            }
            *((gint32*)value) = ctrl.value;
        }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

gboolean
gvb_camera_query_control(
    GvbCamera *self, GvbCameraControl control
  , gint32 *min, gint32 *max, gint32 *step, gint32 *deflt
  , GError **error )
{
    if(!self) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "camera is null");
        return FALSE;
    }
    if(!gvb_camera_isopen(self)) {
        g_set_error(error, GVB_CAMERA_ERROR, GVB_CAMERA_ERROR_CLOSED, "camera closed");
        return FALSE;
    }
    
    struct v4l2_queryctrl query = {0};
    if(control==GVB_CAMERA_CONTROL_EXPOSURE_TIME) {
        query.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    }
    else if (control==GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD) {
        query.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
    }
    else if (control==GVB_CAMERA_CONTROL_INLINE_HEADER) {
        query.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
    }
    else if (control==GVB_CAMERA_CONTROL_BITRATE) {
        query.id = V4L2_CID_MPEG_VIDEO_BITRATE;
    }
    else if(control==GVB_CAMERA_CONTROL_FRAME_RATE) {
        if(min) {
            *min = 5;
        }
        if(max) {
            *max = 60;
        }
        if(step) {
            *step = 1;
        }
        if(deflt) {
            *deflt = 30;
        }
        return TRUE;
    }
    else {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "unsupported control: [%d]", control);
        return FALSE;
    }
    
    if(GVB_CAMERA_IOCTL(self, VIDIOC_QUERYCTRL, &query)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_QUERYCTRL");
        return FALSE;
    }

    if(control==GVB_CAMERA_CONTROL_EXPOSURE_TIME) {
        if(min) {
            *min = 100;
        }
        if(max) {
            *max = 100;
        }
        if(deflt) {
            *deflt = 100;
        }
    }
    else if(control==GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD) {
        if(min) {
            *min = 20;
        }
        if(max) {
            *max = 100;
        }
        if(deflt) {
            *deflt = 20;
        }
    }
    else {
        if(min) {
            *min = query.minimum;
        }
        if(max) {
            *max = query.maximum;
        }
        if(deflt) {
            *deflt = query.default_value;
        }
    }
    
    if(step) {
        *step = query.step;
    }
    
    return TRUE;
}

gboolean 
gvb_camera_default_control(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    gint32 btr_deflt = 0, fps_deflt = 0, exp_deflt = 0, ifr_deflt = 0, inln_dflt = 0;
    if(!gvb_camera_query_control(self, GVB_CAMERA_CONTROL_BITRATE, NULL, NULL, NULL, &btr_deflt, error)
        ||!gvb_camera_query_control(self, GVB_CAMERA_CONTROL_FRAME_RATE, NULL, NULL, NULL, &fps_deflt, error)
        ||!gvb_camera_query_control(self, GVB_CAMERA_CONTROL_EXPOSURE_TIME, NULL, NULL, NULL, &exp_deflt, error)
        ||!gvb_camera_query_control(self, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, NULL, NULL, NULL, &ifr_deflt, error)
        ||!gvb_camera_query_control(self, GVB_CAMERA_CONTROL_INLINE_HEADER, NULL, NULL, NULL, &inln_dflt, error)
    ){
        return FALSE;
    }
    
    gvb_log_message("default bitrate: [%d]", btr_deflt);
    gvb_log_message("default framerate: [%d]", fps_deflt);
    gvb_log_message("default exposure time: [%d]", exp_deflt);
    gvb_log_message("default inter frame period: [%d]", ifr_deflt);
    gvb_log_message("default inline header: [%d]", inln_dflt);
    
    if(!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_BITRATE, &btr_deflt)
        ||!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_FRAME_RATE, &fps_deflt)
        ||!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &exp_deflt)
        ||!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &ifr_deflt)
        ||!gvb_camera_set_control(self, error, GVB_CAMERA_CONTROL_INLINE_HEADER, &inln_dflt)
    ){
        return FALSE;
    }
    
    return TRUE;
}

gboolean
gvb_camera_get_statistics(GvbCamera *self, GError **error, GvbCameraStatistics *statistics)
{
    g_return_val_if_fail(self!=NULL, FALSE);
    g_return_val_if_fail(statistics!=NULL, FALSE);
    
    STAT_LOCK(self);
    
    // static is safe with STAT_LOCK
    static guint32 bytes2, bytes5, bytes10, bytes20, bytes60;
    static gint64 edge2, edge5, edge10, edge20, edge60;
    static GvbStatsSample *sample;
    static gint64 curtime;
    static GvbStatsPriv *stats;
    static guint num_samples;
    static gdouble delta_t0;
    static gdouble delta_t0_2;
    static gdouble delta_t1_2;
    static gdouble delta_t0_5;
    static gdouble delta_t1_5;
    static gdouble delta_t0_10;
    static gdouble delta_t1_10;
    static gdouble delta_t0_20;
    static gdouble delta_t1_20;
    static gdouble delta_t0_60;
    static gdouble delta_t1_60;
    
    stats = &P(self)->stats;
    if((num_samples=g_queue_get_length(P(self)->stats.samples)) <= 0) {
        goto ret_success;
    }
    
    curtime = g_get_monotonic_time();
    bytes2 = bytes5 = bytes10 = bytes20 = bytes60 = 0;
    edge2 = edge5 = edge10 = edge20 = edge60 = curtime;
    sample = NULL;
    
    guint idx = 0;
    while((sample=g_queue_pop_tail(stats->samples))) {
        // идём по семплам с хвоста, то есть анализируем сначала самые свежие данные
        // мгновенная скорость
        if(num_samples<=1) {
            if(idx==0) {
                stats->stats_pub.bytes_per_second_avg0
                    = sample->buf_size / (sample->sample_time - stats->wrap_time);
            }
        }
        else {
            if(idx==1) {
                stats->stats_pub.bytes_per_second_avg0
                    = bytes2 / (edge2 - sample->sample_time);
            }
        }
        idx++;
        // другие статистики
        if(curtime - sample->sample_time <= 2*G_TIME_SPAN_SECOND) {
            bytes2 += sample->buf_size;
            edge2 = sample->sample_time;
        }
        if(curtime - sample->sample_time <= 5*G_TIME_SPAN_SECOND) {
            bytes5 += sample->buf_size;
            edge5 = sample->sample_time;
        }
        if(curtime - sample->sample_time <= 10*G_TIME_SPAN_SECOND) {
            bytes10 += sample->buf_size;
            edge10 = sample->sample_time;
        }
        if(curtime - sample->sample_time <= 20*G_TIME_SPAN_SECOND) {
            bytes20 += sample->buf_size;
            edge20 = sample->sample_time;
        }
        if(curtime - sample->sample_time <= 60*G_TIME_SPAN_SECOND) {
            bytes60 += sample->buf_size;
            edge60 = sample->sample_time;
        }
        gvb_stat_sample_free(sample);
    }
    
    // время прошедшее от начала семплирования до последней свёртки
    delta_t0 = stats->wrap_time - stats->start_time;
    delta_t0 = ( delta_t0==0 ? curtime-stats->start_time : delta_t0 );
    //
    delta_t0_2  = (delta_t0 > 2*G_TIME_SPAN_SECOND ? 2*G_TIME_SPAN_SECOND : delta_t0);
    delta_t1_2  = curtime - edge2;
    delta_t0_5  = (delta_t0 > 5*G_TIME_SPAN_SECOND ? 5*G_TIME_SPAN_SECOND : delta_t0);
    delta_t1_5  = curtime - edge5;
    delta_t0_10 = (delta_t0 > 10*G_TIME_SPAN_SECOND ? 10*G_TIME_SPAN_SECOND : delta_t0);
    delta_t1_10 = curtime - edge20;
    delta_t0_20 = (delta_t0 > 20*G_TIME_SPAN_SECOND ? 20*G_TIME_SPAN_SECOND : delta_t0);
    delta_t1_20 = curtime - edge60;
    delta_t0_60 = (delta_t0 > 60*G_TIME_SPAN_SECOND ? 60*G_TIME_SPAN_SECOND : delta_t0);
    delta_t1_60 = curtime - edge60;
    
    stats->stats_pub.bytes_per_second_avg2
        = stats->stats_pub.bytes_per_second_avg2 * delta_t0_2 / ( delta_t0_2 + delta_t1_2 )
            + G_TIME_SPAN_SECOND * bytes2 / ( delta_t0_2 + delta_t1_2 );
    stats->stats_pub.bytes_per_second_avg5
        = stats->stats_pub.bytes_per_second_avg5 * delta_t0_5 / ( delta_t0_5 + delta_t1_5 )
            + G_TIME_SPAN_SECOND * bytes5 / ( delta_t0_5 + delta_t1_5 );
    stats->stats_pub.bytes_per_second_avg10
        = stats->stats_pub.bytes_per_second_avg10 * delta_t0_10 / ( delta_t0_10 + delta_t1_10 )
            + G_TIME_SPAN_SECOND * bytes10 / ( delta_t0_10 + delta_t1_10 );
    stats->stats_pub.bytes_per_second_avg20
        = stats->stats_pub.bytes_per_second_avg20 * delta_t0_20 / ( delta_t0_20 + delta_t1_20 )
            + G_TIME_SPAN_SECOND * bytes20 / ( delta_t0_20 + delta_t1_20 );
    stats->stats_pub.bytes_per_second_avg60
        = stats->stats_pub.bytes_per_second_avg60 * delta_t0_60 / ( delta_t0_60 + delta_t1_60 )
            + G_TIME_SPAN_SECOND * bytes60 / ( delta_t0_60 + delta_t1_60 );
    
    stats->stats_pub.analyzed_us = curtime - stats->wrap_time;
    stats->stats_pub.analyzed_samples = num_samples;
    stats->wrap_time = curtime;
    
ret_success:
    memcpy(statistics, &stats->stats_pub, sizeof(GvbCameraStatistics));
    STAT_UNLOCK(self);
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
gvb_camera_initial_buf_queue(GvbCamera *self, GError **error, gboolean force);
static gpointer
gvb_camera_capt_mill_fn(gpointer user_data);

gboolean
gvb_camera_open(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR, "self is null");
        return FALSE;
    }
    
    LOCK(self);
    if(!P(self)->dev_path) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE, "no dev path");
        goto fail0;
    }
    if(P(self)->dev_fd>=0) {
        gvb_log_warning("video device file descriptor is not null in gvb_camera_open");
        g_close(P(self)->dev_fd, NULL);
        P(self)->dev_fd = -1;
    }
    if((P(self)->dev_fd = g_open(P(self)->dev_path, O_RDWR))<0) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, strerror(errno));
        goto fail0;
    }
    UNLOCK(self);
    return TRUE;
    
fail0:
    UNLOCK(self);
    return FALSE;
}

gboolean
gvb_camera_close(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR, "self is null");
        return FALSE;
    }
    LOCK(self);
    g_close(P(self)->dev_fd,NULL);
    P(self)->dev_fd = -1;
    UNLOCK(self);
    return TRUE;
}

gboolean
gvb_camera_start_capturing(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR, "self is null");
        return FALSE;
    }
    GError *error_priv = NULL;
    
    gvb_log_message(__func__);
    
    LOCK(self);
    
    if(P(self)->state==GVB_CAMERA_STATE_CAPTURING) {
        UNLOCK(self);
        return TRUE;
    }
    if(!gvb_camera_isopen(self) && !gvb_camera_open(self, error)) {
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
    if(GVB_CAMERA_IOCTL(self, VIDIOC_G_FMT, &format)<0) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_G_FMT [%s]", strerror(errno));
        goto fail1;
    }
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    if(P(self)->width>0 && P(self)->height>0) {
        format.fmt.pix.width = P(self)->width;
        format.fmt.pix.height = P(self)->height;
    }
    gvb_log_message("default format: [%dx%d]", format.fmt.pix.width, format.fmt.pix.height);
    if(GVB_CAMERA_IOCTL(self, VIDIOC_S_FMT, &format)<0) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_IOCTL, "VIDIOC_S_FMT [%s]", strerror(errno));
        goto fail1;
    }
    
    // bitrate mode
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
    ctrl.value = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR;
    if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_MPEG_VIDEO_BITRATE_MODE)");
        goto fail1;
    }
    // exposure mode
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ctrl.value = V4L2_EXPOSURE_MANUAL;
    if(GVB_CAMERA_IOCTL(self, VIDIOC_S_CTRL, &ctrl)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_S_CTRL (V4L2_CID_EXPOSURE_AUTO)");
        return FALSE;
    }
    
    // Сбрасываем настройки в состояние по умолчанию
    if(!gvb_camera_default_control(self, error)) {
        goto fail1;
    }
    // initialize memory
    if(!gvb_camera_init_memory(self, error)) {
        goto fail2;
    }
    // here we start buffers handling
    if(!gvb_camera_capt_mill_start(self, error) 
        || !gvb_camera_turn_streaming_on(self, error))
    {
        goto fail3;
    }
    P(self)->state = GVB_CAMERA_STATE_CAPTURING;
    P(self)->stats.start_time = g_get_monotonic_time();
    P(self)->stats.wrap_time = P(self)->stats.start_time;
    
    UNLOCK(self);
    return TRUE;
fail3:
    // stop thread
    g_atomic_int_set(&P(self)->capt_mill_stop, 1);
    if(P(self)->capt_mill_thread) {
        UNLOCK(self);
        g_thread_join(P(self)->capt_mill_thread);
        LOCK(self);
        P(self)->capt_mill_thread = NULL;
    }
    if(!gvb_camera_capt_mill_stop(self, &error_priv) 
        || !gvb_camera_turn_streaming_off(self, &error_priv)) {
        gvb_error_stacked(error, &error_priv);
    }
fail2:
    gvb_camera_free_memory(self, error);
fail1:
    gvb_camera_close(self, error);
fail0:
    UNLOCK(self);
    return FALSE;
}

gboolean
gvb_camera_stop_capturing(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR, "self is null");
        return FALSE;
    }
    
    LOCK(self);
    
    gvb_log_message(__func__);
    
    if(P(self)->state==GVB_CAMERA_STATE_IDLE
        || P(self)->dev_fd<0 ) {
        UNLOCK(self);
        return TRUE;
    }
    
    if(!gvb_camera_capt_mill_stop(self,error) 
        || !gvb_camera_turn_streaming_off(self, error)) {
        goto fail0;
    }
    
    if(!gvb_camera_free_memory(self, error)) {
        goto fail0;
    }
    
    if(!g_close(P(self)->dev_fd, error)) {
        goto fail0;
    }
    P(self)->dev_fd = -1;
    
    if(P(self)->out_fd>=0 
            && !g_close(P(self)->out_fd, error)) {
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
    if(GVB_CAMERA_IOCTL(self, VIDIOC_REQBUFS, &v4l2_reqbufs)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_REQBUFS");
        goto fail0;
    }
    
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
        if(GVB_CAMERA_IOCTL(self, VIDIOC_QUERYBUF, &v4l2_buf)<0) {
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
    if(GVB_CAMERA_IOCTL(self, VIDIOC_REQBUFS, &v4l2_reqbufs)<0) {
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

gboolean
gvb_camera_turn_streaming_on(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    gvb_log_message(__func__);
    // Before VIDIOC_STREAMON we must queue buffers. Otherwise we get "VIDIOC_STREAMON error [Operation not permitted]"
    if(!gvb_camera_initial_buf_queue(self, error, TRUE)) {
        return FALSE;
    }
    
    enum v4l2_buf_type v4l2_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(GVB_CAMERA_IOCTL(self, VIDIOC_STREAMON, &v4l2_buf_type)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_STREAMON");
        return FALSE;
    }
    g_mutex_lock(&P(self)->streaming_mutex);
    P(self)->streaming_on = TRUE;
    g_cond_signal(&P(self)->streaming_cond);
    g_mutex_unlock(&P(self)->streaming_mutex);
    return TRUE;
}

gboolean
gvb_camera_turn_streaming_off(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    gvb_log_message(__func__);
    
    g_mutex_lock(&P(self)->streaming_mutex);
    
    enum v4l2_buf_type v4l2_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(GVB_CAMERA_IOCTL(self, VIDIOC_STREAMOFF, &v4l2_buf_type)<0) {
        SET_IOCTL_ERROR(error, "VIDIOC_STREAMOFF");
        return FALSE;
    }
    P(self)->streaming_on = FALSE;
    g_cond_signal(&P(self)->streaming_cond);
    
    g_mutex_unlock(&P(self)->streaming_mutex);
    return TRUE;
}

gboolean
gvb_camera_is_streaming_on(GvbCamera *self)
{
    if(!self) {
        gvb_log_critical("self is null");
        return FALSE;
    }
    gboolean retval = FALSE;
    g_mutex_lock(&P(self)->streaming_mutex);
    retval = P(self)->streaming_on;
    g_mutex_unlock(&P(self)->streaming_mutex);
    return retval;
}

static gboolean
gvb_camera_initial_buf_queue(GvbCamera *self, GError **error, gboolean force)
{
    guint32 idx;
    struct v4l2_buffer v4l2_buf;
    CaptureBuffer *capt_buf;
    for(idx=0; idx<P(self)->capt_bufs_count; idx++) {
        memset(&v4l2_buf,0,sizeof(struct v4l2_buffer));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        capt_buf = P(self)->capt_bufs+idx;
        if(!capt_buf->queued || force) {
            v4l2_buf.index = idx;
            if(GVB_CAMERA_IOCTL(self, VIDIOC_QBUF, &v4l2_buf)<0) {
                SET_IOCTL_ERROR(error, "VIDIOC_QBUF");
                return FALSE;
            }
            capt_buf->queued = TRUE;
        }
    }
    return TRUE;
}

static void
gvb_camera_update_statistics(GvbCamera *self, guint32 buf_size)
{
    g_return_if_fail(self);
    
    STAT_LOCK(self);
    
    GvbStatsSample *sample = gvb_stat_sample_new();
    sample->sample_time = g_get_monotonic_time();
    sample->buf_size = buf_size;
    
    P(self)->stats.stats_pub.buffers_all++;
    P(self)->stats.stats_pub.bytes_all += buf_size;
    
    // push sample
    g_queue_push_tail(P(self)->stats.samples, sample);
    if(g_queue_get_length(P(self)->stats.samples)>MAX_SAMPLES_CNT) {
        gvb_stat_sample_free((GvbStatsSample*)g_queue_pop_head(P(self)->stats.samples));
    }
    
    STAT_UNLOCK(self);
}

static gboolean
gvb_camera_capt_mill_start(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    gvb_log_message(__func__);
    if(g_atomic_int_get(&P(self)->capt_mill_stop)) {
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
    }
    return TRUE;
}

static gboolean
gvb_camera_capt_mill_stop(GvbCamera *self, GError **error)
{
    if(!self) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_INVALID_ARG, "self is null");
        return FALSE;
    }
    gvb_log_message(__func__);
    LOCK(self);
    g_atomic_int_set(&P(self)->capt_mill_stop, 1);
    if(P(self)->capt_mill_thread) {
        UNLOCK(self);
        g_thread_join(P(self)->capt_mill_thread);
        LOCK(self);
        P(self)->capt_mill_thread = NULL;
    }
    UNLOCK(self);
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
    CaptureBuffer *capt_buf = NULL;
    struct v4l2_buffer v4l2_buf;
    
#define CHECK_EXIT()                                  \
    if(g_atomic_int_get(&P(self)->capt_mill_stop)) {  \
        g_thread_exit(NULL);                          \
    }
#define CHECK_STREAMING_ON() {                                                 \
        g_mutex_lock(&P(self)->streaming_mutex);                               \
        while(!P(self)->streaming_on) {                                        \
            g_cond_wait_until(                                                 \
                &P(self)->streaming_cond                                       \
              , &P(self)->streaming_mutex                                      \
              , g_get_monotonic_time()+100*G_TIME_SPAN_MILLISECOND             \
            );                                                                 \
            gvb_camera_update_statistics(self, 0);                             \
        }                                                                      \
        g_mutex_unlock(&P(self)->streaming_mutex);                             \
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
            // проверить флаг streaming_on - если 0, то каким-то образом поставить на паузу получение буферов
            CHECK_STREAMING_ON();
            
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
                    cb(cb_user_data, capt_buf->start, v4l2_buf.bytesused, v4l2_buf.flags);
                }
                if(*out_fd>=0) {
                    write(*out_fd, capt_buf->start, v4l2_buf.bytesused);
                }
                gvb_camera_update_statistics(self, v4l2_buf.bytesused);
//                allbytes += v4l2_buf.bytesused;
//                if(print_slow++%60==0) {
//                    gvb_log_info("[%d]\tprocessing buffer [%p]: bytesused=%d \tallbytes=%d \tfield=%d \tsequence=%d \ttimestamp=%d:%d"
//                        , print_slow
//                        , capt_buf->start, v4l2_buf.bytesused, allbytes, v4l2_buf.field, v4l2_buf.sequence
//                        , v4l2_buf.timestamp.tv_sec, v4l2_buf.timestamp.tv_usec
//                        );
//                }
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

// для тестирования в gvb-misc.c

void
gvb_camera_test(GvbCamera *self)
{
    g_return_if_fail(self);
    
    // вытянуть какие есть настройки у камеры
    
    
}