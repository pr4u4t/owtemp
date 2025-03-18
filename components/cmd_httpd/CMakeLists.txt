idf_component_register(SRCS "cmd_http.c"
                    INCLUDE_DIRS .
                    REQUIRES console nvs_flash esp_http_server json) 
