#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs.h"
#include "lwip/apps/sntp.h"

/* user include */
#include "connect.h"
#include "http_server.h"
#include "gpio.h"
#include "user_nsv_flash.h"
#include "user_sntp.h"
#include <string.h>
#include <stdint.h>


/* Define reset mode */
typedef enum{
    reset_cpu,
    reset_all
} reset_mode_t;


/* Prototype functions */
static void v_http_wifi_inf_callback( char *data, uint16_t len );       // wifi information to register new wifi
static void v_switch_callback( char *data, uint16_t len );              // set state led from html page
static void v_rgb_callback ( char *data, uint16_t len );                // get rgb value and using modul ledc
static void v_reset_wifi_callback( void );                              //
static void v_isr_button_callback( void );                              // handler isr button to set state led
static void v_button_update_http( void );                               // send state led to the html page    
static void v_reset_button( void );                                     // call function to reset variabes key_button in gpio.c
static void encoder_time(char *data, int32_t *value);                   // encoder time set to one uint16 value
static void decoder_time( uint32_t value_in, user_time_t *time_on_out,
                     user_time_t *time_off_out, uint8_t *time_update);  // decoder uint16 value to time 
static void v_time_data_post_callback( char *data, uint16_t len );      // get and handler time set from web
static uint16_t u16_rgb_to_ledc_value( uint8_t inp );                   // convert rgb value to percent value
static rgb_color_t x_seperate_color( char *rgb_color );                 // seperate str rgb to rgb value
static void wifi_state_callback( wifi_notify_state_t wifi_state);       // using for switch ap to sta
static void v_send_ip_callback( char *data, uint16_t len );             // send ip str from lib
static void seperate_n_end_str( char *data_in, char *data_out, char n); // get ip str
static void v_reset_all ( reset_mode_t mode );                          // reset all 
static void v_wifi_disconnect_callback( void );                         // blink state led when wifi disconnected

/* Task handler */
static void  v_task_handler_led( void *arg );                           // task handler led
static void v_task_handler_sntp( void *arg );                           // test handler sntp

/* Timer handler */
static void timer_callback_handler( TimerHandle_t xTimer );             // timer for blink led
static void timer_2_callback_handler ( TimerHandle_t xTimer );          // timer for choosing mode wifi by button and blink state led if wifi disconnected

/*==============*/
// static const char *TAG = "======---------> main.c";

volatile static uint8_t g_u8_flag = 8u;                                 // first init set bit 3
                                                                        // bit 0: state led: uing for turn on or off.
                                                                        // bit 1: temp state led: using for handler in each time the state led change.
                                                                        // bit 2: static state led to blink led state in timer 1 handler function.
                                                                        // bit 3: flag timer using in handler function timer 2 and isr button handler function.
                                                                        // bit 4,5,6: timer 2 counter when press button
                                                                        // bit 7: reset cpu   

static user_time_t data_time, time_on, time_off;                        // time update sntp, time set on , time set off
static char g_wifi_data[113u];                                          // display ip address
static uint8_t time_updt;                                               // time to update real time

uint8_t g_wifi_state_mode = wifi_non_connected;                         // state wifi when connect wifi
                                                                        // bit 0,1: wifi_notify_state_t
                                                                        // bit 2: wifi disconnected
                                                                        // bit 3: reset all

static SemaphoreHandle_t xMutex_key = NULL;                             // mutex handler
TimerHandle_t xTimer_1 = NULL;                                          // timer 1 used for blink state led
TimerHandle_t xTimer_2 = NULL;                                          // timer 2 used for choosing mode by hard button 


