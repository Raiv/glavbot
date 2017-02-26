#include <ncurses.h>
#include <gio/gio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "gvb-ui.h"
#include "gvb-log.h"
#include "gvb-server.h"
#include "gvb-client.h"
#include "gvb-error.h"

G_LOCK_DEFINE_STATIC(log_lock);

static FILE *log_fd;

static volatile gint _running = 0;

struct gvb_ui_data {
    GRecMutex data_lock;
    GMainContext *main_ctx;
    GSource *refresh_ev_src;
    //
    GThread   *ui_thread;
    GvbCamera *camera;
    GvbServer *server;
    GvbClient *client;
    //
    GvbCameraStatistics stats;
    gint32 frame_rate;
    gint32 exposure_time;
    gint32 bitrate;
    gint32 iframe_period;
};

static struct gvb_ui_data _data = {{0}};

static WINDOW *_log_window = NULL;
static WINDOW *_status_window = NULL;
static WINDOW *_help_window = NULL;

#define DATA(a) _data.a
#define DATA_LOCK()    g_rec_mutex_lock(&DATA(data_lock))
#define DATA_UNLOCK()  g_rec_mutex_unlock(&DATA(data_lock))

static void
gvb_ui_status_refresh();
static gboolean
gvb_ui_refresh_cb(gpointer user_data);

static void
gvb_ui_log_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    G_LOCK(log_lock);
    if(!_log_window) {
        G_UNLOCK(log_lock);
        g_print("\n%s: %s", log_domain, message);
        return;
    }
    if(log_fd>0) {
        fputs(message, log_fd);
        fputs("\n", log_fd);
    }
    wprintw(_log_window, "\n%s: %s", log_domain, message);
//    wrefresh(_log_window);
    G_UNLOCK(log_lock);
}

static gpointer
gvb_ui_thread(gpointer user_data)
{
    int inch;
    for(;;) {
        if(!g_atomic_int_get(&_running)) {
            break;
        }
        inch = getch();
        if('d'==inch)
        {
            DATA_LOCK();
            gvb_camera_default_control(DATA(camera), NULL);
            DATA_UNLOCK();
        }
        
/*
        G_LOCK(data_lock);
        if(('q'==inch || 'a'==inch) && DATA(camera)) 
        {
            GError *error = NULL;
            if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_FRAME_RATE, &DATA(frame_rate))) {
                gvb_log_error(&error);
                goto continue_label;
            }
            
            GvbCameraFrameRate frame_rate_new;
            frame_rate_new.numerator = DATA(frame_rate).numerator;
            frame_rate_new.denominator = DATA(frame_rate).denominator;
            
            if(inch=='q') {
                frame_rate_new.numerator = DATA(frame_rate).numerator + DATA(frame_rate).denominator;
            }
            else {
                frame_rate_new.numerator = DATA(frame_rate).numerator - DATA(frame_rate).denominator;
                if(frame_rate_new.numerator<=0) {
                    frame_rate_new.numerator = DATA(frame_rate).numerator;
                }
            }
            
            if(frame_rate_new.numerator!=DATA(frame_rate).numerator
                    || frame_rate_new.denominator!=DATA(frame_rate).denominator)
            {
                if(!gvb_camera_set_control(DATA(camera), &error, GVB_CAMERA_CONTROL_FRAME_RATE, &frame_rate_new)) {
                    gvb_log_error(&error);
                    goto continue_label;
                }
            }
        }
        else if(('w'==inch || 's'==inch) && DATA(camera) ) 
        {
            GError *error = NULL;
            if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_EXPOSURE_TIME
                    , &DATA(exposure_time))) {
                gvb_log_error(&error);
                goto continue_label;
            }
            
            if(inch=='w') {
                DATA(exposure_time) += 20;
            }
            else {
                if(DATA(exposure_time)>20) {
                    DATA(exposure_time) -= 20;
                }
            }
            
            if(!gvb_camera_set_control(DATA(camera), &error, GVB_CAMERA_CONTROL_EXPOSURE_TIME
                    , &DATA(exposure_time))) {
                gvb_log_error(&error);
                goto continue_label;
            }
        }
        else if (('e'==inch || 'd'==inch) && DATA(camera) ) {
            GError *error = NULL;
            if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_BITRATE, &DATA(bitrate))) {
                gvb_log_error(&error);
                goto continue_label;
            }
//            gvb_log_message("cur bitrate: %d", DATA(bitrate));
            if(inch=='e') {
                DATA(bitrate) += 1000000;
            }
            else {
                if(DATA(bitrate)>1000000) {
                    DATA(bitrate) -= 1000000;
                }
                else {
                    if(DATA(bitrate)>1) {
                        DATA(bitrate) -= (DATA(bitrate)/2);
                    }
                }
            }
            if(!gvb_camera_set_control(DATA(camera), &error, GVB_CAMERA_CONTROL_BITRATE, &DATA(bitrate))) {
                gvb_log_error(&error);
                goto continue_label;
            }
        }
        else if (('r'==inch || 'f'==inch) && DATA(camera) ) {
            GError *error = NULL;
            if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
                    , &DATA(iframe_period))) {
                gvb_log_error(&error);
                goto continue_label;
            }
            if(inch=='r') {
                DATA(iframe_period) += 10;
            }
            else {
                if(DATA(iframe_period)>10) {
                    DATA(iframe_period) -= 10;
                }
            }
            if(!gvb_camera_set_control(DATA(camera), &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD
                    , &DATA(iframe_period))) {
                gvb_log_error(&error);
                goto continue_label;
            }
        }        
continue_label:
        G_UNLOCK(data_lock);
        gvb_ui_status_refresh();
*/
    }
    return NULL;
}

