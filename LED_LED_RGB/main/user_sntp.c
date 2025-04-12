#include "user_sntp.h"

get_sntp_callback_t   get_sntp_callback = NULL;

// static const char *TAG = "sntp_example";

static time_t user_now;
static struct tm user_timeinfo;


static void initialize_sntp(void)
{
    // ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;   // time to reconnect to ntp

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        // ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}



void sntp_setup(void)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        // ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }

    // Set timezone to GMT-7
    setenv("TZ", "CST-7", 1);
    tzset();
}

void get_sntp_time( user_time_t *time_inf )
{
    // update 'now' variable with current time
    time(&user_now);
    localtime_r(&user_now, &user_timeinfo);

    if (user_timeinfo.tm_year < (2016 - 1900)) {
        time_inf->state = FUNC_ERR;
    } else {
        time_inf->hour = user_timeinfo.tm_hour;
        time_inf->min  = user_timeinfo.tm_min;
        time_inf->sec  = user_timeinfo.tm_sec;
        time_inf->state= FUNC_OK;
    }
    //ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());
}









