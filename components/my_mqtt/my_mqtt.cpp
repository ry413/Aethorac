#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_mac.h"

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"

#include "my_mqtt.h"

static const char *TAG = "mqtt";

#define MAX_PAYLOAD_SIZE 20480  // 设置负载最大容量，需大于 JSON 消息大小
static char payload_buffer[MAX_PAYLOAD_SIZE];
static int payload_len = 0;
static int topic_verified = 0;

esp_mqtt_client_handle_t client;

char str_mac[20];
char full_topic[40];
const char* boardcast_topic = "/AETHORAC/FFFF";

static esp_err_t ota_http_event_handler(esp_http_client_event_t *evt) {
    static int binary_file_length = 0;
    static int total_length = 0;
    static float last_reported_progress = 0;  // 上一次报告的进度
    const float progress_threshold = 1.0;     // 设定进度的变化阈值(1%)

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE("ota", "HTTP_EVENT_ERROR");
            binary_file_length = 0;
            total_length = 0;
            last_reported_progress = 0;
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI("ota", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI("ota", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI("ota", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            if (strcmp(evt->header_key, "Content-Length") == 0) {
                total_length = atoi(evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                binary_file_length += evt->data_len;
                if (total_length > 0) {
                    float progress = (binary_file_length * 100.0) / total_length;
                    if (progress - last_reported_progress >= progress_threshold) {
                        ESP_LOGI("ota", "Download progress: %.2f%%", progress);
                        // lv_label_set_text_fmt(ui_OTA_Progress_Text, "%d%%", (int)progress);
                        // lv_bar_set_value(ui_OTA_Progress_Bar, progress, LV_ANIM_OFF);
                        // last_reported_progress = progress;
                    }
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI("ota", "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("ota", "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI("ota", "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}
void ota_task(void *pvParameter)
{
    ESP_LOGI("ota", "Starting OTA example task");
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));

    // config.url = parsed.ota_url;
    config.crt_bundle_attach = esp_crt_bundle_attach; 
    config.event_handler = ota_http_event_handler;
    config.keep_alive_enable = true;
    config.skip_cert_common_name_check = true;
    config.use_global_ca_store = true;

    esp_https_ota_config_t ota_config;
    memset(&ota_config, 0, sizeof(ota_config));
    ota_config.http_config = &config;
    ESP_LOGI("ota", "Attempting to download update from %s", config.url);
    
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI("ota", "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE("ota", "Firmware upgrade failed");
        vTaskDelete(NULL);
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        esp_mqtt_client_subscribe(client, full_topic, 0);
        printf("已订阅接收主题: %s\n", full_topic);

        esp_mqtt_client_subscribe(client, boardcast_topic, 0);
        printf("已订阅接收主题: %s\n", boardcast_topic);
        break;

    case MQTT_EVENT_DATA:
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        // if ()
        // write_json_to_spiffs("/spiffs/rcu_config.json", event->data);
        // ...
    default:
        break;
    }
}

void mqtt_app_start() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {.address = {.uri = "mqtt://xiaozhuiot.cn:1883",}},
        .credentials = {
            .username = "xiaozhu",
            .authentication = {.password = "ieTOIugSDfieWw23gfiwefg"},
        },
        .session = {.protocol_ver = MQTT_PROTOCOL_V_5},
        .task = {.stack_size = 4096},
        .buffer = {.size = 20480},
    };

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(str_mac, sizeof(str_mac), "%02X%02X%02X%02X%02X%02X", 
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(full_topic, sizeof(full_topic), "/AETHORAC/%s", str_mac);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
void mqtt_app_stop(void) {
    esp_mqtt_client_stop(client);
}
void mqtt_app_cleanup(void) {
    esp_mqtt_client_destroy(client);
}

void mqtt_publish_message(const char *message, int qos, int retain) {
    char up_topic[40];
    snprintf(up_topic, sizeof(up_topic), "/XZBGS3UP/%s", str_mac);
    int msg_id = esp_mqtt_client_publish(client, up_topic, message, strlen(message), qos, retain);
    ESP_LOGI(TAG, "Message sent, msg_id=%d, topic=%s, message=%s", msg_id, up_topic, message);
}

// 写入数据到 SPIFFS 文件
void write_json_to_spiffs(const char *filename, const char *json_data) {
    // 打开文件（写入模式）
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("无法打开文件: %s\n", filename);
        return;
    }

    // 写入 JSON 数据
    fwrite(json_data, 1, strlen(json_data), file);

    // 关闭文件
    fclose(file);
    printf("数据已写入文件: %s\n", filename);
}