gboolean
gvb_ui_start(GMainContext *main_ctx, GError **error)
{
    if(g_atomic_int_get(&_running)) {
        return TRUE;
    }
    
    if(log_fd) {
        fclose(log_fd);
        log_fd = NULL;
    }
    if(!(log_fd = fopen("/home/pi/_temp/gvb-log.txt", "ab+"))) {
        g_set_error_literal(error, GVB_ERROR, GVB_ERROR_OTHER, strerror(errno));
        return FALSE;
    }
    
    // Log handler
    g_log_set_default_handler(gvb_ui_log_handler, NULL);
    
    // UI init
    initscr();
    noecho();
    
    _log_window = newwin(15, 0, 1, 0);
    scrollok(_log_window, TRUE);
    
    mvhline(16, 0, ACS_BULLET, getmaxx(stdscr));
    mvvline(16, 51, ACS_NEQUAL, 30);
    
    _status_window = newwin(15, 50, 17, 1);
    scrollok(_status_window, TRUE);
    wprintw(_status_window, " status: ");
    
    _help_window = newwin(15, 70, 17, 52);
    scrollok(_help_window, TRUE);    
    wprintw(_help_window, " help: ");
    
    refresh();
    wrefresh(_log_window);
    wrefresh(_status_window);
    wrefresh(_help_window);
    
    // Input hadler
    g_atomic_int_set(&_running, 1);
    
    DATA_LOCK();
    
    DATA(ui_thread) = g_thread_new("ui", gvb_ui_thread, NULL);
    DATA(main_ctx) = (main_ctx ? g_main_context_ref(main_ctx) : g_main_context_ref_thread_default());
    // start refresh timer
    DATA(refresh_ev_src) = g_timeout_source_new(500);
    g_source_set_callback(DATA(refresh_ev_src), gvb_ui_refresh_cb, NULL, NULL);
    g_source_attach(DATA(refresh_ev_src), DATA(main_ctx));
    
    DATA_UNLOCK();
    return TRUE;
}

gboolean
gvb_ui_stop(GError **error)
{
    if(!g_atomic_int_get(&_running)) {
        return TRUE;
    }
    // Stop input thread
    DATA_LOCK();
    if(DATA(ui_thread)) {
        g_atomic_int_set(&_running, 0);
        g_thread_join(DATA(ui_thread));
        DATA(ui_thread) = NULL;
    }
    g_main_context_unref(DATA(main_ctx));
    // Clear log handler
    g_log_set_default_handler(g_log_default_handler, NULL);
    if(log_fd) {
        fclose(log_fd);
        log_fd = NULL;
    }
    DATA_UNLOCK();
    // Clear UI
    endwin();
    
    return TRUE;
}

gboolean
gvb_ui_set_camera(GvbCamera *camera, GError **error)
{
    DATA_LOCK();
    if(DATA(camera)==camera) {
        DATA_UNLOCK();
        return FALSE;
    }
    if(DATA(camera)) {
        g_clear_object(&DATA(camera));
    }
    if(camera) {
        DATA(camera) = g_object_ref(camera);
    }
    DATA_UNLOCK();
    return TRUE;
}

gboolean
gvb_ui_set_client(GvbClient *client, GError **error)
{
    DATA_LOCK();
    DATA(client) = client;
    DATA_UNLOCK();
    return TRUE;
}

gboolean
gvb_ui_set_server(GvbServer *server, GError **error)
{
    DATA_LOCK();
    DATA(server) = server;
    DATA_UNLOCK();
    return TRUE;
}

