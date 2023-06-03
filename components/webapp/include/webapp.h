#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <inttypes.h>



typedef struct webapp {
    httpd_config_t config;
    httpd_handle_t server;
    uint8_t uri_list_size;
    httpd_uri_t* uri_list;
} webapp_t;

typedef webapp_t* webapp_handle_t;


/* API */
webapp_handle_t webapp_get_defaults();
httpd_handle_t* webapp_start_webserver(webapp_handle_t app);
void webapp_stop_webserver(httpd_handle_t* server);

#ifdef __cplusplus
}
#endif