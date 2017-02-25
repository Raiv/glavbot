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
    gint64  start_time; // время начала захвата картинки с камеры
    gint64  analyzed_us; // интервал времени в мкс между двумя wrap-time
    gint64  analyzed_samples; // количество семплов между двумя wrap-time
    guint32 bytes_all;
    guint32 buffers_all;
    gdouble bytes_per_second_avg0; // мгновенная скорость генерации байтов
    gdouble bytes_per_second_avg0_5;
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

/**
 * Конструктор инстанса
 * 
 * @param options - параметры создания инстанса
 * @return созданный инстанс или NULL в случае ошибки
 */
GvbCamera*
gvb_camera_new(GvbCameraOptions *options);

/**
 * Открыто ли системное устройство?
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return - открыта ли камера? В случае ошибки будет установлен указатель error, в этом случае результат функции не гарантируется
 */
gboolean
gvb_camera_isopen(GvbCamera *self, GError **error);

/**
 * Получение системного пути v4l2-устройства
 * 
 * @param self - инстанс
 * @param path - системный путь устройства камеры
 * @param error - описание ошибки
 * @return TRUE если указатель path успешно установлен. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_get_dev_path(GvbCamera *self, gchar **path, GError **error);

/**
 * @param self - инстанс
 * @param state - состояние камеры
 * @param error - описание ошибки
 * @return TRUE если указатель state успешно установлен. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_get_state(GvbCamera *self, GvbCameraState *state, GError **error);

/**
 * Установка колбэков для обработки событий получения буферов и финализации инстанса
 * 
 * @param self - инстанс
 * @param callback - функция обработки буферов
 * @param user_data - пользователькие данные, которые будут передаваться в функцию callback и destroy_cb
 * @param destroy_cb - функция обработки финализации инстанса
 * @param error - описание ошибки
 * @return TRUE если callback-и успешно установлены. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_set_buffer_callback(GvbCamera *self, GvbCameraBufferCbFn callback, gpointer user_data, GDestroyNotify destroy_cb, GError **error);

/**
 * Открытие системного устройства камеры. Должно быть вызвано перед запуском захвата данных и перед попыткой установки параметров камеры
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE если устройство успешно открыто. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_open(GvbCamera *self, GError **error);

/**
 * Закрытие системного устройства камеры. Должно быть вызвано после окончания работы с камерой
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE если устройство успешно закрыто. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_close(GvbCamera *self, GError **error);

/**
 * Запуск стримминга
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE если стримминг включён. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_turn_streaming_on(GvbCamera *self, GError **error);

/**
 * Остановка стримминга
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE если стримминг остановлен. FALSE - если произошла ошибка и установлен указатель error
 */
gboolean
gvb_camera_turn_streaming_off(GvbCamera *self, GError **error);

/**
 * Информация ою активности процессе стримминга
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return включён ли стримминг? В случае ошибки будет установлен указатель error, 
 *   в этом случае возвращаемое значение функции не гарантируется
 */
gboolean
gvb_camera_is_streaming_on(GvbCamera *self, GError **error);

/**
 * Включение процесса обработки буферов v4l2 устройства
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return - TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean
gvb_camera_start_capturing(GvbCamera *self, GError **error);

/**
 * Остановка процесса обработки буферов v4l2 устройства
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean
gvb_camera_stop_capturing(GvbCamera *self, GError **error);

/**
 * Сброс настроек устройства в состояние по умолчанию
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean 
gvb_camera_default_control(GvbCamera *self, GError **error);

/**
 * Получение метаинформации о настроечном параметре
 * 
 * @param self - инстанс
 * @param control - идентификатор настройки
 * @param min - минимальное допустимое значение настройки
 * @param max - максимальное допустимое значние настройки
 * @param step - шан изменения настройки
 * @param deflt - значение по умолчанию для настройки
 * @param error - описание ошибки
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean
gvb_camera_query_control(GvbCamera *self, GvbCameraControl control
  , gint32 *min, gint32 *max, gint32 *step, gint32 *deflt
  , GError **error );

/**
 * Установка параметра устройства
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @param control - идентификатор параметра
 * @param value - значение параметра
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean
gvb_camera_set_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value);

/**
 * Получение значения параметра
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @param control - идентификатор параметра
 * @param value - значение параметра
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 */
gboolean
gvb_camera_get_control(GvbCamera *self, GError **error, GvbCameraControl control, gint32 *value);

/**
 * Получение статистической информации
 * 
 * @param self - инстанс
 * @param error - описание ошибки
 * @param statistics - указатель на возвращаемую структуру
 * @return TRUE в случае успешного выполнения. FALSE в случае ошибки и установки указателя error
 *    Память по указателю statistics должна быть освобождена с помощью g_free
 */
gboolean
gvb_camera_get_statistics(GvbCamera *self, GError **error, GvbCameraStatistics *statistics);

#endif /* GVB_CAMERA_H */

