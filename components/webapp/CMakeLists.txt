idf_component_register(SRCS "webapp.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server mdns spiffs fatfs vfs sdmmc json database)
