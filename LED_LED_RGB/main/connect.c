#include "connect.h"

/* Trace message */
static const char *TAG = "======================> connect.c";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* Callback function */
wifi_notify_callback_t wifi_static_callback = NULL;
send_data_callback_t   send_data_callback = NULL;
wifi_connect_callback_t wifi_connnect_state_callback = NULL;



static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    static int s_retry_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10u) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "event_handler:  retry to connect to the AP");
        } else {
            wifi_connnect_state_callback();
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"event_handler:  connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "event_handler:  got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
        /* Send ip */
        send_data_callback(ip4addr_ntoa(&event->ip_info.ip), strlen(ip4addr_ntoa(&event->ip_info.ip)));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}



void wifi_switch_ap( void )
{
    //tcpip_adapter_init();
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler));


    ESP_ERROR_CHECK(esp_wifi_stop());


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL));
    

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
}

void wifi_init_softap( void ){
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL));
    

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .pmf_cfg = {
                .capable= true,
                .required= false
            },
        },
    };

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    // ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        // ESP_LOGI(TAG, "connected to ap SSID:%s\n", EXAMPLE_ESP_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        // ESP_LOGI(TAG, "Failed to connect to SSID:%s\n", EXAMPLE_ESP_WIFI_SSID);
    } else {
        // ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}

void set_wifi_state_callback( void *cb )
{
    wifi_static_callback = cb;
}

void set_send_ip_callback ( void *cb )
{
    send_data_callback = cb;
}

void wifi_switch_station( char *ssid, char *pass, wifi_switch_mode_t wifi_mode )
{
    if( wifi_mode == wifi_switch_normal ){
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler));
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
    
    s_wifi_event_group = xEventGroupCreate();

    if( wifi_mode == wifi_switch_reset ){
        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_create_default());
    }


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .pmf_cfg = {
                .capable= true,
                .required= false
            },
        },
    };
    /* Insert ssid and password */
    sprintf((char *)wifi_config.sta.ssid, "%s", ssid);
    sprintf((char *)wifi_config.sta.password, "%s", pass);
    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    // ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        // ESP_LOGI(TAG, "connected to ap SSID: %s\n", (char *)wifi_config.sta.ssid);
        wifi_static_callback(wifi_connected);
    } else if (bits & WIFI_FAIL_BIT) {
        // ESP_LOGI(TAG, "Failed to connect to SSID: %s\n", (char *)wifi_config.sta.ssid);
        wifi_switch_ap();
        wifi_static_callback(wifi_disconnected);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}

void set_wifi_even_state_callback ( void *cb )
{
    wifi_connnect_state_callback = cb;
}


// void wifi_switch_station_mormal( char *ssid, char *pass)
// {
//     ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler));
//     ESP_ERROR_CHECK(esp_wifi_stop());
    
//     s_wifi_event_group = xEventGroupCreate();

//     //tcpip_adapter_init();

//     //ESP_ERROR_CHECK(esp_event_loop_create_default());

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));

//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = "",
//             .password = "",
//             .pmf_cfg = {
//                 .capable= true,
//                 .required= false
//             },
//         },
//     };
//     /* Insert ssid and password */
//     sprintf((char *)wifi_config.sta.ssid, "%s", ssid);
//     sprintf((char *)wifi_config.sta.password, "%s", pass);
//     /* Setting a password implies station will connect to all security modes including WEP/WPA.
//         * However these modes are deprecated and not advisable to be used. Incase your Access point
//         * doesn't support WPA2, these mode can be enabled by commenting below line */

//     if (strlen((char *)wifi_config.sta.password)) {
//         wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
//     }

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
//     ESP_ERROR_CHECK(esp_wifi_start() );

//     ESP_LOGI(TAG, "wifi_init_sta finished.");

//     /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
//      * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
//     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//             pdFALSE,
//             pdFALSE,
//             portMAX_DELAY);

//     /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
//      * happened. */
//     if (bits & WIFI_CONNECTED_BIT) {
//         ESP_LOGI(TAG, "connected to ap SSID: %s\n", (char *)wifi_config.sta.ssid);
//         wifi_static_callback(wifi_connected);
//     } else if (bits & WIFI_FAIL_BIT) {
//         ESP_LOGI(TAG, "Failed to connect to SSID: %s\n", (char *)wifi_config.sta.ssid);
//         wifi_switch_ap();
//         wifi_static_callback(wifi_disconnected);
//     } else {
//         ESP_LOGE(TAG, "UNEXPECTED EVENT");
//     }

// }

// void wifi_switch_station_reset( char *ssid, char *pass )
// {
//     s_wifi_event_group = xEventGroupCreate();

//     tcpip_adapter_init();

//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = "",
//             .password = "",
//             .pmf_cfg = {
//                 .capable= true,
//                 .required= false
//             },
//         },
//     };
//     /* Insert ssid and password */
//     sprintf((char *)wifi_config.sta.ssid, "%s", ssid);
//     sprintf((char *)wifi_config.sta.password, "%s", pass);

//     /* Setting a password implies station will connect to all security modes including WEP/WPA.
//         * However these modes are deprecated and not advisable to be used. Incase your Access point
//         * doesn't support WPA2, these mode can be enabled by commenting below line */

//     if (strlen((char *)wifi_config.sta.password)) {
//         wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
//     }

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
//     ESP_ERROR_CHECK(esp_wifi_start() );

//     ESP_LOGI(TAG, "wifi_init_sta finished.");

//     /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
//      * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
//     EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//             WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//             pdFALSE,
//             pdFALSE,
//             portMAX_DELAY);

//     /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
//      * happened. */
//     if (bits & WIFI_CONNECTED_BIT) {
//         ESP_LOGI(TAG, "connected to ap SSID: %s\n", (char *)wifi_config.sta.ssid);
//         wifi_static_callback(wifi_connected);
//     } else if (bits & WIFI_FAIL_BIT) {
//         ESP_LOGI(TAG, "Failed to connect to SSID: %s\n", (char *)wifi_config.sta.ssid);
//         wifi_switch_ap();
//         wifi_static_callback(wifi_disconnected);
//     } else {
//         ESP_LOGE(TAG, "UNEXPECTED EVENT");
//     }

//     // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
//     // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
//     // vEventGroupDelete(s_wifi_event_group);
// }

