void app_main()
{
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Create semaphore mutex */
    do{
        xMutex_key = xSemaphoreCreateMutex();
    }
    while(xMutex_key == pdFAIL);
    
    /* Create Timer */
    xTimer_1 = xTimerCreate("Timer_1", pdMS_TO_TICKS(100u), pdTRUE, (void *const)0u, timer_callback_handler);       // after use, timer will delete
    xTimerStart(xTimer_1, portMAX_DELAY);
    xTimer_2 = xTimerCreate("Timer_2", pdMS_TO_TICKS(200u), pdTRUE, (void *const)1u, timer_2_callback_handler);     // after use, timer will delete

    /* Create task handler led */
    xTaskCreate(v_task_handler_led, "led_task", configMINIMAL_STACK_SIZE, NULL, 10u, NULL);

    /* Read time on and time off from nsv_flash */
    decoder_time(read_i32_from_nvs_flash("time_set"), &time_on, &time_off, &time_updt);
    // ESP_LOGI(TAG, "time on:  %d, %d, %d----time off: %d, %d, %d----time ud:  %d\n", 
    //                                 time_on.hour, time_on.min, time_on.enable,
    //                                 time_off.hour, time_off.min, time_off.enable, time_updt);

    /* save ssid, pass to flash */
    uint8_t wifi_data_len = read_i8_from_nvs_flash("wifi_dat_len") + 1u;
    // ESP_LOGI(TAG, "wifi data len: %d\n", wifi_data_len);
    char wifi_data[wifi_data_len];
    read_str_from_nvs_flash("wifi_dat", wifi_data, wifi_data_len);      
    // ESP_LOGI(TAG, "The wifi inf: %s\n", wifi_data);                          

    /* init time update is 1 when not set */
    if(time_updt == 0u)
        time_updt = 1u;

    /* Config gpio */
    config_gpio_led_status();
    config_led_pin();
    config_button();
    v_ledc_init();

    /* set call back function */
    set_wifi_inf_callback(v_http_wifi_inf_callback);
    set_switch_callback(v_switch_callback);
    set_rgb_callback(v_rgb_callback);
    set_wifi_reset_callback(v_reset_wifi_callback);
    set_input_isr_callback(v_isr_button_callback);
    set_http_switch_value_callback( v_button_update_http );
    set_time_data_post_callback( v_time_data_post_callback );
    set_wifi_state_callback(wifi_state_callback);
    set_send_ip_callback(v_send_ip_callback);
    set_wifi_even_state_callback(v_wifi_disconnect_callback);

    /* Start wifi mode: sta or ap according wifi_data_len */
    if( wifi_data_len > 20u ){
        /* reset str */
        char ssid[32u]; memset(ssid, '\0', 32u);
        char pass[64u]; memset(pass, '\0', 64u);
        char *token = NULL;

        /* seperate data to ssid and pass */
        token = strtok(wifi_data, "#@@#@");
        if(token){
            sprintf(ssid, "%s", token);
        }
        token = strtok(NULL, "#@@#@");
        if(token){
            sprintf(pass, "%s", token);
        }
        // ESP_LOGI(TAG, "in main:   ---   ssid: %s, pass: %s\n", ssid, pass);

        wifi_switch_station(ssid, pass, wifi_switch_reset);
        /* get time in sntp */
        xTaskCreate(v_task_handler_sntp, "sntp_task", 2048u, NULL, 8u, NULL);
    }else{
        wifi_init_softap();
    }
    
    /* start server */
    start_webserver();
}


/* sntp handler task */
static void v_task_handler_sntp( void *arg )
{
    /* setup sntp */
    sntp_setup();
    while(1){
        /* get time */
        get_sntp_time(&data_time);

        /* use semaphore */
        if( pdPASS == xSemaphoreTake(xMutex_key, portMAX_DELAY) ){
            /* Handler timer set on */
            if (time_on.enable == TIME_SET_ENA )
                if( data_time.hour == time_on.hour )
                    if( data_time.min == time_on.min)
                        g_u8_flag |= 1u; /* change state led */
            /* Handler timer set off */
            if (time_off.enable == TIME_SET_ENA )
                if( data_time.hour == time_off.hour )
                    if( data_time.min == time_off.min)
                        g_u8_flag &= 0xFE; /* change state led */
            xSemaphoreGive(xMutex_key);
        }
        vTaskDelay(pdMS_TO_TICKS( time_updt *1000u));  
    }
    vTaskDelete(NULL);
}

/* handler button to control led */
void v_task_handler_led( void *arg )
{
    while(1){
        /* using semaphore */
        if( pdPASS == xSemaphoreTake(xMutex_key, portMAX_DELAY) ){
            /* if value change then go to handler */
            if( (g_u8_flag&2u) != (g_u8_flag&1u) ){
                if( (g_u8_flag&1u) == 1u ){
                    g_u8_flag |= 2u; // set bit 1.
                    v_turn_on_led();
                }
                else if( (g_u8_flag&1u) == 0u ){
                    g_u8_flag &= ~(uint8_t)2u; // clear bit 1.
                    v_turn_off_led();
                }
                /* reset key button in lib gpio.c */
                v_reset_button();
            }
            xSemaphoreGive(xMutex_key);
        }
        /* reset all by hard button */
        if( (g_u8_flag>>7u)&1u ){
            v_reset_all( reset_all );
        }
        else if( (g_wifi_state_mode>>3u)&1u ){
            v_reset_all( reset_cpu );
        }

        if( (g_wifi_state_mode>>2u)&1u ){
            /* reset bit 2 */
            g_wifi_state_mode &= 0xFB;
            /* Create to blink state led */
            xTimer_1 = xTimerCreate("Timer_1", pdMS_TO_TICKS(100u), pdTRUE, (void *const)0u, timer_2_callback_handler);       // after use, timer will delete
            xTimerStart(xTimer_1, portMAX_DELAY);
        }
        /* Delay */   
        vTaskDelay(pdMS_TO_TICKS(100u));
    }
    vTaskDelete(NULL);
}

