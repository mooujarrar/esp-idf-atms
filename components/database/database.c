/**
**********************************
* Program Description:
* @brief Manage the users Time and Attendance entries inside a Database
* Persistency is insured using the Flash memory of the ESP32
* NVS (Non Volatile Storage) is used for CRUD actions on the entries.
* @file database.c
* @author Mohyiddine Oujarrar (mooujarrar) (mohyiddineoujarrar@gmail.com) 
* @date 16/08/2023
*************************************
*/

#include <stdio.h>
#include "database.h"

ESP_EVENT_DEFINE_BASE(DB_EVENTS);

static db_event_handle_t db_handler;

/**
 * @brief Read and return the user time entry from the database.
 *        It takes a timestamp as a key, and gets the stored
 *        information: Card_Tag, Direction.
 * 
 * @param ptr Pointer on the database handle.
 * @param key The saved timestamp when the Card was read.
 * @param db_data The value stored in the map <Card_Tag, Direction>.
 * @return esp_err_t The Status of the operation.
 */
static esp_err_t read_time_entry(nvs_handle_t* ptr, const char* key, db_data_array_t* db_data) {
    esp_err_t err;
    time_blob_t* valuePtr = (time_blob_t*) malloc(sizeof(time_blob_t));
    size_t length = sizeof(time_blob_t);
    err = nvs_get_blob(*ptr, key, valuePtr, &length);
    if (err != ESP_OK) return err;
    db_data->size++;
    db_data->array = (db_data_t) realloc(db_data->array, db_data->size * sizeof(db_data_entry_t));
    char* tag = (char*) malloc(sizeof(valuePtr->tag));
    strcpy(tag, valuePtr->tag); 
    char* time = (char*) malloc(sizeof(key));
    strcpy(time, key); 
    db_data_entry_t new_data_entry = {
        .time = time,
        .card_tag = tag,
        .direction = valuePtr->direction
    };
    db_data->array[(db_data->size) - 1] = new_data_entry;
    free(valuePtr);
    return ESP_OK;
}

/**
 * @brief Saves the <Tag, Time, Direction> in the **time_table**.
 *        To get the current timestamp the RTC module ds3231 is used.
 * 
 * @param time_entry_ptr The <Tab, Direction> already set in db_save_tag().
 * @return esp_err_t Status of the operation.
 */
