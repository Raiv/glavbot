#include "gvb-tests.h"
#include "gvb-log.h"
#include "gvb-client.h"
#include "gvb-server.h"
#include "gvb-camera.h"
#include "gvb-error.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <linux/videodev2.h>

#define UI FALSE

#ifndef V4L2_BUF_FLAG_MAPPED
#define V4L2_BUF_FLAG_MAPPED			0x00000001
#endif
#ifndef V4L2_BUF_FLAG_QUEUED
#define V4L2_BUF_FLAG_QUEUED			0x00000002
#endif
#ifndef V4L2_BUF_FLAG_DONE
#define V4L2_BUF_FLAG_DONE			0x00000004
#endif
#ifndef V4L2_BUF_FLAG_KEYFRAME
#define V4L2_BUF_FLAG_KEYFRAME			0x00000008
#endif
#ifndef V4L2_BUF_FLAG_PFRAME
#define V4L2_BUF_FLAG_PFRAME			0x00000010
#endif
#ifndef V4L2_BUF_FLAG_BFRAME
#define V4L2_BUF_FLAG_BFRAME			0x00000020
#endif
#ifndef V4L2_BUF_FLAG_ERROR
#define V4L2_BUF_FLAG_ERROR			0x00000040
#endif
#ifndef V4L2_BUF_FLAG_TIMECODE
#define V4L2_BUF_FLAG_TIMECODE			0x00000100
#endif
#ifndef V4L2_BUF_FLAG_PREPARED
#define V4L2_BUF_FLAG_PREPARED			0x00000400
#endif
#ifndef V4L2_BUF_FLAG_NO_CACHE_INVALIDATE
#define V4L2_BUF_FLAG_NO_CACHE_INVALIDATE	0x00000800
#endif
#ifndef V4L2_BUF_FLAG_NO_CACHE_CLEAN
#define V4L2_BUF_FLAG_NO_CACHE_CLEAN		0x00001000
#endif
#ifndef V4L2_BUF_FLAG_TIMESTAMP_MASK
#define V4L2_BUF_FLAG_TIMESTAMP_MASK		0x0000e000
#endif
#ifndef V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN
#define V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN		0x00000000
#endif
#ifndef V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC
#define V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC	0x00002000
#endif
#ifndef V4L2_BUF_FLAG_TIMESTAMP_COPY
#define V4L2_BUF_FLAG_TIMESTAMP_COPY		0x00004000
#endif
#ifndef V4L2_BUF_FLAG_TSTAMP_SRC_MASK
#define V4L2_BUF_FLAG_TSTAMP_SRC_MASK		0x00070000
#endif
#ifndef V4L2_BUF_FLAG_TSTAMP_SRC_EOF
#define V4L2_BUF_FLAG_TSTAMP_SRC_EOF		0x00000000
#endif
#ifndef V4L2_BUF_FLAG_TSTAMP_SRC_SOE
#define V4L2_BUF_FLAG_TSTAMP_SRC_SOE		0x00010000
#endif



static GvbNetworkOptions _nwk_opts = {0};
static GvbCameraOptions  _cam_opts = {0};
//static GvbServerOptions  _srv_opts = {};
static GvbClientOptions  _cli_opts = {};

static GOptionContext *_opt_ctx = NULL;
static GMainLoop      *_loop    = NULL;
static GvbServer      *_server  = NULL;
static GvbClient      *_client  = NULL;
static GvbCamera      *_camera  = NULL;

static gint _client_data_fd = -1;

static gfloat   _bitrate_ratio = 1.0;
static gboolean _drop_buffers  = FALSE;
static gboolean _max_quality   = FALSE;

typedef struct {
    gint32 id;
    // принимает положительное или отрицательное значение
    // показывает, каким образом увеличение влияет на объём генерируемых данных
    // положительное значение - при увеличении параметра ожидается увеличение объёма данных
    // отричательное значение - обратная зависимость
    gint32 rtt_influence;
    gint32 min;
    gint32 max;
    gint32 step;
    gint32 deflt;
    gint32 value;
} Control;

