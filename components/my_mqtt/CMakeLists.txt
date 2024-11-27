idf_component_register(SRCS "my_mqtt.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash mqtt app_update esp_https_ota)
