#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_com.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ethernet";

static bool network_ready = false;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        network_ready = false;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        network_ready = false;
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    network_ready = true;
}

void ethernet_init()
{
    if (esp_netif_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_netif_init failed!");
        return;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed!");
        return;
    }

    ESP_LOGI(TAG, "Initializing Ethernet MAC for WirelessTag WT32-ETH01...");
    eth_esp32_emac_config_t mac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    mac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    mac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;
    mac_config.smi_mdc_gpio_num = GPIO_NUM_23;
    mac_config.smi_mdio_gpio_num = GPIO_NUM_18;

    eth_mac_config_t eth_mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_mac_config.sw_reset_timeout_ms = 1000;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config, &eth_mac_config);
    if (mac == NULL)
    {
        ESP_LOGE(TAG, "esp_eth_mac_new_esp32 failed!");
        return;
    }

    ESP_LOGI(TAG, "Initializing Ethernet PHY (LAN8720A) for WT32-ETH01...");
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = -1;
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

    // Enable external oscillator (pulled down at boot to allow IO0 strapping)
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 1));
    ESP_LOGI(TAG, "Starting Ethernet interface...");

    // Install and start Ethernet driver
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));
    if (eth_handle == NULL)
    {
        ESP_LOGE(TAG, "esp_eth_driver_install failed!");
        return;
    }

    esp_netif_config_t const netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *global_netif = esp_netif_new(&netif_config);
    esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
    if (eth_netif_glue == NULL)
    {
        ESP_LOGE(TAG, "esp_eth_new_netif_glue failed!");
        return;
    }
    ESP_ERROR_CHECK(esp_netif_attach(global_netif, eth_netif_glue));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
    ESP_LOGI(TAG, "Started Ethernet interface!");

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
}

bool ethernet_is_ready()
{
    return network_ready;
}

void ethernet_wait_for_network()
{
    for (size_t i = 0; i < 1000 && !network_ready; i++)
    {
        ESP_LOGI(TAG, "Wait for network %d / 1000", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    if (!network_ready)
    {
        ESP_LOGE(TAG, "Failed to acquire network, will reboot!");
        return;
    }

    ESP_LOGI(TAG, "Network is now available");
}