#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_mac.h"

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "../manager_base/manager_base.h"
#include "../lamp/lamp.h"
#include "../curtain/curtain.h"
#include "../other_device/other_device.h"

#include "../json.hpp"


#include "my_mqtt.h"

static const char *TAG = "mqtt";

esp_mqtt_client_handle_t client;

char str_mac[20];
char full_topic[40];

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

// 固定间隔上报所有设备状态
static void report_state_task(void *param) {
    while (true) {
        report_states();

        vTaskDelay(60000 / portTICK_PERIOD_MS);

        UBaseType_t remaining_stack = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(__func__, "Remaining stack size: %u", remaining_stack);
    }
    vTaskDelete(nullptr);
}

// 固定间隔上报过去一分钟的操作记录
static void report_operation_task(void *param) {
    while (true) {
        auto logs = fetch_and_clear_logs();
        if (!logs.empty()) {
            nlohmann::json json_array = logs;
            std::string json_str = json_array.dump();

            mqtt_publish_message(json_str, 0, 0);
        } else {
            ESP_LOGI(TAG, "日志数组为空, 无需发送");
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(nullptr);
}
static void register_the_rcu();
static void handle_mqtt_data(const char* data, size_t data_len);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        esp_mqtt_client_subscribe(client, full_topic, 0);
        printf("已订阅接收主题: %s\n", full_topic);

        // 注册本机
        register_the_rcu();
        // 启动报告状态任务
        xTaskCreate(report_state_task, "report_state_task", 4096, nullptr, 3, nullptr);
        // 启动报告操作任务
        xTaskCreate(report_operation_task, "report_operation_task", 4096, nullptr, 3, nullptr);

        break;

    case MQTT_EVENT_DATA:
    {
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        std::string msg_data(event->data, event->data_len);
        handle_mqtt_data(msg_data.c_str(), msg_data.size());
    }
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
        .task = {.stack_size = 8192},
        .buffer = {.size = 20480},
    };

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(str_mac, sizeof(str_mac), "%02X%02X%02X%02X%02X%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(full_topic, sizeof(full_topic), "/XZRCU/DOWN/%s", str_mac);

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

void mqtt_publish_message(const std::string& message, int qos, int retain) {
    char up_topic[40];
    snprintf(up_topic, sizeof(up_topic), "/XZRCU/UP/%s", str_mac);
    int msg_id = esp_mqtt_client_publish(client, up_topic, message.c_str(), message.length(), qos, retain);
    ESP_LOGI(TAG, "Message sent, msg_id=%d, topic=%s, message=%s", msg_id, up_topic, message.c_str());
}

// 注册时携带所有信息
static void register_the_rcu() {
    nlohmann::json j;
    j["mac"] = str_mac;
    j["type"] = "regis";
    auto& config_version = LordManager::getInstance().getConfigVersion();
    j["config_version"] = config_version;

    auto& hotel_name = LordManager::getInstance().getHotelName();
    j["hotel_name"] = hotel_name;

    auto& room_name = LordManager::getInstance().getRoomName();
    j["room_name"] = room_name;

    // 构造 lights
    j["lights"] = nlohmann::json::array();
    auto lamps = DeviceManager::getInstance().getDevicesOfType<Lamp>();
    for (auto& lamp : lamps) {
        nlohmann::json lamp_obj;
        lamp_obj["id"] = std::to_string(lamp->uid);
        lamp_obj["name"] = lamp->name;
        lamp_obj["output_type"] = std::to_string(static_cast<int>(lamp->output->type));
        lamp_obj["output_num"] = std::to_string(lamp->output->channel);
        lamp_obj["state"] = lamp->isOn() ? "1" : "0";
        j["lights"].push_back(lamp_obj);
    }

    // 构造 curtains
    j["curtains"] = nlohmann::json::array();
    auto curtains = DeviceManager::getInstance().getDevicesOfType<Curtain>();
    for (auto& curtain : curtains) {
        nlohmann::json curtain_obj;
        curtain_obj["id"] = std::to_string(curtain->uid);
        curtain_obj["name"] = curtain->name;
        curtain_obj["output_open_type"] = std::to_string(static_cast<int>(curtain->output_open->type));
        curtain_obj["output_open_num"] = std::to_string(curtain->output_open->channel);
        curtain_obj["output_close_type"] = std::to_string(static_cast<int>(curtain->output_close->type));
        curtain_obj["output_close_num"] = std::to_string(curtain->output_close->channel);
        curtain_obj["state"] = curtain->getState() ? "1" : "0";
        j["curtains"].push_back(curtain_obj);
    }

    // 构造 other(device)
    j["others"] = nlohmann::json::array();
    auto others = DeviceManager::getInstance().getDevicesOfType<OtherDevice>();
    for (auto& other : others) {
        if (other->type == OtherDeviceType::OUTPUT_CONTROL) {
            nlohmann::json other_obj;
            other_obj["id"] = std::to_string(other->uid);
            other_obj["name"] = other->name;
            other_obj["output_type"] = std::to_string(static_cast<int>(other->output->type));
            other_obj["output_num"] = std::to_string(other->output->channel);
            other_obj["state"] = other->isOn() ? "1" : "0";
            j["others"].push_back(other_obj);
        }
    }

    // 构造 modes
    j["modes"] = nlohmann::json::array();
    auto& panels = PanelManager::getInstance().getAllItems();
    for (auto& [_, panel] : panels) {
        for (auto& [_, button] : panel->buttons) {
            if (!button->mode_name.empty()) {
                j["modes"].push_back(button->mode_name);
            }
        }
    }
    // 没找到的话就在输入里找到
    auto& boards = BoardManager::getInstance().getAllItems();
    for (auto& [_, board] : boards) {
        for (auto& input : board->inputs) {
            if (!input.mode_name.empty()) {
                j["modes"].push_back(input.mode_name);
            }
        }
    }


    std::string json_str = j.dump();
    mqtt_publish_message(json_str.c_str(), 0, 0);
}

void report_states() {
    nlohmann::json j;
    j["mac"] = str_mac;
    j["type"] = "alldevicestate";

    // 构造 lights
    j["lights"] = nlohmann::json::array();
    auto lamps = DeviceManager::getInstance().getDevicesOfType<Lamp>();
    for (auto& lamp : lamps) {
        nlohmann::json lamp_obj;
        lamp_obj["id"] = std::to_string(lamp->uid);
        // lamp_obj["name"] = lamp->name;
        // lamp_obj["output_type"] = std::to_string(static_cast<int>(lamp->output->type));
        // lamp_obj["output_num"] = std::to_string(lamp->output->channel);
        lamp_obj["state"] = lamp->isOn() ? "1" : "0";
        j["lights"].push_back(lamp_obj);
    }

    // 构造 curtains
    j["curtains"] = nlohmann::json::array();
    auto curtains = DeviceManager::getInstance().getDevicesOfType<Curtain>();
    for (auto& curtain : curtains) {
        nlohmann::json curtain_obj;
        curtain_obj["id"] = std::to_string(curtain->uid);
        // curtain_obj["name"] = curtain->name;
        // curtain_obj["output_open_type"] = std::to_string(static_cast<int>(curtain->output_open->type));
        // curtain_obj["output_open_num"] = std::to_string(curtain->output_open->channel);
        // curtain_obj["output_close_type"] = std::to_string(static_cast<int>(curtain->output_close->type));
        // curtain_obj["output_close_num"] = std::to_string(curtain->output_close->channel);
        curtain_obj["state"] = curtain->getState() ? "1" : "0";
        j["curtains"].push_back(curtain_obj);
    }

    // 构造 other(device)
    j["others"] = nlohmann::json::array();
    auto others = DeviceManager::getInstance().getDevicesOfType<OtherDevice>();
    for (auto& other : others) {
        if (other->type == OtherDeviceType::OUTPUT_CONTROL) {
            nlohmann::json other_obj;
            other_obj["id"] = std::to_string(other->uid);
            // other_obj["name"] = other->name;
            // other_obj["output_type"] = std::to_string(static_cast<int>(other->output->type));
            // other_obj["output_num"] = std::to_string(other->output->channel);
            other_obj["state"] = other->isOn() ? "1" : "0";
            j["others"].push_back(other_obj);
        }
    }

    // 模式
    j["mode"] = LordManager::getInstance().getCurrMode();

    // 状态
    j["states"] = get_states();

    std::string json_str = j.dump();
    mqtt_publish_message(json_str.c_str(), 0, 0);
}


static void handle_mqtt_data(const char* data, size_t data_len) {
    // 将 data 转换为 string，以便用 nlohmann::json::parse
    std::string payload(data, data_len);
    nlohmann::json j = nlohmann::json::parse(payload);

    if (!j.is_object()) {
        ESP_LOGE(TAG, "Received JSON is not an object!");
        return;
    }

    // 解析字段 "type"
    if (j.contains("type") && j["type"].is_string()) {
        std::string type = j["type"].get<std::string>();
        // 被催促, 上报状态
        if (type == "urge") {
            report_states();
            return;
        }
        if (type != "ctl") {
            ESP_LOGE(TAG, "至少现在, type只应该是ctl");
            return;
        }
    } else {
        ESP_LOGW(TAG, "Field 'type' is missing or not a string.");
    }

    // 解析字段 "devicetype"
    std::string devicetype;
    if (j.contains("devicetype") && j["devicetype"].is_string()) {
        devicetype = j["devicetype"].get<std::string>();
        ESP_LOGI(TAG, "devicetype: %s", devicetype.c_str());
    } else {
        ESP_LOGW(TAG, "Field 'devicetype' is missing or not a string.");
    }

    // 解析字段 "deviceid"
    uint16_t deviceid_u16 = 0;
    bool deviceid_valid = false;
    if (j.contains("deviceid") && j["deviceid"].is_string()) {
        std::string deviceid_str = j["deviceid"].get<std::string>();
        ESP_LOGI(TAG, "deviceid: %s", deviceid_str.c_str());

        int deviceid = std::stoi(deviceid_str);
        if (deviceid >= 0 && deviceid <= std::numeric_limits<uint16_t>::max()) {
            deviceid_u16 = static_cast<uint16_t>(deviceid);
            deviceid_valid = true;
        } else {
            ESP_LOGE(TAG, "deviceid超出了uint16_t的范围: %d", deviceid);
        }
    } else {
        ESP_LOGW(TAG, "Field 'deviceid' is missing or not a string.");
    }

    // 解析字段 "operation"
    std::string operation_str;
    if (j.contains("operation") && j["operation"].is_string()) {
        operation_str = j["operation"].get<std::string>();
        ESP_LOGI(TAG, "operation: %s", operation_str.c_str());
    } else {
        ESP_LOGW(TAG, "Field 'operation' is missing or not a string.");
    }

    // 解析字段 "param"
    std::string param_str;
    if (j.contains("param") && j["param"].is_string()) {
        param_str = j["param"].get<std::string>();
        ESP_LOGI(TAG, "param: %s", param_str.c_str());
    } else {
        ESP_LOGW(TAG, "Field 'param' is missing or not a string.");
    }

    // 根据解析到的字段值执行操作
    if (devicetype == "mode") {
        // 切换模式
        auto& panels = PanelManager::getInstance().getAllItems();
        // 遍历所有面板的按钮, 执行对应的模式按钮
        for (auto& [_, panel] : panels) {
            for (auto& [_, button] : panel->buttons) {
                if (button->mode_name == operation_str) {
                    button->execute();
                    return;
                }
            }
        }
        // 没找到的话就在输入里找到
        auto& boards = BoardManager::getInstance().getAllItems();
        for (auto& [_, board] : boards) {
            for (auto& input : board->inputs) {
                if (input.mode_name == operation_str) {
                    input.execute();
                    return;
                }
            }
        }
        // ...否则就不存在啊
        ESP_LOGE(TAG, "不存在的模式: %s", operation_str.c_str());
    } else {
        // 设备控制
        if (deviceid_valid) {
            // 设备控制绝对退出模式
            LordManager::getInstance().setCurrMode("");
            auto device = DeviceManager::getInstance().getItem(deviceid_u16);
            device->execute(operation_str.c_str(), param_str);
        }
    }

    // add_log_entry(devicetype, deviceid_u16, operation_str, param_val);
}