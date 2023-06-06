#pragma once 

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "time.h"
#include <sys/time.h>
#include <string.h>

#define TAGS_STORAGE_NAMESPACE "tags"
#define TIME_STORAGE_NAMESPACE "time"

ESP_EVENT_DECLARE_BASE(DB_EVENTS);

typedef struct db_data_entry {
    const char* time;
    const char* card_tag;
    uint8_t direction;
} db_data_entry_t;

typedef db_data_entry_t* db_data_t;

typedef struct {
    db_data_t array;
    uint16_t size;
} db_data_array_t;

struct db_event_handle {
    esp_event_loop_handle_t event_handle;  /*<! Handle of event loop */
};

typedef struct db_event_handle* db_event_handle_t;

typedef enum {
    DB_EVENT_ANY = ESP_EVENT_ANY_ID,
    DB_EVENT_NONE,
    DB_EVENT_TABLE_READ,             /*<! Table is read */
} db_event_t;

typedef struct {
    db_event_handle_t db;
    void* ptr;
} db_event_data_t;

typedef enum tag_direction {
    IN,
    OUT,
} tag_direction_t;

typedef struct time_blob {
    char tag[25];
    tag_direction_t direction;
} time_blob_t;

esp_err_t db_save_tag(uint64_t tag);
esp_err_t db_read_attendance();

esp_err_t db_register_events(db_event_handle_t db, db_event_t event, esp_event_handler_t event_handler, void* event_handler_arg);
esp_err_t db_unregister_events(db_event_handle_t db, db_event_t event, esp_event_handler_t event_handler);