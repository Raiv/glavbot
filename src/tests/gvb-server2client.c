#include "gvb-tests.h"
#include "gvb-log.h"
#include "gvb-client.h"
#include "gvb-server.h"
#include "gvb-camera.h"
#include "gvb-ui.h"
#include "gvb-error.h"

#include <glib-unix.h>
#include <stdlib.h>
#include <math.h>

#define UI TRUE

static GvbNetworkOptions _nwk_opts = {0};
static GvbCameraOptions  _cam_opts = {0};
static GvbServerOptions  _srv_opts = {};
static GvbClientOptions  _cli_opts = {};

static GOptionContext *_opt_ctx = NULL;
static GMainLoop      *_loop    = NULL;
static GvbServer      *_server  = NULL;
//static GvbClient      *_client  = NULL;
static GvbCamera      *_camera  = NULL;

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
} Control;

// элементы массива расположены в порядке приоритета
// сначала изменяется первый; если уперлись в min или max, только тогда берёмся
// за второй, потом за третий и т.д.
static Control controls[] = { 
    { .id=GVB_CAMERA_CONTROL_BITRATE, .rtt_influence=1,  .min=0, .max=0, .step=0, .deflt=0 }
  , { .id=GVB_CAMERA_CONTROL_FRAME_RATE, .rtt_influence=1,  .min=0, .max=0, .step=0, .deflt=0 }
  , { .id=GVB_CAMERA_CONTROL_EXPOSURE_TIME, .rtt_influence=-1, .min=0, .max=0, .step=0, .deflt=0 }
  , { .id=GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, .rtt_influence=-1, .min=0, .max=0, .step=0, .deflt=0 }
};

#define BTR_CTL (controls[0])
#define FPS_CTL (controls[1])
#define EXP_CTL (controls[2])
#define IFR_CTL (controls[3])

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
            return TRUE;
        }
        else {
            gvb_log_message("ctl equals: id=[%d] infl=[%d] ctl_val=[%d] min=[%d] max=[%d]"
                    , controls[i].id
                    , controls[i].rtt_influence
                    , ctl_value
                    , controls[i].min
                    , controls[i].max
                    );
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
        
        ctl_value_new = ctl_value;
        if(controls[i].rtt_influence > 0){
            ctl_value_new = MIN(ctl_value+step_mult*controls[i].step, controls[i].max); // увеличиваем
        }
        else if(controls[i].rtt_influence < 0){
            ctl_value_new = MAX(ctl_value-step_mult*controls[i].step, controls[i].min); // уменьшаем
        }

        if(ctl_value_new!=ctl_value) {
            gvb_log_message("set control: [id=%d] [new_val=%d, old_value=%d] [mult=%d, step=%d]"
                    , controls[i].id
                    , ctl_value_new, ctl_value
                    , step_mult, controls[i].step);
            if(!gvb_camera_set_control(_camera, error, controls[i].id, &ctl_value_new)){
                return FALSE;
            }
            return TRUE;
        }
        else {
            gvb_log_message("ctl equals: infl=[%d], ctl_val=[%d], min=[%d], max=[%d]"
                , controls[i].rtt_influence
                , ctl_value
                , controls[i].min
                , controls[i].max
                );
        }
    }
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
    if(!gvb_server_get_rtt(_server, &snd_rtt, &rcv_rtt, &snd_mss, &rcv_mss, &error)) {
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
    gfloat bytes_tolerance = 10000.0; // 10 KB
    
    if(camera_bytes_per_sec > tx_bytes_per_sec + bytes_tolerance ) {
        // уменшить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        gvb_log_message("<<<: "
                "snd_rtt=[%d] snd_mss=[%d] rcv_rtt=[%d] rcv_mss=[%d] "
                "tx_bytes_per_sec=[%f] bytes_tolerance=[%f] camera_bytes_per_sec=[%f]"
              , snd_rtt, snd_mss, rcv_rtt, rcv_mss
              , tx_bytes_per_sec, bytes_tolerance, camera_bytes_per_sec
              );
        if(!camera_slow_down(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
            return TRUE;
        }
    }
    else if (camera_bytes_per_sec < tx_bytes_per_sec - bytes_tolerance) {
        // увеличить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        gvb_log_message(">>>: "
                "snd_rtt=[%d] snd_mss=[%d] rcv_rtt=[%d] rcv_mss=[%d] "
                "tx_bytes_per_sec=[%f] bytes_tolerance=[%f] camera_bytes_per_sec=[%f]"
              , snd_rtt, snd_mss, rcv_rtt, rcv_mss
              , tx_bytes_per_sec, bytes_tolerance, camera_bytes_per_sec
              );
        if(!camera_speed_up(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
            return TRUE;
        }
    }
    else {
        gvb_log_message("===: speed ok");
    }
    
    return TRUE;
}

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
    GvbServer *server = GVB_SERVER(user_data);
    g_autoptr(GSocketConnection) cconn = NULL;
    
    // pre send checks & actions
    // FIXME: нужно вызывать на регулярной основе, а не так
    // иначе, если скорость падает очень сильно, то вызовы этого метода происходят очень редко
