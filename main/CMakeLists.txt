idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES webapp esp-idf-rc522 wi-fi spiffs database)
set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../components/webapp")
if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()