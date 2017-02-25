/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <ncurses.h>
#include "gvb-ui.h"
#include "gvb-log.h"
#include "gvb-server.h"
#include "gvb-client.h"

static volatile gint     running = 0;
static GThread          *ui_thread = NULL;
static GvbCamera        *camera = NULL;
static GvbServer        *sever = NULL;
static GvbClient        *client = NULL;

static GvbCameraFrameRate   frame_rate;
static guint32              exposure_time;
static guint32              bitrate;
static guint32              iframe_period;

static WINDOW   *log_window = NULL;
static WINDOW   *status_window = NULL;
static WINDOW   *help_window = NULL;

static void
gvb_ui_log_handler (
      const gchar *log_domain
    , GLogLevelFlags log_level
    , const gchar *message
    , gpointer user_data )
{
    wprintw(log_window, "\n%s: %s", log_domain, message);
    wrefresh(log_window);
}

static gpointer
gvb_ui_thread(gpointer user_data)
{
    int inch;
    for(;;) {
        if(!g_atomic_int_get(&running)) {
            break;
        }
        inch = getch();
        if('q'==inch || 'a'==inch) 
        {
            GError *error = NULL;
            if(!gvb_camera_get_control(camera, &error, GVB_CAMERA_CONTROL_FRAME_RATE, &frame_rate)) {
                gvb_log_error(&error);
                continue;
            }
            
            GvbCameraFrameRate frame_rate_new;
            frame_rate_new.numerator = frame_rate.numerator;
            frame_rate_new.denominator = frame_rate.denominator;
            
            if(inch=='q') {
                frame_rate_new.numerator = frame_rate.numerator + frame_rate.denominator;
            }
            else {
                frame_rate_new.numerator = frame_rate.numerator - frame_rate.denominator;
                if(frame_rate_new.numerator<=0) {
                    frame_rate_new.numerator = frame_rate.numerator;
                }
            }
            
            if(frame_rate_new.numerator!=frame_rate.numerator
                    || frame_rate_new.denominator!=frame_rate.denominator)
            {
                if(!gvb_camera_set_control(camera, &error, GVB_CAMERA_CONTROL_FRAME_RATE, &frame_rate_new)) {
                    gvb_log_error(&error);
                    continue;
                }
            }
        }
        else if('w'==inch || 's'==inch) 
        {
            GError *error = NULL;
            if(!gvb_camera_get_control(camera, &error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &exposure_time)) {
                gvb_log_error(&error);
                continue;
            }
            
            if(inch=='w') {
                exposure_time += 20;
            }
            else {
                if(exposure_time>20) {
                    exposure_time -= 20;
                }
            }
            
            if(!gvb_camera_set_control(camera, &error, GVB_CAMERA_CONTROL_EXPOSURE_TIME, &exposure_time)) {
                gvb_log_error(&error);
                continue;
            }
        }
        else if ('e'==inch || 'd'==inch) {
            GError *error = NULL;
            if(!gvb_camera_get_control(camera, &error, GVB_CAMERA_CONTROL_BITRATE, &bitrate)) {
                gvb_log_error(&error);
                continue;
            }
            if(inch=='e') {
                bitrate += 25000;
            }
            else {
                if(bitrate>25000) {
                    bitrate -= 25000;
                }
            }
            if(!gvb_camera_set_control(camera, &error, GVB_CAMERA_CONTROL_BITRATE, &bitrate)) {
                gvb_log_error(&error);
                continue;
            }
        }
        else if ('r'==inch || 'f'==inch) {
            GError *error = NULL;
            if(!gvb_camera_get_control(camera, &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &iframe_period)) {
                gvb_log_error(&error);
                continue;
            }
            if(inch=='r') {
                iframe_period += 10;
            }
            else {
                if(iframe_period>10) {
                    iframe_period -= 10;
                }
            }
            if(!gvb_camera_set_control(camera, &error, GVB_CAMERA_CONTROL_INTER_FRAME_PERIOD, &iframe_period)) {
                gvb_log_error(&error);
                continue;
            }
        }
        
        wclear(status_window);
        wprintw(status_window, "\nframerate = [%d/%d]", frame_rate.numerator, frame_rate.denominator);
        wprintw(status_window, "\nexposure_time = [%d]", exposure_time);
        wprintw(status_window, "\nbitrate = [%d]", bitrate);
        wprintw(status_window, "\ninter frame period = [%d]", iframe_period);
        wrefresh(status_window);
    }
}

gboolean
gvb_ui_start(GvbCamera *pcamera, GvbServer *server, GvbClient *client, GError **error)
{
    if(g_atomic_int_get(&running)) {
        return TRUE;
    }
    
    camera = pcamera;
    server = server;
    client = client;
    
    // Log handler
    g_log_set_default_handler(gvb_ui_log_handler, NULL);
    
    // UI init
    initscr();
    noecho();
    
    log_window = newwin(15, 0, 1, 0);
    scrollok(log_window, TRUE);
    
    mvhline(16, 0, ACS_BULLET, getmaxx(stdscr));
    mvvline(16, 51, ACS_NEQUAL, 30);
    
    status_window = newwin(15, 50, 17, 1);
    scrollok(status_window, TRUE);
    wprintw(status_window, " status: ");
    
    help_window = newwin(15, 70, 17, 52);
    scrollok(help_window, TRUE);
    wprintw(help_window, " help: ");
    wprintw(help_window, "\n q/a - increase, decrease framerate");
    wprintw(help_window, "\n w/s - increase, decrease exposure time (step 20)");
    wprintw(help_window, "\n e/d - increase, decrease bitrate (step 25000)");
    wprintw(help_window, "\n r/f - increase, decrease inter frame period (step 10)");
    
    refresh();
    wrefresh(log_window);
    wrefresh(status_window);
    wrefresh(help_window);
    
    // Input hadler
    g_atomic_int_set(&running, 1);
    ui_thread = g_thread_new("ui", gvb_ui_thread, NULL);
    return TRUE;
}

gboolean
gvb_ui_stop(GError **error)
{
    if(!g_atomic_int_get(&running)) {
        return TRUE;
    }
    // Stop input thread
    if(ui_thread) {
        g_atomic_int_set(&running, 0);
        g_thread_join(ui_thread);
        ui_thread = NULL;
    }
    // Clear log handler
    g_log_set_default_handler(g_log_default_handler, NULL);
    // Clear UI
    endwin();
    
    return TRUE;
}
