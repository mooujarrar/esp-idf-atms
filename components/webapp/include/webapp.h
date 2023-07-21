#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "database.h"
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"

typedef struct webapp {
    httpd_config_t config;
    const char* mount_point;
    uint8_t uri_list_size;
    httpd_uri_t* uri_list;
} webapp_t;

typedef webapp_t* webapp_handle_t;


/* API */
webapp_handle_t webapp_get_defaults();
void webapp_start_webserver();
void webapp_stop_webserver();

#ifdef __cplusplus
}
#endif