//    if(!pre_send(server, bytes, &error)) {
//        gvb_log_error(&error);
//        return;
//    }
    
    // cconn must be g_object_unrefed (see it autoptr)
    if(!gvb_server_get_client_connection(server, &cconn, &error)) {
        gvb_log_error(&error);
        return;
    }
    if(!cconn) {
        gvb_log_info("no client connection");
        return; // no client connection yet
    }
    GIOStream *io_stream = G_IO_STREAM(cconn);
    GOutputStream *o_stream = g_io_stream_get_output_stream(io_stream);
    gssize wsize = -1;
    if((wsize=g_output_stream_write(o_stream, start, bytes, NULL, &error))<0) {
        gvb_log_error(&error);
        return;
    }
}

static void 
camera_destroy_cb(gpointer data)
{
    g_object_unref(GVB_SERVER(data));
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
    return TRUE;
}

static void
client_connect_cb(gboolean connected, gpointer user_data)
{
    GError *error = NULL;
    GvbServer *server = GVB_SERVER(user_data);
    
    if(connected) {
        g_autoptr(GvbCamera) camera = gvb_camera_new(&_cam_opts);
        if(!gvb_camera_set_buffer_callback(camera, camera_cb, g_object_ref(server), camera_destroy_cb)) {
            gvb_log_critical("gvb_camera_set_buffer_callback fails");
            return;
        }
        if(!gvb_camera_open(camera, &error)){
            gvb_log_error(&error);
            return;
        }
        if(!camera_query_controls(camera, &error)) {
            gvb_log_error(&error);
            return;
        }
        gvb_ui_set_camera(camera, NULL);
        if(!gvb_camera_start_capturing(camera, &error)) {
            gvb_log_error(&error);
            return;
        }
        _camera = g_object_ref(camera);
    }
    else {
        if(!gvb_camera_stop_capturing(_camera, &error)) {
            gvb_log_error(&error);
            return;
        }
#if UI
        gvb_ui_set_camera(NULL, NULL);
#endif
        g_clear_object(&_camera);
    }
}

static int
run_client(int argc, char** argv)
{
    g_option_context_add_group(_opt_ctx, gvb_client_opt_get_group(&_cli_opts));
    GError *error = NULL;
    if(!g_option_context_parse(_opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    g_autoptr(GMainContext) main_ctx = NULL;
    // create client
    g_autoptr(GvbClient) client = gvb_client_new(&_nwk_opts, &_cli_opts, main_ctx, &error);
    if(!client) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    // connect to server
    if(!gvb_client_connect(client, NULL, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#if UI
    // start ui
    gvb_ui_set_client(client, NULL);
    if(!gvb_ui_start(main_ctx, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#endif
    // run main loop
    _loop = g_main_loop_new(main_ctx, TRUE);
    g_main_loop_run(_loop);
    // finish
    return EXIT_SUCCESS;
}

static int
run_server(int argc, char** argv)
{
    g_option_context_add_group(_opt_ctx, gvb_server_opt_get_group(&_srv_opts));
    g_option_context_add_group(_opt_ctx, gvb_camera_opt_get_group(&_cam_opts));
    GError *error = NULL;
    if(!g_option_context_parse(_opt_ctx, &argc, &argv, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    
    }
    g_autoptr(GMainContext) main_ctx = NULL;
    // create server
    _server = gvb_server_new(&_nwk_opts, &_srv_opts, main_ctx, &error);
    if(!_server) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    // set client connection callback
    if(!gvb_server_set_client_connect_cb(_server, client_connect_cb, _server, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#if UI
    // start ui
    gvb_ui_set_server(_server, NULL);
    if(!gvb_ui_start(main_ctx, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#endif
    
    GSource *timeout_source = g_timeout_source_new(100);
    g_source_set_callback(timeout_source, timeout_cb, NULL, NULL);
    g_source_attach(timeout_source, main_ctx);
    
    // start main loop
    _loop = g_main_loop_new(main_ctx, TRUE);
    g_main_loop_run(_loop);
    // finish
    return EXIT_SUCCESS;
}

static int
switch_mode(int argc, char** argv)
{
    _opt_ctx = g_option_context_new("- Glavbot Camera application");
    g_option_context_add_group(_opt_ctx, gvb_network_opt_get_group(&_nwk_opts)); // option group will be freed with context
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
test_server2client(int argc, char** argv)
{   
    g_unix_signal_add(SIGINT, sigint_handler, NULL);
    return switch_mode(argc, argv);
}
