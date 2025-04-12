#ifndef __USER_SNTP_H

#define __USER_SNTP_H


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "lwip/apps/sntp.h"

typedef enum {
    FUNC_OK,
    FUNC_ERR,
} state_func_t;

typedef enum {
    enable_time,
    disable_time,
} time_enable_t;

typedef struct rgb {
    uint8_t r, g, b;
    state_func_t state;
} rgb_color_t;

typedef struct time {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    state_func_t state;
    time_enable_t enable;
} user_time_t;

typedef void (*get_sntp_callback_t) (char *data, uint16_t len);


void sntp_setup(void);
void get_sntp_time( user_time_t *time_inf );



#endif



