// элементы массива расположены в порядке приоритета
// сначала изменяется первый; если уперлись в min или max, только тогда берёмся
// за второй, потом за третий и т.д.
static Control controls[] = { 
    { .id=GVB_CAMERA_CONTROL_BITRATE, .rtt_influence=1,  .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_FRAME_RATE, .rtt_influence=1,  .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_EXPOSURE_TIME, .rtt_influence=-1, .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, .rtt_influence=-1, .min=0, .max=0, .step=0, .deflt=0, .value=0 }
};

#define BTR_CTL (controls[0])
#define FPS_CTL (controls[1])
#define EXP_CTL (controls[2])
#define IFR_CTL (controls[3])

static
gboolean sigint_handler(gpointer user_data)
{
    if(_loop) {
        g_main_loop_quit(_loop);
    }
    return TRUE;
}

static void 
camera_cb(gpointer user_data, gpointer start, guint32 bytes, guint32 flags)
{
    GError *error = NULL;
    GvbClient *client = GVB_CLIENT(user_data);
    guint32 bytes_tx = bytes;
/*
    gvb_log_message("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
      , (flags & V4L2_BUF_FLAG_MAPPED ? "V4L2_BUF_FLAG_MAPPED " : "")
      , (flags & V4L2_BUF_FLAG_QUEUED ? "V4L2_BUF_FLAG_QUEUED " : "")
      , (flags & V4L2_BUF_FLAG_DONE ? "V4L2_BUF_FLAG_DONE " : "")
      , (flags & V4L2_BUF_FLAG_ERROR ? "V4L2_BUF_FLAG_ERROR " : "")
      , (flags & V4L2_BUF_FLAG_KEYFRAME ? "V4L2_BUF_FLAG_KEYFRAME " : "")
      , (flags & V4L2_BUF_FLAG_PFRAME ? "V4L2_BUF_FLAG_PFRAME " : "")
      , (flags & V4L2_BUF_FLAG_BFRAME ? "V4L2_BUF_FLAG_BFRAME " : "")
      , (flags & V4L2_BUF_FLAG_TIMECODE ? "V4L2_BUF_FLAG_TIMECODE " : "")
      , (flags & V4L2_BUF_FLAG_PREPARED ? "V4L2_BUF_FLAG_PREPARED " : "")
      , (flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE ? "V4L2_BUF_FLAG_NO_CACHE_INVALIDATE " : "")
      , (flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN ? "V4L2_BUF_FLAG_NO_CACHE_CLEAN " : "")
      //, (flags & V4L2_BUF_FLAG_LAST ? "V4L2_BUF_FLAG_LAST " : "")
      , (flags & V4L2_BUF_FLAG_TIMESTAMP_MASK ? "V4L2_BUF_FLAG_TIMESTAMP_MASK " : "")
      , (flags & V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN ? "V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN " : "")
      , (flags & V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC ? "V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC " : "")
      , (flags & V4L2_BUF_FLAG_TIMESTAMP_COPY ? "V4L2_BUF_FLAG_TIMESTAMP_COPY " : "")
      , (flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK ? "V4L2_BUF_FLAG_TSTAMP_SRC_MASK " : "")
      , (flags & V4L2_BUF_FLAG_TSTAMP_SRC_EOF ? "V4L2_BUF_FLAG_TSTAMP_SRC_EOF " : "")
      , (flags & V4L2_BUF_FLAG_TSTAMP_SRC_SOE ? "V4L2_BUF_FLAG_TSTAMP_SRC_SOE " : "")
    );
*/
    
    if(_bitrate_ratio < 1 && _drop_buffers) {
        //NOTE: убрал, чтобы совсем не отбрасывать данные. При критической ситуации - 
        // минимальные настройки камеры и всё равно не влезаем в канал, мы будем делать
        // turn_streaming_off
        //bytes_tx = (guint32)round(_bitrate_ratio*bytes);
    }
    
    GOutputStream *ostream = NULL;
    if(!gvb_client_get_output_stream(client, &ostream, &error)) {
        gvb_log_error(&error);
        return;
    }
    
    gssize wsize = -1;
    if((wsize=g_output_stream_write(ostream, start, bytes_tx, NULL, &error))<0) {
        gvb_log_error(&error);
        if(error && error->code==G_IO_ERROR_BROKEN_PIPE) {
            if(!gvb_camera_turn_streaming_off(_camera, &error)) {
                gvb_log_error(&error);
            }
        }
        return;
    }
    
    if(bytes_tx!=bytes) {
        gvb_log_message("partially send: [%d/%d]", bytes-bytes_tx, bytes_tx);
    }
}

