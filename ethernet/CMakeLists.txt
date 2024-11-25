idf_component_register(SRCS "ethernet.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_eth esp_netif)
