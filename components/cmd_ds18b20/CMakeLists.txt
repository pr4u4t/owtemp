idf_component_register(SRCS "cmd_ds18b20.c"
                    INCLUDE_DIRS .
                    REQUIRES console spi_flash driver esp_driver_gpio nvs_flash onewire_bus ds18b20)
