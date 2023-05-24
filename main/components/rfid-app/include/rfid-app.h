#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"


typedef struct rfidapp {
    httpd_config_t* config;
    httpd_handle_t* server;
    uint8_t uri_list_size;
    httpd_uri_t* uri_list;
} rfidapp_t;



typedef rfidapp_t* rfid_webapp_handle_t;

/* Static methods, needed only internally */
static inline esp_err_t get_handler(httpd_req_t *req);
/* URI handler structure for GET /uri */
static const httpd_uri_t uri_get = {
    .uri      = "/uri",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

const rfid_webapp_handle_t rfid_app_get_defaults();
void rfid_app_start_webserver(rfid_webapp_handle_t app);
void rfid_app_stop_webserver(rfid_webapp_handle_t app);
void rfid_app_register_events(esp_event_handler_t* handler);
void rfid_app_unregister_events(esp_event_handler_t* handler);

#ifdef __cplusplus
}
#endif