idf_component_register(SRCS "ethernet.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver esp_eth esp_netif)
