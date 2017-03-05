#include "gvb-tests.h"
#include "gvb-log.h"
#include "gvb-client.h"
#include "gvb-server.h"
#include "gvb-camera.h"
#include "gvb-error.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <linux/videodev2.h>
#include "gsth264parser.h"
#include "nalutils.h"

#define UI FALSE

static GvbNetworkOptions _nwk_opts = {0};
static GvbCameraOptions  _cam_opts = {0};
static GvbClientOptions  _cli_opts = {};
static GstH264NalParser *_parser   = NULL;
static GBytes           *_nalu_buf = NULL;
static GstH264NalUnit    _nalu;

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
    gfloat volume_influence;
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
    { .id=GVB_CAMERA_CONTROL_BITRATE, .volume_influence=1.0,  .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_FRAME_RATE, .volume_influence=1.0,  .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_EXPOSURE_TIME, .volume_influence=-1.0, .min=0, .max=0, .step=0, .deflt=0, .value=0 }
  , { .id=GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, .volume_influence=0.0, .min=0, .max=0, .step=0, .deflt=0, .value=0 }
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

static const gchar* 
get_nal_type_desc(GstH264NalUnitType type)
{
    const gchar *retval = NULL;
    switch(type) {
        case GST_H264_NAL_UNKNOWN:
            retval = "GST_H264_NAL_UNKNOWN";
            break;
        case GST_H264_NAL_SLICE:
            retval = "GST_H264_NAL_SLICE";
            break;
        case GST_H264_NAL_SLICE_DPA:
            retval = "GST_H264_NAL_SLICE_DPA";
            break;
        case GST_H264_NAL_SLICE_DPB:
            retval = "GST_H264_NAL_SLICE_DPB";
            break;
        case GST_H264_NAL_SLICE_DPC:
            retval = "GST_H264_NAL_SLICE_DPC";
            break;
        case GST_H264_NAL_SLICE_IDR:
            retval = "GST_H264_NAL_SLICE_IDR";
            break;
        case GST_H264_NAL_SEI:
            retval = "GST_H264_NAL_SEI";
            break;
        case GST_H264_NAL_SPS:
            retval = "GST_H264_NAL_SPS";
            break;
        case GST_H264_NAL_PPS:
            retval = "GST_H264_NAL_PPS";
            break;
        case GST_H264_NAL_AU_DELIMITER:
            retval = "GST_H264_NAL_AU_DELIMITER";
            break;
        case GST_H264_NAL_SEQ_END:
            retval = "GST_H264_NAL_SEQ_END";
            break;
        case GST_H264_NAL_STREAM_END:
            retval = "GST_H264_NAL_STREAM_END";
            break;
        case GST_H264_NAL_FILLER_DATA:
            retval = "GST_H264_NAL_FILLER_DATA";
            break;
        case GST_H264_NAL_SPS_EXT:
            retval = "GST_H264_NAL_SPS_EXT";
            break;
        case GST_H264_NAL_PREFIX_UNIT:
            retval = "GST_H264_NAL_PREFIX_UNIT";
            break;
        case GST_H264_NAL_SUBSET_SPS:
            retval = "GST_H264_NAL_SUBSET_SPS";
            break;
        case GST_H264_NAL_DEPTH_SPS:
            retval = "GST_H264_NAL_DEPTH_SPS";
            break;
        case GST_H264_NAL_SLICE_AUX:
            retval = "GST_H264_NAL_SLICE_AUX";
            break;
        case GST_H264_NAL_SLICE_EXT:
            retval = "GST_H264_NAL_SLICE_EXT";
            break;
        case GST_H264_NAL_SLICE_DEPTH:
            retval = "GST_H264_NAL_SLICE_DEPTH";
            break;
        default:
            retval = "<<unknown>>";
            break;
    }
    return retval;
}

