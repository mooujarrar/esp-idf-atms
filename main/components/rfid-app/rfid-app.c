#include <stdio.h>
#include "rfid-app.h"

/* Our URI handler function to be called during GET /uri request */
static inline esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}



/* ------------------ API ------------------------ */

rfid_webapp_handle_t rfid_app_get_defaults() {
    rfidapp_t app = {};
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t* server = NULL;
    app.uri_list_size = 1;
    httpd_uri_t* uri_list_ptr = (httpd_uri_t*) malloc(app.uri_list_size * sizeof(httpd_uri_t));
    if(uri_list_ptr != NULL) {
        uri_list_ptr[0] = uri_get;
    }

    app.config = &config;
    app.server = server;
    app.uri_list = uri_list_ptr;

    rfid_webapp_handle_t handler = &app;
    return handler;
}


/* Function for starting the webserver */
void rfid_app_start_webserver(rfid_webapp_handle_t app)
{
    /* Start the httpd server */
    if (httpd_start(app->server, app->config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(*(app->server), app->uri_list);
    }
}

/* Function for stopping the webserver */
void rfid_app_stop_webserver(rfid_webapp_handle_t app)
{
    if (app->server) {
        /* Stop the httpd server */
        httpd_stop(app->server);
    }
}

void rfid_app_register_events(esp_event_handler_t* handler) {
    esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, handler, NULL);
}

void rfid_app_unregister_events(esp_event_handler_t* handler) {
    esp_event_handler_unregister(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, handler);
}