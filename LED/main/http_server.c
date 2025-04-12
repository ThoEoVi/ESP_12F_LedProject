#include "http_server.h"

// static const char *TAG = "---------> HTTP app server.c";
static httpd_handle_t server = NULL;
static char flag_wifi_inf_enable = 1u; 

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");


httpd_req_t *req_sw, *req_time; 

/* Declare callback function */
get_data_callback_t    get_data_sw_callback             = NULL;
post_data_callback_t   post_data_callback               = NULL;
post_data_callback_t   post_switch_data_callback        = NULL;
post_callback_t        reset_wifi_callback              = NULL;
post_data_callback_t   time_data_callback               = NULL;
post_data_callback_t   rgb_data_callback                = NULL;

/*======================== static function ======================*/
/* Get Home page */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    /* send http page */
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}
static httpd_uri_t get_home_page = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL
};

/* Interval button value  */
static esp_err_t get_sw_handler(httpd_req_t *req)
{   
    // sprintf(resp_str, "{\"temperature\": \"%d\", \"humidity\": \"%d\"}", u8_test_value, u8_test_value);
    // httpd_resp_send(req, (const char*)resp_str, strlen(resp_str));
    req_sw = req;
    /* using callback func */
    get_data_sw_callback();
    return ESP_OK;
}
static httpd_uri_t get_data_sw_interval = {
    .uri       = "/switch_value",
    .method    = HTTP_GET,
    .handler   = get_sw_handler,
    .user_ctx  = NULL
};

/* POST wifi inf data */
static esp_err_t wifi_data_post_handler(httpd_req_t *req){
    if( flag_wifi_inf_enable == 1){
        char buf[111u];
        memset(buf, 0u, 111u);
        httpd_req_recv(req, buf, req->content_len);
        post_data_callback(buf, strlen(buf));
        // End response
        httpd_resp_send_chunk(req, NULL, 0);
    }
    return ESP_OK;
}
static httpd_uri_t wifi_inf_post_data = {
    .uri       = "/verify_wifi",
    .method    = HTTP_POST,
    .handler   = wifi_data_post_handler,
    .user_ctx  = NULL
};

/* POST switch button data */
static esp_err_t switch_post_handler(httpd_req_t *req){
    char buf[20u];
    memset(buf, 0u, 20u);
    httpd_req_recv(req, buf, req->content_len);
    post_switch_data_callback(buf, strlen(buf));
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
static httpd_uri_t switch_post_data = {
    .uri       = "/switch_button",
    .method    = HTTP_POST,
    .handler   = switch_post_handler,
    .user_ctx  = NULL
};



/* POST reset data */
static esp_err_t reset_wifi_post_handler(httpd_req_t *req){
    reset_wifi_callback();
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
static httpd_uri_t reset_wifi_post_data = {
    .uri       = "/reset_wifi",
    .method    = HTTP_POST,
    .handler   = reset_wifi_post_handler,
    .user_ctx  = NULL
};

/* POST time to control led data */
static esp_err_t time_post_handler(httpd_req_t *req){
    char buf[23u];
    memset(buf, 0u, 23u);
    httpd_req_recv(req, buf, req->content_len);
    time_data_callback(buf, strlen(buf));
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
static httpd_uri_t time_post_data = {
    .uri       = "/set_time",
    .method    = HTTP_POST,
    .handler   = time_post_handler,
    .user_ctx  = NULL
};


/* POST rgb color data */
static esp_err_t rgb_post_handler(httpd_req_t *req){
    char buf[8u];
    memset(buf, 0u, 8u);
    httpd_req_recv(req, buf, req->content_len);
    rgb_data_callback(buf, strlen(buf));
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
static httpd_uri_t rgb_post_data = {
    .uri       = "/rgb",
    .method    = HTTP_POST,
    .handler   = rgb_post_handler,
    .user_ctx  = NULL
};
/*======================== user function ======================*/
/* Init function */
void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    // ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        // ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_home_page);
        httpd_register_uri_handler(server, &wifi_inf_post_data);
        httpd_register_uri_handler(server, &switch_post_data);
        httpd_register_uri_handler(server, &rgb_post_data);
        httpd_register_uri_handler(server, &reset_wifi_post_data);
        httpd_register_uri_handler(server, &get_data_sw_interval);
        httpd_register_uri_handler(server, &time_post_data);
    }
    else{
        // ESP_LOGI(TAG, "Error starting server!");
    }
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}

void http_get_data_switch( char *data, uint16_t len)
{
    httpd_resp_send(req_sw, data, len);
}

/*======================== callback function ======================*/

void set_wifi_inf_callback( void *cb )
{
    post_data_callback = cb;
}

void set_switch_callback ( void *cb)
{
    post_switch_data_callback = cb;
}

void set_wifi_reset_callback(void *cb)
{
    reset_wifi_callback = cb;
}

void set_http_switch_value_callback( void *cb )
{
    get_data_sw_callback = cb;
}

void set_time_data_post_callback( void *cb )
{
    time_data_callback = cb;
}

void http_set_flag_disable( void )
{
    flag_wifi_inf_enable = 0u;
}

void set_rgb_callback( void *cb )
{
    rgb_data_callback = cb;
}















