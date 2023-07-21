#include "webapp.h"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)
#define SCRATCH_BUFSIZE (10240)

static const char* TAG = "webapp";
static const char *REST_TAG = "esp-rest";
static const char *WS_TAG = "websocket";
static db_event_handle_t db;
httpd_handle_t server = NULL;


typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;


/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Our URI handler function to be called during GET /uri request */
static esp_err_t get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    ESP_LOGI(REST_TAG, "Request file %s%s", filepath, req->uri);
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        ESP_LOGI(REST_TAG, "Reading chunks of : %s", filepath);
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed: %s", filepath);
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete %s", filepath);
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Asynchronous response data structure
struct async_resp_arg
{
    httpd_handle_t hd; // Server instance
    int fd;            // Session socket file descriptor
};

struct async_resp_arg *resp_arg;

// The asynchronous response
static void generate_async_resp(const char* data)
{
    // Data format to be sent from the server as a response to the client
    httpd_handle_t hd = resp_arg->hd;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    static size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t fds = max_clients;
    int client_fds[max_clients];

    esp_err_t ret = httpd_get_client_list(server, &fds, client_fds);

    if (ret != ESP_OK) {
        return;
    }

    for (int i = 0; i < fds; i++) {
        int client_info = httpd_ws_get_fd_info(server, client_fds[i]);
        // Send data to the client
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(hd, client_fds[i], &ws_pkt);
        }
    }

}

// Initialize a queue for asynchronous communication
static esp_err_t ws_handler(httpd_req_t *req)
{
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    ESP_LOGI(WS_TAG, "ws_handler was called : %d", httpd_req_to_sockfd(req));

    return ESP_OK;
}


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

static void db_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    db_event_data_t* data = (db_event_data_t*) event_data;

    switch(event_id) {
        case DB_EVENT_TABLE_READ: {
                db_data_array_t* db_data = (db_data_array_t*) data->ptr;
                ESP_LOGI(TAG, "RECEIVED BY THE WEBAPP, IT SHOULD DISPLAY, Size of the entries is '%d'", db_data->size);
                const int db_data_length = db_data->size;
                cJSON *root = cJSON_CreateArray();
                for(int i = 0; i < db_data_length; i++) {
                    cJSON *object = cJSON_CreateObject();
                    // Prepare the json and send it via WS
                    cJSON_AddStringToObject(object, "tag", db_data->array[i].card_tag);
                    cJSON_AddStringToObject(object, "date", db_data->array[i].time);
                    cJSON_AddNumberToObject(object, "direction", db_data->array[i].direction);
                    cJSON_AddItemToArray(root, object);
                }
                const char *json_data = cJSON_Print(root);
                generate_async_resp(json_data);
                cJSON_Delete(root);
                free((void*)json_data);
            }
            break;
    }
}

/* ------------------ API ------------------------ */

// URI handler for send state of variables using webSockets.
static httpd_uri_t ws_get = {
    .uri       = "/ws",  
    .method    = HTTP_GET,
    .handler   = ws_handler,
    .user_ctx  = NULL,
    .is_websocket = true
};

/* URI handler structure for GET /uri */
static httpd_uri_t uri_get = {
    .uri      = "/*",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

webapp_handle_t webapp_get_defaults() {
    webapp_handle_t app = (webapp_handle_t) malloc(sizeof(webapp_t));
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    app->config = config;
    app->config.max_open_sockets = 10;
    app->config.uri_match_fn = httpd_uri_match_wildcard;
    app->uri_list_size = 2;
    httpd_uri_t* uri_list_ptr = (httpd_uri_t*) malloc(app->uri_list_size * sizeof(httpd_uri_t));
    char* default_mount_point = (char*) malloc(sizeof(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
    strcpy(default_mount_point, CONFIG_EXAMPLE_WEB_MOUNT_POINT);
    app->mount_point = default_mount_point;

    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    strlcpy(rest_context->base_path, app->mount_point, sizeof(rest_context->base_path));
    uri_get.user_ctx = rest_context;

    if(uri_list_ptr != NULL) {
        uri_list_ptr[0] = ws_get;
        uri_list_ptr[1] = uri_get;
        app->uri_list = uri_list_ptr;
    }
    return app;
}

static void register_to_db_events() {
    esp_event_loop_args_t event_args = {
        .queue_size = 1,
        .task_name = NULL, // no task will be created
    };
    db = (db_event_handle_t) malloc(sizeof(struct db_event_handle));
    if(ESP_OK != esp_event_loop_create(&event_args, &db->event_handle)) {
        ESP_LOGE(TAG, "Cannot create event loop for DB_EVENTS");
        return;
    }
    db_register_events(db, DB_EVENT_ANY, db_handler, NULL);
} 

/* Function for starting the webserver */
void webapp_start_webserver()
{
    ESP_LOGI(TAG, "Starting webserver");
    const webapp_handle_t app = webapp_get_defaults();
    // Initialize the handler for the ws
    resp_arg = malloc(sizeof(struct async_resp_arg));

    /* Start the httpd server */
    if (httpd_start(&server, &(app->config)) == ESP_OK) {
        /* Register URI handlers */
        httpd_uri_t* uri_list_ptr =  app->uri_list;
        for(uint8_t i = 0; i < app->uri_list_size; i++) {
            ESP_ERROR_CHECK(httpd_register_uri_handler(server, uri_list_ptr));
            uri_list_ptr++;
        }
        ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, http_server_handler, NULL));
        register_to_db_events();
    }
}

/* Function for stopping the webserver */
void webapp_stop_webserver()
{
    ESP_LOGI(TAG, "Stopping webserver");
    if (httpd_stop(server) == ESP_OK) {
        server = NULL;
        db_unregister_events(db, DB_EVENT_ANY, db_handler);
        free(db);
        free(resp_arg);
    } else {
        ESP_LOGE(TAG, "Failed to stop http server");
    }
}