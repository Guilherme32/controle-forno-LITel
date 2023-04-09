idf_component_register(SRCS "main.c" "wifi.c" "storage.c" "server.c"
                    INCLUDE_DIRS ".")

spiffs_create_partition_image(storage ../web_code FLASH_IN_PROJECT)
