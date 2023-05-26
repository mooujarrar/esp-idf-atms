#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"



typedef struct webapp {
    httpd_config_t* config;
    httpd_handle_t* server;
    uint8_t uri_list_size;
    httpd_uri_t* uri_list;
} webapp_t;

typedef webapp_t* webapp_handle_t;

/* Static methods, needed only internally */
static inline esp_err_t get_handler(httpd_req_t *req);
/* URI handler structure for GET /uri */
static const httpd_uri_t uri_get = {
    .uri      = "/uri",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* API */
webapp_handle_t webapp_get_defaults();
void webapp_start_webserver(webapp_handle_t app);
void webapp_stop_webserver(webapp_handle_t app);
void webapp_register_events(esp_event_handler_t* handler);
void webapp_unregister_events(esp_event_handler_t* handler);

#ifdef __cplusplus
}
#endif