static void 
camera_destroy_cb(gpointer data)
{
    gvb_log_message("camera_destroy_cb, unref client");
    g_object_unref(GVB_CLIENT(data));
}

static void
client_connect_cb(gboolean connected, gpointer user_data)
{
    GError *error = NULL;
    gvb_log_message("client_connect_cb");
    if(connected) {
        if(_client_data_fd>=0) {
            if(!g_close(_client_data_fd, &error)) {
                gvb_log_error(&error);
            }
            else {
                _client_data_fd = -1;
            }
        }
        
        const gchar *home_dir = g_get_home_dir();
        const gchar *file_name = "client_data.h264";
        
        if(!home_dir) {
            gvb_log_warning("can't find home");
        }
        else {            
            gchar *path = g_build_filename(home_dir, file_name, NULL);
            if(path){
                gvb_log_message("create output file: [%s]", path);
                _client_data_fd = g_open(path, O_RDWR | O_CREAT | O_TRUNC
                      , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                if(_client_data_fd<0) {
                    g_set_error_literal(&error, GVB_ERROR, GVB_ERROR_OTHER, strerror(errno));
                    gvb_log_error(&error);
                }
                g_free(path);
            }
            else {
                gvb_log_warning("g_build_filename error");
            }
        }
        if(!gvb_camera_turn_streaming_on(_camera, &error)) {
            gvb_log_error(&error);
        }
    }
    else {
        if(!gvb_camera_turn_streaming_off(_camera, &error)) {
            gvb_log_error(&error);
        }
        if(_client_data_fd>=0) {
            if(!g_close(_client_data_fd, &error)) {
                gvb_log_error(&error);
            }
            else {
                _client_data_fd = -1;
            }
        }
    }
}

static void
client_input_data_cb(gchar *data, gsize size, gpointer user_data)
{
    static guint32 size_sum = 0;
    static gint64 last_tm = 0;
    gboolean print = FALSE;
    
    if(_client_data_fd >= 0) {
        gsize wc = size;
        gsize ret = -1;
        while(wc > 0) {
            ret = write(_client_data_fd, data, size);
            if(ret < 0) {
                gvb_log_critical("write error: [%s]", strerror(errno));
                break;
            }
            else if(!ret) {
                gvb_log_warning("write zero bytes: [%s]", strerror(errno));
            }
            else {
                wc -= ret;
            }
        }
    }
    
    size_sum += size;
    
    if(!last_tm) {
        last_tm = g_get_monotonic_time();
        print = TRUE;
    }
    else {
        if(g_get_monotonic_time() - last_tm > G_TIME_SPAN_MILLISECOND*500) {
            last_tm = g_get_monotonic_time();
            print = TRUE;
        }
    }
    
    if(print) {
        gvb_log_message("client_input_data_cb: receive client data [%d]", size_sum);
    }
    
    // todo: данные, пришедшие от клиента
}

static gboolean
camera_slow_down(
      gfloat *tx_bytes_per_sec, gfloat *camera_bytes_per_sec, gfloat *bytes_tolerance, GError **error )
{
    // множитель для шага изменения параметра равен отношению между
    // 1) отклонением текущего bytes_per_sec от значения, которое может обеспечить сеть
    // 2) значению tolerance
    gint32 step_mult = 0;
    if(*tx_bytes_per_sec <= *camera_bytes_per_sec) {
        step_mult = round((*camera_bytes_per_sec-*tx_bytes_per_sec)/(*bytes_tolerance));
    }
    else {
        g_set_error(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE
                , "camera_speed_up: unexpected tx_bytes_per_sec > camera_bytes_per_sec");
        return FALSE;
    }
    
    gint32 ctl_value = 0;
    gint32 ctl_value_new = 0;
    gint32 i = 0;
    for(i=0; i<G_N_ELEMENTS(controls); i++) 
    {
        if(!gvb_camera_get_control(_camera, error, controls[i].id, &ctl_value)) {
            return FALSE;
        }
        
        if(controls[i].id==GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD) {
            continue;
        }
        
        ctl_value_new = ctl_value;    
        if(controls[i].rtt_influence > 0){
            ctl_value_new = MAX(ctl_value-step_mult*controls[i].step, controls[i].min); // уменьшаем
        }
        else if(controls[i].rtt_influence < 0){
            ctl_value_new = MIN(ctl_value+step_mult*controls[i].step, controls[i].max); // увеличиваем
        }
        
        if(ctl_value_new!=ctl_value) {
            if(!gvb_camera_set_control(_camera, error, controls[i].id, &ctl_value_new)){
                return FALSE;
            }
            _max_quality = FALSE;
            controls[i].value = ctl_value_new;
            return TRUE;
        }
    }
    
    // no controls was set. Droping
    _drop_buffers = TRUE;
    if(gvb_camera_is_streaming_on(_camera)) {
        if(!gvb_camera_turn_streaming_off(_camera, error)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

// увеличивать нужно медленней, чем уменьшать - должны быть разные алгоритмы
// определения величины корректировки параметров камеры. Пока всё скопировано 
// из camera_slow_down (кроме step_mult), но алгоритм может быть совершенно другим
static gboolean
camera_speed_up(
      gfloat *tx_bytes_per_sec, gfloat *camera_bytes_per_sec, gfloat *bytes_tolerance, GError **error )
{
    _drop_buffers = FALSE;
    
    if(!gvb_camera_is_streaming_on(_camera)) {
        if(!gvb_camera_turn_streaming_on(_camera, error)) {
            return FALSE;
        }
    }
    
    gint32 step_mult = 0;
    if(*tx_bytes_per_sec >= *camera_bytes_per_sec) {
        step_mult = round((*tx_bytes_per_sec - *camera_bytes_per_sec)/((*bytes_tolerance)));
    }
    else {
        g_set_error(error, GVB_ERROR, GVB_ERROR_ILLEGAL_STATE
                , "camera_speed_up: unexpected tx_bytes_per_sec < camera_bytes_per_sec");
        return FALSE;
    }
    
    if(step_mult < 0) {
        step_mult = 1;
    }
    
    gint32 ctl_value = 0;
    gint32 ctl_value_new = 0;
    gint32 i = 0;
    for(i=G_N_ELEMENTS(controls)-1; i>=0; i--) 
    {
        if(!gvb_camera_get_control(_camera, error, controls[i].id, &ctl_value)) {
            return FALSE;
        }
        
        if(controls[i].id==GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD) {
            continue;
        }
        
        ctl_value_new = ctl_value;
        if(controls[i].rtt_influence > 0){
            ctl_value_new = MIN(ctl_value+step_mult*controls[i].step, controls[i].max); // увеличиваем
        }
        else if(controls[i].rtt_influence < 0){
            ctl_value_new = MAX(ctl_value-step_mult*controls[i].step, controls[i].min); // уменьшаем
        }

        if(ctl_value_new!=ctl_value) {
            if(!gvb_camera_set_control(_camera, error, controls[i].id, &ctl_value_new)){
                return FALSE;
            }
            controls[i].value = ctl_value_new;
            return TRUE;
        }
    }
    
    _max_quality = TRUE;
    
    return TRUE;
}

static gboolean 
timeout_cb(gpointer user_data) 
{
    GError *error = NULL;
    
    // Проверить размер очереди отправки, поместятся ли туда наши данные
    //    NOTE: не так всё просто с очередями (очередь драйвера, QDisc, Class, Filter)
    //      Более универсальный способ, наверно, всё-таки поддерживать целевой rtt
    //      потому что есть ли очередь, её размер очень сильно зависят от настроек
    //      сетевого стека и это может меняться в процессе выполенения

    // rtt в ms
    static guint32 snd_rtt=0, rcv_rtt=0, snd_mss=0, rcv_mss=0;
    if(!gvb_client_get_rtt(_client, &snd_rtt, &rcv_rtt, &snd_mss, &rcv_mss, &error)) {
        gvb_log_error(&error);
        return TRUE;
    }
    
    GvbCameraStatistics camera_stats = {0};
    if(!gvb_camera_get_statistics(_camera, &error, &camera_stats)) {
        gvb_log_error(&error);
        return TRUE;
    }
    
    // сколько пакетов в секунду мы можем пересылать при таком rtt?
    gfloat tx_pkg_per_sec = 1000000.0/snd_rtt;
    // сколько байт в секунду мы можем пересылать при таком rtt?
    gfloat tx_bytes_per_sec = tx_pkg_per_sec*rcv_mss;
    // сколько байт в секунду генерирует камера?
    // здесь нужно подождать минимум 2 секунды
    static gint64 start_time = 0;
    static gboolean stable = FALSE;
    if(!stable) {
        if(start_time==0) {
            start_time = g_get_monotonic_time();
            return TRUE;
        }
        else {
            if(g_get_monotonic_time()-start_time < 3*G_TIME_SPAN_SECOND) {
                return TRUE;
            }
            else {
                stable = TRUE;
            }
        }
    }
    gfloat camera_bytes_per_sec = camera_stats.bytes_per_second_avg2/2.0;
    // мы должны иметь запас между camera_bytes_per_sec и tx_bytes_per_sec
    gfloat bytes_handicap = tx_bytes_per_sec/10.0;
    tx_bytes_per_sec = tx_bytes_per_sec - bytes_handicap;
    // на какое изменение bytes_per_sec мы не будем реагировать
    //gfloat bytes_tolerance = bytes_handicap / 2.0;
    gfloat bytes_tolerance = tx_bytes_per_sec / 10; // 10000.0; // 10 KB
    // tx channel vs camera bitrate ratio
    _bitrate_ratio = tx_bytes_per_sec / camera_bytes_per_sec;
    
    const char *indicator = NULL;
    if((camera_bytes_per_sec > tx_bytes_per_sec + bytes_tolerance) && !_drop_buffers ) {
        // уменшить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        indicator = "< ";
        if(!camera_slow_down(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
        }
    }
    else if ((camera_bytes_per_sec < tx_bytes_per_sec - bytes_tolerance) && !_max_quality) {
        // увеличить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        indicator = " >";
        if(!camera_speed_up(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
        }
    }
    else {
//        gvb_log_message("%f <> %f +- %f, [%d]", camera_bytes_per_sec, tx_bytes_per_sec, bytes_tolerance, _max_quality);
        indicator = " =";
    }
    gvb_log_message("%s: "
            "bitrate=[%d] fps=[%d] exp_time=[%d] iframe=[%d] "
            "s_rtt=[%d] s_mss=[%d] r_rtt=[%d] r_mss=[%d] "
            "tx_Bps=[%f] camera_Bps=[%f]"
          , indicator
          , BTR_CTL.value, FPS_CTL.value, EXP_CTL.value, IFR_CTL.value
          , snd_rtt, snd_mss, rcv_rtt, rcv_mss
          , tx_bytes_per_sec, camera_bytes_per_sec
          );
    return TRUE;
}

static gboolean
camera_query_controls(GvbCamera *camera, GError **error)
{
    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE
            , &(FPS_CTL.min), &(FPS_CTL.max), &(FPS_CTL.step), &(FPS_CTL.deflt), error)) {
        return FALSE;
    }
    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME
            , &(EXP_CTL.min), &(EXP_CTL.max), &(EXP_CTL.step), &(EXP_CTL.deflt), error)) {
        return FALSE;
    }
    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE
            , &(BTR_CTL.min), &(BTR_CTL.max), &(BTR_CTL.step), &(BTR_CTL.deflt), error)) {
        return FALSE;
    }
    if(!gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
            , &(IFR_CTL.min), &(IFR_CTL.max), &(IFR_CTL.step), &(IFR_CTL.deflt), error)) {
        return FALSE;
    }
    
    if(!gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_FRAME_RATE, &FPS_CTL.value)) {
        return FALSE;
    }
    if(!gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &EXP_CTL.value)) {
        return FALSE;
    }
    if(!gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_BITRATE, &BTR_CTL.value)) {
        return FALSE;
    }
    if(!gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &IFR_CTL.value)) {
        return FALSE;
    }
    return TRUE;
}