/* reset key button in the gpio.c */
static void v_reset_button( void )
{
    v_reset_key_button();
}

/* handler for isr button */
static void v_isr_button_callback( void )
{   
    BaseType_t  pxHigherPriorityTaskWoken = pdFAIL;
    if( pdPASS == xSemaphoreTakeFromISR(xMutex_key, &pxHigherPriorityTaskWoken)){
        /* toggle state led */
        g_u8_flag ^= 1u;
        
        /* handler timer mode */
        if( ((g_u8_flag>>3u)&1u) ){
            g_u8_flag &= ~(uint8_t)8u;
            /* reset bit 4-6 */
            g_u8_flag &= ~(uint8_t)(7u << 4u);
            xTimerStartFromISR(xTimer_2, &pxHigherPriorityTaskWoken);
        }
        else if( !((g_u8_flag>>3u)&1u) ){
            g_u8_flag |= 8u;
            xTimerStopFromISR(xTimer_2, &pxHigherPriorityTaskWoken);
            if(((g_u8_flag>>4u)&7u) <= 3u ){
                /* set bit 3 */
                g_wifi_state_mode |= (uint8_t)0x08;
            }
            else if( ((g_u8_flag>>4u)&7u) < 7u ){
                // set bit 7: reset cpu
                g_u8_flag |= 128u;  
            }
        }
        xSemaphoreGiveFromISR(xMutex_key, &pxHigherPriorityTaskWoken);
    }
    if( pdPASS == pxHigherPriorityTaskWoken)
        taskYIELD();
}

/* get the value from html page to set or clear state led bit */
static void v_switch_callback( char *data, uint16_t len )
{
    if( pdPASS == xSemaphoreTake(xMutex_key, portMAX_DELAY) ){
        if(*data == '1'){
            g_u8_flag |= 1u;
        }
        else{
            g_u8_flag &= ~(uint8_t)1u;
        }
        xSemaphoreGive(xMutex_key);
    }
}


/* Send data to html */
static void v_button_update_http( void )
{
    char resp_str[200u];
    char ip[16u];
    memset(ip, '\0', 16u);

    /* get ip value from g_wifi_data */
    seperate_n_end_str(g_wifi_data, ip, 15u);

    /* update to html page */
    sprintf(resp_str, "{\"switch\": \"%d\", \"hour\": \"%d\", \"min\": \"%d\", \"sec\": \"%d\", \"houro\": \"%d\", \"mino\": \"%d\", \"enao\": \"%d\", \"hourof\": \"%d\", \"minof\": \"%d\", \"enaof\": \"%d\", \"ip\": \"%s\"}",
                                    (g_u8_flag&1u), data_time.hour, data_time.min, data_time.sec, 
                                    time_on.hour, time_on.min, time_on.enable, time_off.hour, time_off.min, time_off.enable, ip);
    http_get_data_switch(resp_str, strlen(resp_str));
}

/* get n characters at the end of the string */
static void seperate_n_end_str( char *data_in, char *data_out, char n)
{
    char dest_index = 0u;
    uint16_t len = strlen(data_in);
    for( char i= len-n; i< len; i++){
        //data_out[dest_index] = data_in[i];
        *(data_out + dest_index) = *(data_in + i);
        dest_index++;
    }
}


/* handler time set  */
static void v_time_data_post_callback( char *data, uint16_t len )
{
    //ESP_LOGI(TAG, "the data: %s\n", data);
    int32_t value;
    encoder_time(data, &value );
    /* Update data: time on, time off */
    decoder_time(value, &time_on, &time_off, &time_updt);
    /* write to flash */
    write_i32_to_nsv_flash("time_set", value);
}