static 
gboolean process_nalu(gpointer chunk, guint32 chunk_len, gboolean last)
{
    static GByteArray *ba = NULL;
    static gint scan_result1 = -1, scan_result2 = -1;
    
    gboolean retval = FALSE;
    GstH264ParserResult result;
    
    if(!ba) {
        ba = g_byte_array_new();
    }
    
    // this will copy data memory to internal store
    g_byte_array_append(ba, chunk, chunk_len);
    
    if(scan_result1<0) {
        scan_result1 = scan_for_start_codes(ba->data, ba->len);
    }

    if(scan_result1>=0) {
        scan_result2 = scan_for_start_codes(ba->data+scan_result1+3, ba->len-scan_result1);
        if(scan_result2>=0) {
            scan_result2 += 3;
        }
    }

    if(scan_result1>=0)
    {
        if(scan_result2>0) {
            g_assert_true(scan_result2>scan_result1);
        }
        if(scan_result2>0 || last) {
            memset(&_nalu, 0, sizeof(GstH264NalUnit));
            result = gst_h264_parser_identify_nalu(_parser, ba->data, 0, ba->len, &_nalu);
            if(result==GST_H264_PARSER_OK)
            {
                if(_nalu_buf) {
                    g_clear_pointer(&_nalu_buf, g_bytes_unref);
                }
                _nalu_buf = g_bytes_new_take(g_memdup(ba->data, scan_result2), scan_result2);
                g_byte_array_remove_range(ba, 0, scan_result2);
                scan_result1 = -1;
                scan_result2 = -1;
                retval = TRUE;
            }
            else {
                // gst_h264_parser_identify_nalu does not know how to handle end of stream
                if(last) {
                    _nalu_buf = g_bytes_new_take(g_memdup(ba->data, ba->len), ba->len);
                    retval = TRUE;
                }
                else {
                    g_print("parser failed!\n");
                }
            }
        }
    }
    return retval;
}