static int
run_client(int argc, char** argv)
{    
    g_option_context_add_group(_opt_ctx, gvb_client_opt_get_group(&_cli_opts));
    g_option_context_add_group(_opt_ctx, gvb_camera_opt_get_group(&_cam_opts));
    
    GError *error = NULL;
    if(!g_option_context_parse(_opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GMainContext) main_ctx = NULL;
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    _loop = g_main_loop_new(main_ctx, TRUE);
    
    _client = gvb_client_new(&_nwk_opts, &_cli_opts, main_ctx, &error);
    if(!_client) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    if(!gvb_client_connect(_client, cancellable, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    _camera = gvb_camera_new(&_cam_opts);
    
    if(!gvb_camera_set_buffer_callback(_camera, camera_cb, g_object_ref(_client), camera_destroy_cb)) {
        gvb_log_critical("gvb_camera_set_buffer_callback fails");
        return EXIT_FAILURE;
    }
    
    if(!gvb_camera_open(_camera, &error)){
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    if(!camera_query_controls(_camera, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    if(!gvb_camera_start_capturing(_camera, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    gint32 ctl_value = 1;
    if(!gvb_camera_set_control(_camera, &error, GVB_CAMERA_CONTROL_INLINE_HEADER, &ctl_value)) {
        gvb_log_error(&error);
    }
    gvb_log_message("set GVB_CAMERA_CONTROL_INLINE_HEADER = 1");
    ctl_value = 1;
    if(!gvb_camera_set_control(_camera, &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &ctl_value)) {
        gvb_log_error(&error);
    }
    gvb_log_message("set GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD = 1");
    
#if UI
    if(!gvb_ui_start(_camera, NULL, _client, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#endif
    
    GSource *timeout_source = g_timeout_source_new(100);
    g_source_set_callback(timeout_source, timeout_cb, NULL, NULL);
    g_source_attach(timeout_source, main_ctx);
    
    g_main_loop_run(_loop);
    return EXIT_SUCCESS;
}

static int
run_server(int argc, char** argv)
{
    static GvbServerOptions srv_opts = {};
    
    g_option_context_add_group(_opt_ctx, gvb_server_opt_get_group(&srv_opts));
    
    GError *error = NULL;
    if(!g_option_context_parse(_opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GMainContext) main_ctx = NULL;
    _loop = g_main_loop_new(main_ctx, TRUE);
    
    _server = gvb_server_new(&_nwk_opts, &srv_opts, main_ctx, &error);
    if(!_server) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    // set client connection callback
    if(!gvb_server_set_client_connect_cb(_server, client_connect_cb, _server, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    if(!gvb_server_set_input_data_cb(_server, client_input_data_cb, _server, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
#if UI
    if(!gvb_ui_start(NULL, _server, NULL, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#endif
    
    g_main_loop_run(_loop);
    
    if(_client_data_fd>=0) {
        if(!g_close(_client_data_fd, &error)) {
            gvb_log_error(&error);
        }
        else {
            _client_data_fd = -1;
        }
    }
    
    return EXIT_SUCCESS;
}

static int
switch_mode(int argc, char** argv)
{
    _opt_ctx = g_option_context_new("- Glavbot Camera application");
    g_option_context_add_group(_opt_ctx, gvb_network_opt_get_group(&_nwk_opts));
    g_option_context_set_ignore_unknown_options(_opt_ctx, TRUE);
    
    GError *error = NULL;
    if(!g_option_context_parse(_opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    gint retval = EXIT_FAILURE;
    switch(_nwk_opts.mode) {
        case GVB_NETWORK_MODE_CLIENT:
            retval = run_client(argc, argv);
            break;
        case GVB_NETWORK_MODE_SERVER:
            retval = run_server(argc, argv);
            break;
        case GVB_NETWORK_MODE_INVALD:
            break;
    }
    
    g_option_context_free(_opt_ctx);
    _opt_ctx = NULL;
    
    return retval;
}

int
test_client2server(int argc, char** argv)
{   
    g_unix_signal_add(SIGINT, sigint_handler, NULL);
    return switch_mode(argc, argv);
}
