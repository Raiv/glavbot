#ifndef GVB_CAMERA_H
#define GVB_CAMERA_H

#include <glib.h>
#include <glib-object.h>
#include "gvb-camera-opt.h"
    
/**
 * GOBJECT defines
 ******************************************************************************/

#define GVB_TYPE_CAMERA             (gvb_camera_get_type ())
#define GVB_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GVB_TYPE_CAMERA, GvbCamera))
#define GVB_IS_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GVB_TYPE_CAMERA))
#define GVB_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GVB_TYPE_CAMERA, GvbCameraClass))
#define GVB_IS_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GVB_TYPE_CAMERA))
#define GVB_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GVB_TYPE_CAMERA, GvbCameraClass))

/**
 * MACROS
 ******************************************************************************/
#define GVB_CAMERA_ERROR gvb_camera_error_quark()

/**
 * TYPEDEFs
 ******************************************************************************/
    
// Структуры класса
typedef struct _GvbCameraClass GvbCameraClass;
typedef struct _GvbCamera GvbCamera;
typedef struct _GvbCameraPrivate GvbCameraPrivate;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GvbCamera, g_object_unref)

typedef enum {
    GVB_CAMERA_CONTROL_WIDTH,           // not supported yet
    GVB_CAMERA_CONTROL_HEIGHT,          // not supported yet
    GVB_CAMERA_CONTROL_FRAME_RATE,
    GVB_CAMERA_CONTROL_EXPOSURE_TIME,
    GVB_CAMERA_CONTROL_BITRATE,
    GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD,
    GVB_CAMERA_CONTROL_INLINE_HEADER
} GvbCameraControl;

typedef enum {
    GVB_CAMERA_STATE_IDLE
  , GVB_CAMERA_STATE_CAPTURING
} GvbCameraState;

typedef void (*GvbCameraBufferCbFn)(gpointer user_data, gpointer start, guint32 bytes, guint32 flags);

typedef struct {
    guint32 numerator;
    guint32 denominator;
} GvbCameraFrameRate;

typedef struct {
    gint64  analyzed_us; // интервал времени в мкс между двумя wrap-time
    gint64  analyzed_samples; // количество семплов между двумя wrap-time
    guint32 bytes_all;
    guint32 buffers_all;
    gdouble bytes_per_second_avg0; // мгновенная скорость генерации байтов
    gdouble bytes_per_second_avg2;
    gdouble bytes_per_second_avg5;
    gdouble bytes_per_second_avg10;
    gdouble bytes_per_second_avg20;
    gdouble bytes_per_second_avg60;
} GvbCameraStatistics;

typedef enum {
    GVB_CAMERA_ERROR_CLOSED
  , GVB_CAMERA_BAD_CONTROL
} GvbCameraErrorEnum;

/**
 * FUNCTIONs
 ******************************************************************************/

GQuark 
gvb_camera_error_quark (void);

GvbCamera*
gvb_camera_new(GvbCameraOptions *options);

gboolean
gvb_camera_isopen(GvbCamera *self);

gboolean
gvb_camera_get_dev_path(GvbCamera *self, gchar **path);

gboolean
gvb_camera_get_state(GvbCamera *self, GvbCameraState *state);

gboolean
gvb_camera_set_buffer_callback(GvbCamera *self, GvbCameraBufferCbFn callback, gpointer user_data, GDestroyNotify destroy_cb);

gboolean
gvb_camera_open(GvbCamera *self, GError **error);

gboolean
gvb_camera_close(GvbCamera *self, GError **error);

gboolean
gvb_camera_turn_streaming_on(GvbCamera *self, GError **error);

gboolean
gvb_camera_turn_streaming_off(GvbCamera *self, GError **error);

gboolean
gvb_camera_is_streaming_on(GvbCamera *self);

// automatic open
gboolean
gvb_camera_start_capturing(GvbCamera *self, GError **error);

// automatic close
gboolean
gvb_camera_stop_capturing(GvbCamera *self, GError **error);

gboolean 
gvb_camera_default_control(GvbCamera *self, GError **error);

gboolean
gvb_camera_query_control(GvbCamera *self, GvbCameraControl control
  , gint32 *min, gint32 *max, gint32 *step, gint32 *deflt
  , GError **error );

gboolean
gvb_camera_set_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value);

gboolean
gvb_camera_get_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value);

gboolean
gvb_camera_get_statistics(GvbCamera *self, GError **error, GvbCameraStatistics *statistics);

#endif /* GVB_CAMERA_H */