/* encode time data to int32 value */
static void encoder_time(char *data, int32_t *value)
{   
    /* reset value */
    *value = 0u;
    char *token = NULL;
    /* seperate data to ssid and pass */
    token = strtok(data, "/");
    /* get time on : hour, min, sec */
    if(token){
        *value <<= 5u;
        *value |= atoi(token);
    }
    token = strtok(NULL, "/");
    if(token){
        *value <<= 6u;
        *value |= atoi(token);
    }
    token = strtok(NULL, "/");
    if(token){
        *value <<= 1u;
        *value |= atoi(token);
    }
    /* get time off : hour, min, sec */
    token = strtok(NULL, "/");
    if(token){
        *value <<= 5u;
        *value |= atoi(token);
    }
    token = strtok(NULL, "/");
    if(token){
        *value <<= 6u;
        *value |= atoi(token);
    }
    token = strtok(NULL, "/");
    if(token){
        *value <<= 1u;
        *value |= atoi(token);
    }
    /* get time update */
    token = strtok(NULL, "/");
    if(token){
        *value <<= 8u;
        *value |= atoi(token);
    }
    // ESP_LOGI(TAG, "the value: %d, hour: %d, min: %d, set: %d ----- hour: %d, min: %d, set: %d --- time_u: %d\n",
    //                                         *value,  (*value>>27u&31u), (*value>>21u&63u), (*value>>20u&1),
    //                                         (*value>>15u&31u), (*value>>9u&63u), (*value>>8u&1), (*value&255));
}

/* decode uint32 value to the time out */
static void decoder_time( uint32_t value_in, user_time_t *time_on_out, user_time_t *time_off_out, uint8_t *time_update)
{
    /* In time on */
    time_on_out->hour    = value_in>>27u&31u;
    time_on_out->min     = value_in>>21u&63u;
    time_on_out->enable  = value_in>>20u&1;
    /* In time off */
    time_off_out->hour    = value_in>>15u&31u;
    time_off_out->min     = value_in>>9u&63u;
    time_off_out->enable  = value_in>>8u&1;
    /* In time update */
    *time_update = value_in&255;
}


/* uing for ledc modul */
static void v_rgb_callback ( char *data, uint16_t len )
{
    rgb_color_t color = {0};
    color = x_seperate_color(data);
    // ESP_LOGI(TAG, "The rgb --> %d, %d, %d\n", color.r, color.g, color.b);
    v_ledc_fade_set_duty(0, u16_rgb_to_ledc_value(color.r), 1000u);
    v_ledc_fade_set_duty(1, u16_rgb_to_ledc_value(color.g), 1000u);
    v_ledc_fade_set_duty(2, u16_rgb_to_ledc_value(color.b), 1000u);
}

/* seperate and conver string rgb to rgb value1 */
static rgb_color_t x_seperate_color( char *rgb_color )
{
    rgb_color_t color= {0};
    char buff[6u];
    sprintf(buff, "%s", rgb_color);
    
    /* convert str_hex to int */
    for(uint8_t i= 0u; i < 6u; i++){
        if( buff[i]>= 48 && buff[i] <= 57 ){
            if( i < 2){
                color.r <<= 4u;
                color.r |= ((uint8_t)buff[i] - 48);
            }
            else if ( i < 4){
                color.g <<= 4u;
                color.g |= ((uint8_t)buff[i] - 48);
            }
            else if ( i < 6){
                color.b <<= 4u;
                color.b |= ((uint8_t)buff[i] - 48);
            }
        }
        else if( buff[i]>= 65 && buff[i] <= 70 ){
            if( i < 2){
                color.r <<= 4u;
                color.r |= ((uint8_t)buff[i] - 65 + 10);
            }
            else if ( i < 4){
                color.g <<= 4u;
                color.g |= ((uint8_t)buff[i] - 65 + 10);
            }
            else if ( i < 6){
                color.b <<= 4u;
                color.b |= ((uint8_t)buff[i] - 65 + 10);
            }
        }
        else{
            color.state = FUNC_ERR;
            return color;
        }
    }
    color.state = FUNC_OK;
    return color;
}

/* convert 0->8000 to 0->255 */
static uint16_t u16_rgb_to_ledc_value( uint8_t inp )
{
    return ((uint16_t)(inp*8000u/255u));
}


/* get data from html page, store to "g_wifi_data" to using in the function "v_send_ip_callback", 
    handler data receive and switch ap_mode to get ip */
