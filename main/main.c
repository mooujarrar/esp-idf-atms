
#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"
#include "webapp.h"
#include "access-point.h"
#include "database.h"

static const char* TAG = "rfid-web";
static rc522_handle_t scanner;

i2c_dev_t ds3231;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                db_save_tag(tag->serial_number);
                db_read_attendance();
            }
            break;
    }
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    webapp_stop_webserver(server);   
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;

    if (*server == NULL) {
        const webapp_handle_t webapp = webapp_get_defaults();
        *server = webapp_start_webserver(webapp);
    }
}

void app_main()
{

    /*
        Initialize NVS
    */ 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /*
        Initialize WIFI SoftAP
    */ 
    static httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "START WiFi SOFT AP");
    wifi_init_softap();
    // In case of connecting to the SoftAP, we launch the webserver
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    // In case of disconnect, the server will shut down
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    /*
        Initialize ds3231 RTC
    */ 
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&ds3231, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(ds3231_init_desc(&ds3231, 0, CONFIG_RTC_I2C_MASTER_SDA, CONFIG_RTC_I2C_MASTER_SCL));

    /*
        Initialize MFRC522 RFID reader
    */
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = CONFIG_ESP_RFID_MISO,
        .spi.mosi_gpio = CONFIG_ESP_RFID_MOSI,
        .spi.sck_gpio = CONFIG_ESP_RFID_SCK,
        .spi.sda_gpio = CONFIG_ESP_RFID_SDA,
    };
    ESP_LOGI(TAG, "START RFID SCANNER");
    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
}