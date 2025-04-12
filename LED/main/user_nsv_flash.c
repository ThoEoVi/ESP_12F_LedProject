#include "user_nsv_flash.h"

// static const char *TAG = "---------> nsv_flash.c";

static nvs_handle_t my_handle_i8;
static nvs_handle_t my_handle_i32;
static nvs_handle_t my_handle_str;


void write_i32_to_nsv_flash( const char *key, int32_t value)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Open nsv */
    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_i32);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");

        // Write
        // printf("Updating  NVS ... ");
        err = nvs_set_i32(my_handle_i32, key, value);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle_i32);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle_i32);
    }
}

int32_t read_i32_from_nvs_flash ( const char *key )
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    int32_t buff;

    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_i32);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");

        // Read
        // printf("Reading from NVS ... ");
        err = nvs_get_i32(my_handle_i32, key, &buff);
        // switch (err) {
        //     case ESP_OK:
        //         // printf("Done\n");
        //         // printf("Value read = %d\n", buff);
        //         break;
        //     case ESP_ERR_NVS_NOT_FOUND:
        //         // printf("The value is not initialized yet!\n");
        //         break;
        //     default :
        //         // printf("Error (%s) reading!\n", esp_err_to_name(err));
        // }
    }
    return buff;
}

void write_i8_to_nsv_flash( const char *key, int8_t value)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Open nsv */
    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_i8);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");

        // Write
        // printf("Updating  NVS ... ");
        err = nvs_set_i8(my_handle_i8, key, value);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle_i8);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle_i8);
    }
}

int8_t read_i8_from_nvs_flash ( const char *key )
{
    /* return value */
    int8_t buff;

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_i8);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");

        // Read
        // printf("Reading from NVS ... ");
        err = nvs_get_i8(my_handle_i8, key, &buff);
        // switch (err) {
        //     case ESP_OK:
        //         // printf("Done\n");
        //         // printf("Value read = %d\n", buff);
        //         break;
        //     case ESP_ERR_NVS_NOT_FOUND:
        //         // printf("The value is not initialized yet!\n");
        //         break;
        //     default :
        //         // printf("Error (%s) reading!\n", esp_err_to_name(err));
        // }
    }
    return buff;
}

void write_str_to_nsv_flash(const char *key, char *data)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Open nsv */
    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_str);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");

        // Write
        // printf("Updating  NVS ... ");
        err = nvs_set_str(my_handle_str, key, data);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle_str);
        // printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle_str);
    }
}

void read_str_from_nvs_flash ( const char *key, char *data_out, size_t len )
{
    size_t xlen[1u];
    xlen[0] = len;
    // ESP_LOGI(TAG, "the len input: %d\n", xlen[0]);
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // ESP_LOGI(TAG,"open nvs handler\n");
    err = nvs_open(key, NVS_READWRITE, &my_handle_str);
    if (err != ESP_OK) {
        // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // printf("Done\n");
        // Read
        // printf("Reading from NVS ... ");
        err = nvs_get_str(my_handle_str, key, data_out, xlen);
        // switch (err) {
        //     case ESP_OK:
        //         // printf("Done\n");
        //         // printf("Value read = %s\n", data_out);
        //         break;
        //     case ESP_ERR_NVS_NOT_FOUND:
        //         // printf("The value is not initialized yet!\n");
        //         break;
        //     default :
        //         // printf("Error (%s) reading!\n", esp_err_to_name(err));
        // }
    }
}


void nvs_reset_all()
{
    nvs_erase_all(my_handle_i32);
    nvs_erase_all(my_handle_i8);
    nvs_erase_all(my_handle_str);
}




// int16_t read_i16_from_nvs_flash ( const char *key )
// {
//     // Initialize NVS
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // NVS partition was truncated and needs to be erased
//         // Retry nvs_flash_init
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( err );

//     int16_t buff;

//     ESP_LOGI(TAG,"open nvs handler\n");
//     err = nvs_open(key, NVS_READWRITE, &my_handle);
//     if (err != ESP_OK) {
//         printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
//     } else {
//         printf("Done\n");

//         // Read
//         printf("Reading from NVS ... ");
//         err = nvs_get_i16(my_handle, key, &buff);
//         switch (err) {
//             case ESP_OK:
//                 printf("Done\n");
//                 printf("Value read = %d\n", buff);
//                 break;
//             case ESP_ERR_NVS_NOT_FOUND:
//                 printf("The value is not initialized yet!\n");
//                 break;
//             default :
//                 printf("Error (%s) reading!\n", esp_err_to_name(err));
//         }
//     }
//     return buff;
// }
























