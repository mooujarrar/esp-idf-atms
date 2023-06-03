#pragma once 

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "time.h"
#include <sys/time.h>
#include <string.h>

#define TAGS_STORAGE_NAMESPACE "tags"
#define TIME_STORAGE_NAMESPACE "time"

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