static esp_err_t db_save_time_entry(time_blob_t* time_entry_ptr)
{
    nvs_handle_t time_table_handle;
    esp_err_t err;

    char buf[40];
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    err = ds3231_get_time(&ds3231, &timeinfo);
    if (err != ESP_OK) return err;
    snprintf(buf, sizeof buf - 1, "%" PRIu64, (u_int64_t)mktime(&timeinfo));

    char* timePtr = (char*) malloc(sizeof(buf));
    strcpy(timePtr, buf);

    // Open the time table
    err = nvs_open(TIME_STORAGE_NAMESPACE, NVS_READWRITE, &time_table_handle);
    if (err != ESP_OK) return err;
    // Save the new entry
    err = nvs_set_blob(time_table_handle, timePtr, time_entry_ptr, sizeof(time_blob_t));
    if (err != ESP_OK) return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(time_table_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(time_table_handle);
    return ESP_OK;
}

/**
 * @brief - Saves a newly read tag in the **tags_table** if 
 *          it wasnt existing and sets the Direction to IN.
 *        - Erases the tag if was already present, 
 *          then sets the Direction to Out.
 *        - Triggers db_save_time_entry() function to persist 
 *          the Direction and Timestamp with the read tag inside
 *          the **time_table**.
 * 
 * @param tag Serial number of the PICC.
 * @return esp_err_t Returns the status of the operation.
 */
esp_err_t db_save_tag(uint64_t tag)
{
    nvs_handle_t tags_table_handle;
    esp_err_t err;

    // Open tags_table
    err = nvs_open(TAGS_STORAGE_NAMESPACE, NVS_READWRITE, &tags_table_handle);
    if (err != ESP_OK) return err;

    char picc_tag[25];
    snprintf(picc_tag, sizeof picc_tag - 1, "%" PRIu64, tag);

    // Read
    int8_t val = 0;
    err = nvs_get_i8(tags_table_handle, picc_tag, &val);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    // tag found we should clear it => the employee is quitting
    else if (err == ESP_OK) {
        err = nvs_erase_key(tags_table_handle, picc_tag);
        if (err != ESP_OK) return err;
        else {
            time_blob_t* time_entry = (time_blob_t*) malloc (sizeof(time_blob_t));
            time_entry->direction = OUT;
            memcpy(time_entry->tag, picc_tag, 25);
            db_save_time_entry(time_entry);
            free(time_entry);
        }
    }
    // tag not found => the employee is entering
    else if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_set_i8(tags_table_handle, picc_tag, 0);
        if (err != ESP_OK) return err;
        else {
            time_blob_t* time_entry = (time_blob_t*) malloc (sizeof(time_blob_t));
            time_entry->direction = IN;
            memcpy(time_entry->tag, picc_tag, 25);
            db_save_time_entry(time_entry);
            free(time_entry);
        }
    } 

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(tags_table_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(tags_table_handle);
    return ESP_OK;
}

/**
 * @brief Clears both time and tags tables
 *        from the Flash of the ESP.
 * 
 * @return esp_err_t Status of the operation.
 */
esp_err_t db_clear() {
    nvs_handle_t tags_table_handle;
    nvs_handle_t time_table_handle;

    esp_err_t err;

    // Open
    err = nvs_open(TAGS_STORAGE_NAMESPACE, NVS_READWRITE, &tags_table_handle);
    if (err != ESP_OK) return err;

    err = nvs_open(TIME_STORAGE_NAMESPACE, NVS_READWRITE, &time_table_handle);
    if (err != ESP_OK) return err;

    err = nvs_erase_all(tags_table_handle);
    if (err != ESP_OK) return err;
    
    return nvs_erase_all(time_table_handle);
}

/**
 * @brief Pushes the Database events through Event Loop Library.
 *        Subscribers registered to that event will receive its data
 *        after that dispatch.
 * 
 * @param db Handler of the database.
 * @param event Event to push.
 * @param data Data to push with the event.
 * @return esp_err_t Status of the operation.
 */
static esp_err_t db_dispatch_event(db_event_handle_t db, db_event_t event, void* data)
{
    if(!db) {
        return ESP_ERR_INVALID_ARG;
    }

    db_event_data_t e_data = {
        .db = db,
        .ptr = data,
    };
    esp_err_t err;
    if(ESP_OK != (err = esp_event_post_to(db->event_handle, DB_EVENTS, event, &e_data, sizeof(db_event_data_t), portMAX_DELAY))) {
        return err;
    }

    return esp_event_loop_run(db->event_handle, 0);
}

/**
 * @brief Reads the whole database and sends it 
 *        to all subscribers through the event loop.
 * 
 * @return esp_err_t Status of the operation.
 */
esp_err_t db_read_attendance() {
    nvs_handle_t time_table_handle;
    esp_err_t err;

    // Open the time table
    err = nvs_open(TIME_STORAGE_NAMESPACE, NVS_READWRITE, &time_table_handle);
    if (err != ESP_OK) return err;

    // Listing all the key-value pairs of the time table namespace
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, TIME_STORAGE_NAMESPACE, NVS_TYPE_ANY, &it);
    // Initiate the data holder
    db_data_array_t db_data = {
        .array = NULL,
        .size = 0
    };
    // Iterate over the keys, in that case timestamps
    while(res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        // Read the value from the NVS database
        // Put the read result inside the data holder and update its size
        err = read_time_entry(&time_table_handle, info.key, &db_data);
        if (err != ESP_OK) return err;
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    // Pushes the read database entries as data of the DB_EVENT_TABLE_READ event 
    db_dispatch_event(db_handler, DB_EVENT_TABLE_READ, &db_data);

    // Close
    nvs_close(time_table_handle);
    return ESP_OK;
}

/**
 * @brief Event loop register/unregister functions.
 * 
 * @param db Database handle.
 * @param event DB_EVENT_TABLE_READ Table is read.
 * @param event_handler *Not important*
 * @param event_handler_arg *Not important*
 * @return esp_err_t Status of the operation.
 */
esp_err_t db_register_events(db_event_handle_t db, db_event_t event, esp_event_handler_t event_handler, void* event_handler_arg)
{
    db_handler = db;
    if(!db) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_event_handler_register_with(db->event_handle, DB_EVENTS, event, event_handler, event_handler_arg);
}

esp_err_t db_unregister_events(db_event_handle_t db, db_event_t event, esp_event_handler_t event_handler)
{
    db_handler = NULL;
    if(!db) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_event_handler_unregister_with(db->event_handle, DB_EVENTS, event, event_handler);
}