static void
gvb_ui_camera_controls_refresh()
{
    GError *error = NULL;
    
    if(!DATA(camera)) {
        return;
    }
    
    DATA_LOCK();
    
    if(!gvb_camera_isopen(DATA(camera), &error)) {
        DATA_UNLOCK();
        gvb_log_error(&error);
        return;
    }

    if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_FRAME_RATE, &DATA(frame_rate))) {
        gvb_log_error(&error);
    }
    if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &DATA(exposure_time))) {
        gvb_log_error(&error);
    }
    if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_BITRATE, &DATA(bitrate))) {
        gvb_log_error(&error);
    }
    if(!gvb_camera_get_control(DATA(camera), &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &DATA(iframe_period))) {
        gvb_log_error(&error);
    }
    
    DATA_UNLOCK();
}

static void
gvb_ui_camera_stats_refresh()
{
    static gint64 stat_time = 0;
    if(stat_time==0 || (g_get_monotonic_time()-stat_time) > G_TIME_SPAN_SECOND/2) {
        stat_time = g_get_monotonic_time();
        GError *error = NULL;
        if(!gvb_camera_get_statistics(DATA(camera), &error, &DATA(stats))) {
            gvb_log_error(&error);
        }
    }
}

static void
gvb_ui_status_refresh()
{
    if(!g_atomic_int_get(&_running)) {
        return;
    }
    
    wclear(_status_window);
    
    DATA_LOCK();
    guint32 snd_rtt = 0, rcv_rtt = 0, snd_mss = 0, rcv_mss = 0;
    if(DATA(client)) {
        gvb_client_get_rtt(DATA(client), &snd_rtt, &rcv_rtt, &snd_mss, &rcv_mss, NULL);
        wprintw(_status_window, "\nsnd_rtt = [%d] ms", snd_rtt);
        wprintw(_status_window, "\nrcv_rtt = [%d] ms", rcv_rtt);
        wprintw(_status_window, "\n----------------------------");
    }
    
    if(DATA(server)) {
        gvb_server_get_rtt(DATA(server), &snd_rtt, &rcv_rtt, &snd_mss, &rcv_mss, NULL);
        wprintw(_status_window, "\nsnd_rtt       = [%d] ms", snd_rtt);
        wprintw(_status_window, "\nrcv_rtt       = [%d] ms", rcv_rtt);
        wprintw(_status_window, "\nsnd_mss       = [%d] bytes", snd_mss);
        wprintw(_status_window, "\nrcv_mss       = [%d] bytes", rcv_mss);
        wprintw(_status_window, "\n----------------------------");
    }
    if(DATA(camera)) {
        // Camera controls status
        gvb_ui_camera_controls_refresh();
        wprintw(_status_window, "\nframerate     = [%d]", DATA(frame_rate));
        wprintw(_status_window, "\nexp time      = [%d]", DATA(exposure_time));
        wprintw(_status_window, "\nbitrate       = [%d]", DATA(bitrate));
        wprintw(_status_window, "\niframe period = [%d]", DATA(iframe_period));
        wprintw(_status_window, "\n----------------------------");
//        // Camera statistic
        gvb_ui_camera_stats_refresh();
        wprintw(_status_window, "\nazd_u = [%d]", DATA(stats).analyzed_us);
        wprintw(_status_window, "\nazd_s = [%d]", DATA(stats).analyzed_samples);
        wprintw(_status_window, "\nball  = [%d]", DATA(stats).bytes_all);
        //wprintw(_status_window, "\navg0  = [%f]", DATA(stats).bytes_per_second_avg0);
        wprintw(_status_window, "\navg2  = [%f]", DATA(stats).bytes_per_second_avg2);
        //wprintw(_status_window, "\navg5  = [%f]", DATA(stats).bytes_per_second_avg5);
        //wprintw(_status_window, "\navg10 = [%f]", DATA(stats).bytes_per_second_avg10);
        //wprintw(_status_window, "\navg20 = [%f]", DATA(stats).bytes_per_second_avg20);
        //wprintw(_status_window, "\navg60 = [%f]", DATA(stats).bytes_per_second_avg60);
    }
    DATA_UNLOCK();
    
    wrefresh(_status_window);
}

static gboolean
gvb_ui_refresh_cb(gpointer user_data)
{
    gvb_ui_refresh();
    return TRUE;
}

//static void
//gvb_ui_refresh_destroy_cb(gpointer user_data)
//{
//    gvb_log_message("gvb_ui_refresh_destroy_cb");
//}

void
gvb_ui_refresh()
{
    gvb_ui_status_refresh();
    wrefresh(_log_window);
}