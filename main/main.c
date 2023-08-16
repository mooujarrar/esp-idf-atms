/**
**********************************
* Program Description:
* This file is the entry point to the firmware.
* It set all tasks up and the needed configuration for them through the main loop.
* Filename : main.c
* Author : Mohyiddine Oujarrar (mooujarrar)
* Date : 16/08/2023
*************************************
*/

#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"
#include "webapp.h"
#include "access-point.h"
#include "database.h"
#define MDNS_INSTANCE "esp home web server"

static const char *TAG = "MY-ATMS-MAIN";
static rc522_handle_t scanner;

i2c_dev_t ds3231;

/**
 * @brief Event loop handler for the rc522 RFID reader,
 *        events such as a succesful reading of the PICC tag
 *        will be communicated through this handler.
 *
 * @param arg *Not important*
 * @param base *Not important*
 * @param event_id Event id that we are trying to subscribe to in the handler.
 *                 In our case we subscribe to RC522_EVENT_ANY
 *                 which means any event comming from the RC522 driver.
 * @param event_data The data published with that event
 */
static void rc522_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    rc522_event_data_t *data = (rc522_event_data_t *)event_data;

    switch (event_id)
    {
    case RC522_EVENT_TAG_SCANNED:
    {
        rc522_tag_t *tag = (rc522_tag_t *)data->ptr;
        ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
        db_save_tag(tag->serial_number);
        db_read_attendance();
    }
    break;
    }
}

/**
 * @brief That handler is subscribing to the event
 *        of connecting to the ESP32 access point.
 *        It checks if the HTTP server is not previously,
 *        instanciated by another client, if not
 *        we launch an HTTP server instance
 *
 * @param arg The event is triggered with the HTTP server instance as argument
 * @param base *Not important*
 * @param event_id The event ID we subscribe to, in that case WIFI_EVENT_AP_STACONNECTED
 * @param event_data *Not important*
 */
static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
             MAC2STR(event->mac), event->aid);
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        webapp_start_webserver();
    }
}

/**
 * @brief That handler is subscribing to the event
 *        of disconnecting from the ESP32 access point.
 *        It checks if the number of clients is 0,
 *        then shuts down the HTTP server since there is no point
 *        of leaving it running with no client to serve.
 *
 * @param arg The event is triggered with the HTTP server instance as argument
 * @param base *Not important*
 * @param event_id The event ID we subscribe to, in that case WIFI_EVENT_AP_STADISCONNECTED
 * @param event_data *Not important*
 */
static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
             MAC2STR(event->mac), event->aid);
    if (*server != NULL)
    {
        webapp_stop_webserver();
    }
}

/**
 * @brief Initialize the mdns service to give to the ESP32
 *        a domain name where the user can easily connect
 *        to the application through, example: http://my-atms.local
 *
 */
static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}};

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

/**
 * @brief Initializing the Virtual FS of the ESP32
 *        to store the WebApp of the Time and Attendance Management System.
 *
 * @return esp_err_t Status of the operation:
 *                    - ESP_OK if succeded.
 *                   - NOT(ESP_OK) otherwise.
 */
esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 20,
        .format_if_mount_failed = false};
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

/**
 * @brief Application entrypoint and main loop.
 *
 */
void app_main()
{

    /*
        Initialize NVS
    */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /*
        Initialize the FS: SSPI flash to host the webapp
    */
    ESP_ERROR_CHECK(init_fs());

    /*
        Initialize WIFI SoftAP
    */
    extern httpd_handle_t server;
    wifi_init_softap();
    // Subscribe to the connection event: In case of connecting to the SoftAP, we launch the webserver
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    // Subscribe to the disconnect event: In case all clients are disconnected, the HTTP server will be shut down
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    /*
        Initialize ds3231 RTC
    */
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&ds3231, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(ds3231_init_desc(&ds3231, 0, CONFIG_RTC_I2C_MASTER_SDA, CONFIG_RTC_I2C_MASTER_SCL));

    /*
        Initialize mdns to give to the ESP32 a Domain name, default: esp-home.local
    */
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

    /*
        Initialize MFRC522 RFID card reader driver
    */
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = CONFIG_ESP_RFID_MISO,
        .spi.mosi_gpio = CONFIG_ESP_RFID_MOSI,
        .spi.sck_gpio = CONFIG_ESP_RFID_SCK,
        .spi.sda_gpio = CONFIG_ESP_RFID_SDA,
    };
    rc522_create(&config, &scanner);
    rc522_start(scanner);
    // Subscribe to the events from the rc522
    ESP_ERROR_CHECK(rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL));
}