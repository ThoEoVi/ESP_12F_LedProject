#ifndef __USER_NSV_FLASH_H

#define __USER_NSV_FLASH_H


#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "string.h"

#define TIME_SET_ENA ( 1 )
#define TIME_SET_DIS ( 0 )

void write_i32_to_nsv_flash( const char *key, int32_t value);
int32_t read_i32_from_nvs_flash ( const char *key );
int16_t read_i16_from_nvs_flash ( const char *key );
void write_str_to_nsv_flash(const char *key, char *data);
void read_str_from_nvs_flash ( const char *key, char *data_out, size_t len );
void write_i8_to_nsv_flash( const char *key, int8_t value);
int8_t read_i8_from_nvs_flash ( const char *key );
void nvs_reset_all();

#endif




