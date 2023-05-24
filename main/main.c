
#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"
#include "rfid-app.h"

static const char* TAG = "rfid-web";
static rc522_handle_t scanner;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
            }
            break;
    }
}

static void http_server_handler(void* arg, esp_event_base_t base, esp_http_server_event_id_t event_id, void* event_data)
{
    esp_http_server_event_data* data = (esp_http_server_event_data*) event_data;

    switch(event_id) {
        case HTTP_SERVER_EVENT_ON_CONNECTED: {
            ESP_LOGI(TAG, "Connection to the server";
        }
        break;
    }
}

void app_main()
{
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 19,
        .spi.mosi_gpio = 23,
        .spi.sck_gpio = 18,
        .spi.sda_gpio = 5,
    };

    const rfid_webapp_handle_t webapp = rfid_app_get_defaults();
    rfid_app_register_events(&http_server_handler);
    rfid_app_start_webserver(webapp);

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
}