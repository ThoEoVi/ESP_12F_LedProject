#ifndef __HTTP_SERVER_H
#define __HTTP_SERVER_H


#include <sys/param.h>
#include <esp_http_server.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"



typedef void (*get_data_callback_t) ( void );
typedef void (*post_data_callback_t) (char *data, uint16_t len);
typedef void (*post_callback_t) ( void );

void start_webserver(void);
void stop_webserver(void);
void http_get_data_switch( char *data, uint16_t len);

void set_wifi_inf_callback( void *cb );
void set_switch_callback ( void *cd);
void set_rgb_callback( void *cb );
void set_wifi_reset_callback(void *cb);
void set_http_switch_value_callback( void *cb );
void set_time_data_post_callback( void *cb );
void http_set_flag_disable( void );


#endif /* __HTTP_SERVER_H */