static void 
camera_cb(gpointer user_data, gpointer start, guint32 bytes, guint32 flags)
{
    GError *error = NULL;
    GvbClient *client = GVB_CLIENT(user_data);
    gboolean has_nalu = FALSE;
    gint64 time_diff;
    
    static gint64 keep_alive_timeout = 0;
    static gint64 last_tx = 0;
    static gfloat last_ratio = 0.0;
    
    static gboolean post_idr_flag = FALSE;
    static gint     post_idr_seq = 0;
    
    has_nalu = process_nalu(start, bytes, FALSE);
    
    //--------------------------------------------------------------------------
    // Здесь должна начинаться проверка на сброс фрейма
    
    // если не умещаемся в канал
    if(_bitrate_ratio < 1 && _drop_buffers)
    {
        // если ration идёт в рост, то увеличиваем частоту отправки пропорционально
        if(_bitrate_ratio>last_ratio) {
            keep_alive_timeout = keep_alive_timeout*last_ratio/_bitrate_ratio;
        }
        else {
            keep_alive_timeout = G_TIME_SPAN_MILLISECOND*1000;
        }
        
        last_ratio = _bitrate_ratio;
        
        // но периодически мы должны пытаться отправить данные. Иначе rtt не обновится, не смотря на то, что канал может быть уже в порядке
        time_diff = g_get_monotonic_time()-last_tx;
        
        if(has_nalu) 
        {
            if(post_idr_flag)
            {
                if(post_idr_seq>0)
                {
                    // отправить фрейм, уменьшив post_idr_seq
                    post_idr_seq--;
                }
                else
                {
                    post_idr_flag = FALSE;
                }
            }
            if(!post_idr_flag)
            {
                if(_nalu.type!=GST_H264_NAL_SLICE_IDR)
                {
                    if(time_diff < keep_alive_timeout) 
                    {
                        g_clear_pointer(&_nalu_buf, g_bytes_unref);
                        gvb_log_message(
                            "nalu dropped, offset=%-10d, size=%-10d, ref_idc=%-2d, type=%-25s(%-2d)"
                            ", idr_pic_flag=%d, valid=%d, header_bytes=%-2d"
                            ", extension_type=%-2d"
                          , _nalu.offset
                          , _nalu.size
                          , _nalu.ref_idc
                          , get_nal_type_desc(_nalu.type) // GstH264NalUnitType
                          , _nalu.type
                          , _nalu.idr_pic_flag
                          , _nalu.valid
                          , _nalu.header_bytes
                          , _nalu.extension_type
                        );
                        return;
                    }
                }
                else
                {
                    post_idr_flag = TRUE;
                    post_idr_seq = 15;
                }
            }
        }
    }
    
    last_ratio = _bitrate_ratio;
    
    //--------------------------------------------------------------------------
    // Здесь начинается отсылка nalu
    
    if(!has_nalu) {
        return;
    }
    
    GOutputStream *ostream = NULL;
    if(!gvb_client_get_output_stream(client, &ostream, &error)) {
        gvb_log_error(&error);
        return;
    }
    
    gssize wsize = -1;
    gsize nsize = -1;
    gconstpointer ndata = g_bytes_get_data(_nalu_buf, &nsize);
    if((wsize=g_output_stream_write(ostream, ndata, nsize, NULL, &error))<0) {
        gvb_log_error(&error);
        if(error && error->code==G_IO_ERROR_BROKEN_PIPE) {
            if(!gvb_camera_turn_streaming_off(_camera, &error)) {
                gvb_log_error(&error);
            }
        }
        g_clear_pointer(&_nalu_buf, g_bytes_unref);
        return;
    }
    g_clear_pointer(&_nalu_buf, g_bytes_unref);
    last_tx = g_get_monotonic_time();
    
    gvb_log_message("nalu sent   , offset=%-10d, size=%-10d, ref_idc=%-2d, type=%-25s(%-2d)"
          ", idr_pic_flag=%d, valid=%d, header_bytes=%-2d"
          ", extension_type=%-2d"
      , _nalu.offset
      , _nalu.size
      , _nalu.ref_idc
      , get_nal_type_desc(_nalu.type) // GstH264NalUnitType
      , _nalu.type
      , _nalu.idr_pic_flag
      , _nalu.valid
      , _nalu.header_bytes
      , _nalu.extension_type
    );
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
    gvb_log_message("client_connect_cb handler: [%d]", connected);
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
        if(home_dir) {            
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
        else {
            gvb_log_warning("can't find home");
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
    static const gint64 print_timeout = 500*G_TIME_SPAN_MILLISECOND;
    // how many bytes server received so far
    static guint32 size_sum = 0;
    // last monotonic time server received at least one byte
    static gint64 last_tm = 0;
    
    // early exit
    if(size<=0) {
        return;
    }
    
    // write to file
    if(_client_data_fd >= 0) {
        gsize wc = size;
        gsize ret = -1;
        while(wc > 0) {
            ret = write(_client_data_fd, data, size);
            if(ret < 0) {
                gvb_log_critical("write client data to file error: [%s]", strerror(errno));
                break;
            }
            else if(!ret) {
                gvb_log_warning("write zero bytes to client data file: [%s]", strerror(errno));
            }
            else {
                wc -= ret;
            }
        }
    }
    
    size_sum += size;
    
    if(!last_tm || (g_get_monotonic_time() - last_tm > print_timeout)) {
        last_tm = g_get_monotonic_time();
        gvb_log_message("client_input_data_cb: receive client data [%d]", size_sum);
    }
}

static gboolean
camera_slow_down(
      gfloat *tx_bytes_per_sec, gfloat *camera_bytes_per_sec, gfloat *bytes_tolerance, GError **error )
{
    // early exit
    if(*camera_bytes_per_sec < *tx_bytes_per_sec) {
        gvb_log_warning("camera_slow_down: unexpected tx_bytes_per_sec(%f) < camera_bytes_per_sec(%f)", *tx_bytes_per_sec, *camera_bytes_per_sec);
        return TRUE;
    }

    // множитель для шага изменения параметра равен отношению между
    //  - отклонением текущего bytes_per_sec от значения, которое может обеспечить сеть
    //  - значению tolerance
    // то есть величина корректировки зависит от того, насколько объём генерируемых
    // камерой данных не соответствует пропускной способности канала в единицах tolerance
    gfloat step_mult = (*camera_bytes_per_sec-*tx_bytes_per_sec)/(*bytes_tolerance);
    gint32 ctl_value = 0;
    gint32 ctl_value_new = 0;
    gint32 i = 0;
    
    if(step_mult<=0) {
        gvb_log_warning("camera_slow_down: unexpected step_mult(%f)", step_mult);
        gvb_log_message("diagnostics1: %f, %f, %f", *camera_bytes_per_sec, *tx_bytes_per_sec, *bytes_tolerance);
        return TRUE;
    }
    
    for(i=0; i<G_N_ELEMENTS(controls); i++) 
    {
        // не смогли получить параметр
        if(!gvb_camera_get_control(_camera, error, controls[i].id, &ctl_value)) {
            return FALSE;
        }
        // volume_influence - параметр, который позволяет задать насколько он влияет на объём генерируемых камерой данных. 
        // Его нужно настроить либо эмпирическим путём или каким-то образом собирать статистику 
        // во время работы программы и определять из неё
        // volume_influence > 0 - значит, что увеличение численного значения параметра приведёт к
        //   увилечению объёма. То есть, если мы увеличим численное значение параметра, у нас 
        //   будет генерироваться больше данных, которые дольше передавать
        // volume_influence < 0 - значит, наоборот, чем меньше численное значение параметра,
        //   тем больше данных будет генерироваться и тем дольше мы их будем передавать
        ctl_value_new = ctl_value + (-1)*round(controls[i].volume_influence*step_mult*controls[i].step);
        // защищаемся от выхода за границы допустимых значений параметра
        if(controls[i].volume_influence > 0){
            ctl_value_new = MAX( ctl_value_new, controls[i].min );
        }
        else if(controls[i].volume_influence < 0){
            ctl_value_new = MIN( ctl_value_new, controls[i].max );
        }
        else {
            ctl_value_new = ctl_value;
        }
        
        if(ctl_value_new!=ctl_value) {
            if(!gvb_camera_set_control(_camera, error, controls[i].id, &ctl_value_new)){
                gvb_log_message("diagnostics2: %d, %f, %f, %d", ctl_value, controls[i].volume_influence, step_mult, controls[i].step);
                return FALSE;
            }
            _max_quality = FALSE;
            controls[i].value = ctl_value_new;
            return TRUE;
        }
    }
    // no controls was set. Droping
    _drop_buffers = TRUE;
    return TRUE;
}

// увеличивать нужно медленней, чем уменьшать - должны быть разные алгоритмы
// определения величины корректировки параметров камеры. Пока всё скопировано 
// из camera_slow_down (кроме step_mult), но алгоритм может быть совершенно другим
static gboolean
camera_speed_up(
      gfloat *tx_bytes_per_sec, gfloat *camera_bytes_per_sec, gfloat *bytes_tolerance, GError **error )
{
    // early exit
    if(*tx_bytes_per_sec < *camera_bytes_per_sec) {
        gvb_log_warning("camera_speed_up: unexpected tx_bytes_per_sec(%f) < camera_bytes_per_sec(%f)", *tx_bytes_per_sec, *camera_bytes_per_sec);
        return TRUE;
    }
    
    _drop_buffers = FALSE;
    
    gfloat step_mult = (*tx_bytes_per_sec - *camera_bytes_per_sec)/((*bytes_tolerance));
    gint32 ctl_value = 0;
    gint32 ctl_value_new = 0;
    gint32 i = 0;
    
    if(step_mult<=0) {
        gvb_log_warning("camera_slow_down: unexpected step_mult(%f)", step_mult);
        gvb_log_message("diagnostics1: %f, %f, %f", *camera_bytes_per_sec, *tx_bytes_per_sec, *bytes_tolerance);
        return TRUE;
    }
    
    for(i=G_N_ELEMENTS(controls)-1; i>=0; i--) 
    {
        if(!gvb_camera_get_control(_camera, error, controls[i].id, &ctl_value)) {
            return FALSE;
        }
        ctl_value_new = ctl_value + round(controls[i].volume_influence*step_mult*controls[i].step);
        // защищаемся от выхода за границы допустимых значений параметра
        if(controls[i].volume_influence > 0){
            ctl_value_new = MIN(ctl_value_new, controls[i].max);
        }
        else if(controls[i].volume_influence < 0){
            ctl_value_new = MAX(ctl_value_new, controls[i].min);
        }
        else {
            ctl_value_new = ctl_value;
        }

        if(ctl_value_new!=ctl_value) {
            if(!gvb_camera_set_control(_camera, error, controls[i].id, &ctl_value_new)){
                gvb_log_message("diagnostics: %d, %f, %f, %d", ctl_value, controls[i].volume_influence, step_mult, controls[i].step);
                return FALSE;
            }
            controls[i].value = ctl_value_new;
            return TRUE;
        }
    }
    // no controls was set
    _max_quality = TRUE;
    return TRUE;
}

static gboolean 
timeout_cb(gpointer user_data) 
{
    GError *error = NULL;
    
    // получаем статистику с камеры
    GvbCameraStatistics camera_stats = {0};
    if(!gvb_camera_get_statistics(_camera, &error, &camera_stats)) {
        gvb_log_error(&error);
        return TRUE;
    }
    // нам нужно знать сколько байт в секунду генерирует камера? 
    // Чтобы этот показатель был более или менее точным, необходимо подождать 
    // какое-то время
    if(g_get_monotonic_time()-camera_stats.start_time < 2*G_TIME_SPAN_SECOND) {
        return TRUE;
    }
    
    // Проверить размер очереди отправки, поместятся ли туда наши данные
    //    NOTE: не так всё просто с очередями (очередь драйвера, QDisc, Class, Filter)
    //      Более универсальный способ, наверно, всё-таки поддерживать целевой rtt
    //      потому что есть ли очередь, её размер очень сильно зависят от настроек
    //      сетевого стека и это может меняться в процессе выполенения

    // получаем rtt в ms от клиента
    static guint32 snd_rtt=0, rcv_rtt=0, snd_mss=0, rcv_mss=0;
    if(!gvb_client_get_rtt(_client, &snd_rtt, &rcv_rtt, &snd_mss, &rcv_mss, &error)) {
        gvb_log_error(&error);
        return TRUE;
    }
    
    /** 
     * имеем следующий алгоритм:
     * C - сколько байт генерирует камера в секунду
     * T - сколько байт в секунду позволяет нам передавать канал
     * H - запас в пропускной способности канала в сторону уменьшения T 
     *      (алгоритм определения T не идеален, поэтому надо заложить некий "запас"
     *       так как мы пессимисты)
     * t - шаг изменения T при котором мы меняем параметры камеры
     * 
     * если C > T-H+t то
     *   уменьшаем C
     * иначе если C < T-H-t то
     *   увеличиваем C
     * иначе
     *   ничего не меняем
     * конец если
     */
    
    // сколько сегментов в секунду мы можем пересылать при таком rtt?
    gfloat tx_pkg_per_sec = 1000000.0/snd_rtt;
    // сколько байт в секунду мы можем пересылать при таком rtt?
    // mss - maximum segment size, может меняться. Но, чтобы хоть как-то оценить, 
    // берём текущее значение
    gfloat tx_bytes_per_sec = tx_pkg_per_sec*rcv_mss;
    // сколько байт камера в среднем генерирует в секунду
    gfloat camera_bytes_per_sec = 2.0*camera_stats.bytes_per_second_avg0_5;
    // мы должны иметь запас tx_bytes_per_sec
    tx_bytes_per_sec = 0.9*tx_bytes_per_sec;
    // на какое изменение bytes_per_sec мы не будем реагировать
    gfloat bytes_tolerance = tx_bytes_per_sec / 30;
    // tx channel and camera bitrate ratio
    _bitrate_ratio = tx_bytes_per_sec / camera_bytes_per_sec;
    
    const char *indicator = NULL;
    if((camera_bytes_per_sec > tx_bytes_per_sec + bytes_tolerance) && !_drop_buffers ) {
        // уменшить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        indicator = "< ";
        // DOUBT: передаём текущее значение bytes_per_second для камеры, чтобы корректировать
        // параметры на основе актуальных значений, а не статистических
//        gfloat cbps = 2.0*camera_stats.bytes_per_second_avg0_5;
        tx_bytes_per_sec += bytes_tolerance;
        if(!camera_slow_down(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
        }
    }
    else if ((camera_bytes_per_sec < tx_bytes_per_sec - bytes_tolerance) && !_max_quality) {
        // увеличить объём генерируемых камерой данных до (tx_bytes_per_sec-bytes_handicap)
        indicator = " >";
        // DOUBT: передаём текущее значение bytes_per_second для камеры, чтобы корректировать
        // параметры на основе актуальных значений, а не статистических
//        gfloat cbps = 2.0*camera_stats.bytes_per_second_avg0_5;
        tx_bytes_per_sec -= bytes_tolerance;
        if(!camera_speed_up(&tx_bytes_per_sec, &camera_bytes_per_sec, &bytes_tolerance, &error)) {
            gvb_log_error(&error);
        }
    }
    else {
        indicator = "==";
    }
    if(indicator) {
//        gvb_log_message("%s", indicator);
//        gvb_log_message("%s: "
//                "bitrate=[%10d] fps=[%3d] exp_time=[%3d] iframe=[%2d] "
//                "s_rtt=[%7d] s_mss=[%4d] r_rtt=[%7d] r_mss=[%4d] "
//                "tx_Bps=[%12.3f] camera_Bps=[%12.3f]"
//              , indicator
//              , BTR_CTL.value, FPS_CTL.value, EXP_CTL.value, IFR_CTL.value
//              , snd_rtt, snd_mss, rcv_rtt, rcv_mss
//              , tx_bytes_per_sec, camera_bytes_per_sec
//              );
    }
    return TRUE;
}

static gboolean
camera_query_controls(GvbCamera *camera, GError **error)
{
    if(    !gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_FRAME_RATE, &(FPS_CTL.min), &(FPS_CTL.max), &(FPS_CTL.step), &(FPS_CTL.deflt), error)
        || !gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &(EXP_CTL.min), &(EXP_CTL.max), &(EXP_CTL.step), &(EXP_CTL.deflt), error)
        || !gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_BITRATE, &(BTR_CTL.min), &(BTR_CTL.max), &(BTR_CTL.step), &(BTR_CTL.deflt), error)
        || !gvb_camera_query_control(camera, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &(IFR_CTL.min), &(IFR_CTL.max), &(IFR_CTL.step), &(IFR_CTL.deflt), error)
        || !gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_FRAME_RATE, &FPS_CTL.value)
        || !gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &EXP_CTL.value)
        || !gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_BITRATE, &BTR_CTL.value)
        || !gvb_camera_get_control(camera, error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &IFR_CTL.value)
    ){
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
    
    // connect client to destination
    if(!gvb_client_connect(_client, cancellable, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    // create camera object
    _camera = gvb_camera_new(&_cam_opts);
    if(!gvb_camera_set_buffer_callback(_camera, camera_cb, g_object_ref(_client), camera_destroy_cb, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
    // create parser object
    _parser = gst_h264_nal_parser_new();
    if(!_parser) {
        gvb_log_critical("gst_h264_nal_parser_new fails");
        return EXIT_FAILURE;
    }

    // prepare camera for capturing
    if(!gvb_camera_open(_camera, &error)){
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    if(!camera_query_controls(_camera, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    gint32 ctl_value = 1;
    if(!gvb_camera_set_control(_camera, &error, GVB_CAMERA_CONTROL_INLINE_HEADER, &ctl_value)) {
        gvb_log_error(&error);
    }
//    ctl_value = 1;
//    if(!gvb_camera_set_control(_camera, &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &ctl_value)) {
//        gvb_log_error(&error);
//    }
    
    // kick start camera capturing
    if(!gvb_camera_start_capturing(_camera, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
    
#if UI
    if(!gvb_ui_start(_camera, NULL, _client, &error)) {
        gvb_log_error(&error);
        return EXIT_FAILURE;
    }
#endif
    
    // add timeout event to control data flow
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
