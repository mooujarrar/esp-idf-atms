#include "webapp.h"

static const char *TAG = "webapp";

/* Our URI handler function to be called during GET /uri request */
static inline esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    ESP_LOGI(TAG, "Get request received from the webapp, response now will be delivered");
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/* ------------------ API ------------------------ */


static void http_server_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    esp_http_server_event_data* data = (esp_http_server_event_data*) event_data;

    switch(event_id) {
        case HTTP_SERVER_EVENT_ON_CONNECTED: {
            ESP_LOGI(TAG, "Connection to the server");
        }
        break;
    }
}


webapp_handle_t webapp_get_defaults() {
    webapp_t app = {};
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

    webapp_handle_t handler = &app;
    return handler;
}


/* Function for starting the webserver */
void webapp_start_webserver(webapp_handle_t app)
{
    /* Start the httpd server */
    if (httpd_start(app->server, app->config) == ESP_OK) {
        /* Register URI handlers */
        httpd_uri_t* iterator =  app->uri_list;
        for(uint8_t i = 0; i < app->uri_list_size; i++) {
            ESP_ERROR_CHECK(httpd_register_uri_handler(*(app->server), iterator));
            iterator++;
        }
        ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, http_server_handler, NULL));
    }
}

/* Function for stopping the webserver */
void webapp_stop_webserver(webapp_handle_t app)
{
    if (app->server) {
        /* Stop the httpd server */
        ESP_ERROR_CHECK(httpd_stop(app->server));
    }
}

void webapp_register_events(esp_event_handler_t* handler) {
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, handler, NULL));
}

void webapp_unregister_events(esp_event_handler_t* handler) {
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, handler));
}