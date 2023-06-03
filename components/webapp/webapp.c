#include "webapp.h"

static const char* TAG = "webapp";

static const char* webpage = "<!DOCTYPE html><html><head><style>body{width:100%;height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;margin:auto;overflow:auto;background:linear-gradient(315deg,rgba(101,0,94,1) 3%,rgba(60,132,206,1) 38%,rgba(48,238,226,1) 68%,rgba(255,25,25,1) 98%);animation:gradient 15s ease infinite;background-size:400% 400%;background-attachment:fixed;font-family:lato,sans-serif;color:#fff;padding:20px;box-sizing:border-box}table,td,th{border:1px solid #000;border-collapse:collapse}td,th{padding:5px}tr{background-color:#2f4f4f}@keyframes gradient{0%{background-position:0 0}50%{background-position:100% 100%}100%{background-position:0 0}}.wave{background:rgb(255 255 255 / 25%);border-radius:1000% 1000% 0 0;position:fixed;width:200%;height:12em;animation:wave 10s -3s linear infinite;transform:translate3d(0,0,0);opacity:.8;bottom:0;left:0;z-index:-1}.wave:nth-of-type(2){bottom:-1.25em;animation:wave 18s linear reverse infinite;opacity:.8}.wave:nth-of-type(3){bottom:-2.5em;animation:wave 20s -1s reverse infinite;opacity:.9}@keyframes wave{2%{transform:translateX(1)}25%{transform:translateX(-25%)}50%{transform:translateX(-50%)}75%{transform:translateX(-25%)}100%{transform:translateX(1)}}</style></head><body><div><div class=\"wave\"></div><div class=\"wave\"></div><div class=\"wave\"></div></div><h1>Time card</h1><table style=\"width:100%\"><tr><th>PICC Tag</th><th>Date</th><th>Time</th></tr></table></body></html>";

/* Our URI handler function to be called during GET /uri request */
static inline esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    ESP_LOGI(TAG, "Get request received from the webapp, response now will be delivered");
    const char* resp = webpage;
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /uri */
static httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* ------------------ API ------------------------ */


static void http_server_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    //esp_http_server_event_data* data = (esp_http_server_event_data*) event_data;

    switch(event_id) {
        case HTTP_SERVER_EVENT_ON_CONNECTED: {
            ESP_LOGI(TAG, "Connection to the server");
        }
        break;
    }
}


webapp_handle_t webapp_get_defaults() {
    webapp_handle_t app = (webapp_handle_t) malloc(sizeof(webapp_t));
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    app->config = config;
    app->config.max_open_sockets = 2;
    app->server = NULL;
    app->uri_list_size = 1;
    httpd_uri_t* uri_list_ptr = (httpd_uri_t*) malloc(app->uri_list_size * sizeof(httpd_uri_t));
    if(uri_list_ptr != NULL) {
        app->uri_list = &uri_get;
    }
    return app;
}


/* Function for starting the webserver */
httpd_handle_t* webapp_start_webserver(webapp_handle_t app)
{
    ESP_LOGI(TAG, "Starting webserver");
    /* Start the httpd server */
    if (httpd_start(&(app->server), &(app->config)) == ESP_OK) {
        /* Register URI handlers */
        httpd_uri_t* uri_list_ptr =  app->uri_list;
        for(uint8_t i = 0; i < app->uri_list_size; i++) {
            ESP_ERROR_CHECK(httpd_register_uri_handler(app->server, uri_list_ptr));
            uri_list_ptr++;
        }
        ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, http_server_handler, NULL));
        return &(app->server);
    }
    return NULL;
}

/* Function for stopping the webserver */
void webapp_stop_webserver(httpd_handle_t* server)
{
    if (*server != NULL) {
        ESP_LOGI(TAG, "Stopping webserver");
        if (httpd_stop(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}