static void v_http_wifi_inf_callback( char *data, uint16_t len )
{
    /* get wifi ssid and pass to global value */
    memset(g_wifi_data, '\0', sizeof(g_wifi_data));
    strcat(g_wifi_data, data);
    
    /* handler data */
    char ssid[32u]; memset(ssid, 0u, strlen(ssid));
    char pass[64u]; memset(pass, 0u, strlen(pass));
    char *token = NULL;

    /* seperate data to ssid and pass */
    token = strtok(data, "#@@#@");
    if(token){
        sprintf(ssid, "%s", token);
    }
    token = strtok(NULL, "#@@#@");
    if(token){
        sprintf(pass, "%s", token);
    }
    // ESP_LOGI(TAG, "ssid: %s, pass: %s\n", ssid, pass);

    /* switch to station mode */
    wifi_switch_station(ssid, pass, wifi_switch_normal);
}

/* function callback when press reset button on html page */
static void v_reset_wifi_callback( void )
{
    /* reset cpu */
    v_reset_all(reset_all);
}

/* function to reset cpu or reset cpu and flash */
static void v_reset_all ( reset_mode_t mode )
{
    // ESP_LOGI(TAG, "Restarting now.\n");
    if( mode == reset_all ){
        // ESP_LOGI(TAG, "reset flash.\n");
        /* earse flash */
        nvs_reset_all();
    }
    /* reset cpu */
    fflush(stdout);
    esp_restart();
}

/* get wifi state after connect, then using for function handler timer 1 to blink state led */
static void wifi_state_callback( wifi_notify_state_t wifi_state)
{
    /* reset bit 0,1 */
    g_wifi_state_mode &= (uint8_t)0xFC; 
    /* set bit 0,1 */
    g_wifi_state_mode |= wifi_state;
}

/* function handler timer 1: to blink state led */
static void timer_callback_handler( TimerHandle_t xTimer )
{
    if( xTimer == xTimer_1 ){
        //static uint8_t _u8_state_led= 0u;
        if( (g_wifi_state_mode&0x03) == wifi_non_connected || (g_wifi_state_mode&0x03) == wifi_disconnected){
            g_u8_flag ^= 4u;
            gpio_set_level(LED_STATUS, g_u8_flag&4u);
        }
        else if((g_wifi_state_mode&0x03) == wifi_connected){
            gpio_set_level(LED_STATUS, 1u);
            http_set_flag_disable();
            xTimerDelete(xTimer_1, portMAX_DELAY);
        }
    }
}

/* Get ip to "g_wifi_data", save wifi inf to flash, switch to ap_mode */
static void v_send_ip_callback( char *data, uint16_t len )
{
    /* append ip data to g_wifi_data */
    strcat(g_wifi_data, data);
    // ESP_LOGI(TAG, "The wifi data in send ip call back: %s, string lend: %d\n", g_wifi_data, strlen(data));

    if(strlen(g_wifi_data) > 20u ){
        /* save ssid, pass to flash */
        write_i8_to_nsv_flash("wifi_dat_len", strlen(g_wifi_data));
        write_str_to_nsv_flash("wifi_dat", g_wifi_data);

        /* DELAY */
        vTaskDelay(pdMS_TO_TICKS(1000u));

        /* switch to ap mode */
        wifi_switch_ap();
    }
}

/* timer function handler to reset cpu or reset all */
static void timer_2_callback_handler ( TimerHandle_t xTimer )
{   
    if(xTimer == xTimer_2){
        /* Increate count */
        g_u8_flag += (1u << 4u);
        // ESP_LOGI(TAG, "timer 2: %d\n", (g_u8_flag>>4u)&7u);
        /* stop timer 2 */
        if( ((g_u8_flag>>4u)&7u) >= 7u){
            xTimerStop(xTimer_2, portMAX_DELAY);
            g_u8_flag |= 8u;        
        }
        /* Set led state */
        if(((g_u8_flag>>4u)&7u) <= 3u )
            /* turn on state led */
            gpio_set_level(LED_STATUS, 0u); 
        else
            /* turn off state led */
            gpio_set_level(LED_STATUS, 1u);
    }

    if( xTimer == xTimer_1){
        /* Blink state led */
        g_u8_flag ^= 4u;
        gpio_set_level(LED_STATUS, g_u8_flag&4u);
    }
}


/* WIFI disconnect status */
static void v_wifi_disconnect_callback( void )
{
    /* set bit 2 */
    g_wifi_state_mode |= 4u;
}














