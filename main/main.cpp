extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "stdio.h"
#include "esp_ota_ops.h"

// #include "../components/ethernet/ethernet.h"
}

#include <sys/stat.h>
#include <map>
#include "../json.hpp"


#include "board_config.h"
#include "lamp.h"
#include "air_conditioner.h"
#include "rs485.h"
#include "panel.h"
#include "curtain.h"
#include "wifi.h"
#include "../components/other_device/other_device.h"
#include "lwip/sockets.h"

#define PORT 8080
#define TAG "main.cpp"
#define FILE_PATH "/spiffs/rcu_config.json"

void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = nullptr,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        }
    }
}

std::string read_json_to_string(const std::string& filepath) {
    // 打开文件
    int file_fd = open(filepath.c_str(), O_RDONLY);
    if (file_fd < 0) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath.c_str());
        return "";
    }

    // 获取文件大小
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) < 0) {
        ESP_LOGE(TAG, "Failed to get file stat: %s", filepath.c_str());
        close(file_fd);
        return "";
    }
    size_t file_size = file_stat.st_size;
    if (file_size <= 0) {
        ESP_LOGE(TAG, "File is empty: %s", filepath.c_str());
        close(file_fd);
        return "";
    }

    // 读取文件内容
    std::string content(file_size, '\0');
    ssize_t read_size = read(file_fd, &content[0], file_size);
    close(file_fd);

    if (read_size < 0 || static_cast<size_t>(read_size) != file_size) {
        ESP_LOGE(TAG, "Failed to read file: %s", filepath.c_str());
        return "";
    }

    ESP_LOGI(TAG, "File read successfully: %s", filepath.c_str());
    return content;
}

// 解析json
void parseJson(const std::string& json_str) {
    // clean all
    // 现在只有三个Manager
    DeviceManager::getInstance().clear();
    BoardManager::getInstance().clear();
    PanelManager::getInstance().clear();

    ESP_LOGI(TAG, "Free heap before parse: %ld bytes", esp_get_free_heap_size());

    nlohmann::json json_data;
    // try {
    json_data = nlohmann::json::parse(json_str);
    // } catch (const nlohmann::json::parse_error& e) {
    //     ESP_LOGE(TAG, "JSON parse error: %s (at byte %zu)", e.what(), e.byte);
    //     return;
    // }

    ESP_LOGI(TAG, "解析 一般 配置");
    // ****************** 基本信息 ******************
    if (json_data.contains("一般配置") && json_data["一般配置"].is_object()) {
        auto& rcu_common_config = json_data["一般配置"];
        LordManager::getInstance().setConfig(
            rcu_common_config["config_version"].get<std::string>(),
            rcu_common_config["hotel_name"].get<std::string>(),
            rcu_common_config["room_name"].get<std::string>()
        );
    }

    ESP_LOGI(TAG, "解析 板子 配置");
    // ****************** 板子 ******************
    if (json_data.contains("板子列表") && json_data["板子列表"].is_array()) {
        for (auto& item : json_data["板子列表"]) {
            auto board = std::make_shared<BoardConfig>();
            board->id = item["id"].get<unsigned int>();

            ESP_LOGI(TAG, "解析 板 %d 输出列表", board->id);
            if (item.contains("outputs") && item["outputs"].is_array()) {
                for (auto& output_item : item["outputs"]) {
                    auto output = std::make_shared<BoardOutput>();
                    output->host_board_id = output_item["hostBoardId"].get<unsigned int>();
                    output->type = static_cast<OutputType>(output_item["type"].get<int>());
                    output->channel = output_item["channel"].get<unsigned int>();
                    output->uid = output_item["uid"].get<unsigned int>();
                    board->outputs[output->uid] = output;
                }
            }
            BoardManager::getInstance().addItem(board->id, board);
        }
    }

    ESP_LOGI(TAG, "解析 灯 配置");
    // ****************** 灯 ******************
    if (json_data.contains("灯列表") && json_data["灯列表"].is_array()) {
        for (auto& item : json_data["灯列表"]) {
            auto lamp = std::make_shared<Lamp>();
            lamp->uid = item["uid"].get<unsigned int>();
            lamp->type = static_cast<LampType>(item["type"].get<int>());
            lamp->name = item["name"].get<std::string>();
            lamp->output = BoardManager::getInstance().getBoardOutput(
                item["outputUid"].get<unsigned int>());

            // 解析关联按钮
            if (item.contains("associatedButtons") && item["associatedButtons"].is_array()) {
                for (auto& btn_item : item["associatedButtons"]) {
                    uint8_t panel_id = static_cast<uint8_t>(btn_item["panelId"].get<unsigned int>());
                    uint8_t button_id = static_cast<uint8_t>(btn_item["buttonId"].get<unsigned int>());
                    lamp->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(lamp->uid, lamp);
        }
    }

    // 空调部分如有需要，可参考上面方式自行转换，这里注释掉了

    ESP_LOGI(TAG, "解析 窗帘 配置");
    // ****************** 窗帘 ******************
    if (json_data.contains("窗帘列表") && json_data["窗帘列表"].is_array()) {
        for (auto& item : json_data["窗帘列表"]) {
            auto curtain = std::make_shared<Curtain>();
            curtain->uid = item["uid"].get<unsigned int>();
            curtain->name = item["name"].get<std::string>();
            curtain->output_open = BoardManager::getInstance().getBoardOutput(
                item["outputOpenUid"].get<unsigned int>());
            curtain->output_close = BoardManager::getInstance().getBoardOutput(
                item["outputCloseUid"].get<unsigned int>());
            curtain->run_duration = item["runDuration"].get<unsigned int>();

            // 解析关联按钮
            if (item.contains("associatedButtons") && item["associatedButtons"].is_array()) {
                for (auto& btn_item : item["associatedButtons"]) {
                    uint8_t panel_id = static_cast<uint8_t>(btn_item["panelId"].get<unsigned int>());
                    uint8_t button_id = static_cast<uint8_t>(btn_item["buttonId"].get<unsigned int>());
                    curtain->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(curtain->uid, curtain);
        }
    }

    ESP_LOGI(TAG, "解析 RS485 配置");
    // ****************** RS485指令码 ******************
    if (json_data.contains("485指令码列表") && json_data["485指令码列表"].is_array()) {
        for (auto& item : json_data["485指令码列表"]) {
            auto command = std::make_shared<RS485Command>();
            command->uid = item["uid"].get<unsigned int>();
            command->name = item["name"].get<std::string>();
            command->code = pavectorseHexToFixedArray(item["code"].get<std::string>());
            DeviceManager::getInstance().addItem(command->uid, command);
        }
    }

    ESP_LOGI(TAG, "解析 其他设备 配置");
    // ****************** 其他设备 ******************
    if (json_data.contains("其他设备列表") && json_data["其他设备列表"].is_array()) {
        for (auto& item : json_data["其他设备列表"]) {
            auto device = std::make_shared<OtherDevice>();
            device->uid = item["uid"].get<unsigned int>();
            device->type = static_cast<OtherDeviceType>(item["type"].get<int>());
            device->name = item["name"].get<std::string>();
            if (device->type == OtherDeviceType::OUTPUT_CONTROL) {
                device->output = BoardManager::getInstance().getBoardOutput(
                    item["outputUid"].get<unsigned int>());
            }

            // 解析关联按钮
            if (item.contains("associatedButtons") && item["associatedButtons"].is_array()) {
                for (auto& btn_item : item["associatedButtons"]) {
                    uint8_t panel_id = static_cast<uint8_t>(btn_item["panelId"].get<unsigned int>());
                    uint8_t button_id = static_cast<uint8_t>(btn_item["buttonId"].get<unsigned int>());
                    device->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(device->uid, device);
        }
    }

    ESP_LOGI(TAG, "解析 面板 配置");
    // ****************** 面板 ******************
    if (json_data.contains("面板列表") && json_data["面板列表"].is_array()) {
        for (auto& item : json_data["面板列表"]) {
            auto panel = std::make_shared<Panel>();
            panel->id = item["id"].get<unsigned int>();
            panel->name = item["name"].get<std::string>();

            if (item.contains("buttons") && item["buttons"].is_array()) {
                for (auto& button_item : item["buttons"]) {
                    auto button = std::make_shared<PanelButton>();
                    button->id = button_item["id"].get<unsigned int>();
                    button->host_panel = panel;
                    if (button_item.contains("modeName")) {
                        button->mode_name = button_item["modeName"].get<std::string>();
                    }

                    if (button_item.contains("actionGroups") && button_item["actionGroups"].is_array()) {
                        for (auto& action_group_item : button_item["actionGroups"]) {
                            auto action_group = std::make_shared<PanelButtonActionGroup>();
                            action_group->uid = action_group_item["uid"].get<int>();
                            action_group->pressed_polit_actions = 
                                static_cast<ButtonPolitAction>(action_group_item["pressedPolitAction"].get<int>());
                            action_group->pressed_other_polit_actions = 
                                static_cast<ButtonOtherPolitAction>(action_group_item["pressedOtherPolitAction"].get<int>());

                            if (action_group_item.contains("atomicActions") && action_group_item["atomicActions"].is_array()) {
                                for (auto& atomic_action_item : action_group_item["atomicActions"]) {
                                    AtomicAction atomic_action;
                                    atomic_action.target_device = DeviceManager::getInstance().getItem(
                                        atomic_action_item["deviceUid"].get<unsigned int>());
                                    atomic_action.operation = atomic_action_item["operation"].get<std::string>();
                                    atomic_action.parameter = atomic_action_item["parameter"].get<std::string>();

                                    action_group->atomic_actions.push_back(atomic_action);
                                }
                            }
                            ActionGroupManager::getInstance().addItem(action_group->uid, action_group);
                            button->action_groups.push_back(action_group);
                        }
                    }
                    panel->buttons[button->id] = button;
                }
            }
            PanelManager::getInstance().addItem(panel->id, panel);
        }
    }

    ESP_LOGI(TAG, "解析 板子 配置(输入配置)");
    // 再次解析板子，填充输入，因为现在设备已经存在
    if (json_data.contains("板子列表") && json_data["板子列表"].is_array()) {
        for (auto& item : json_data["板子列表"]) {
            auto board = BoardManager::getInstance().getItem(item["id"].get<unsigned int>());

            ESP_LOGI(TAG, "解析 板 %d 输入列表", board->id);
            if (item.contains("inputs") && item["inputs"].is_array()) {
                for (auto& input_item : item["inputs"]) {
                    BoardInput input;
                    input.host_board_id = input_item["hostBoardId"].get<unsigned int>();
                    input.channel = input_item["channel"].get<unsigned int>();
                    input.level = static_cast<InputLevel>(input_item["level"].get<int>());
                    if (input_item.contains("modeName")) {
                        input.mode_name = input_item["modeName"].get<std::string>();
                    }

                    if (input_item.contains("actionGroups") && input_item["actionGroups"].is_array()) {
                        for (auto& action_group_item : input_item["actionGroups"]) {
                            auto action_group = std::make_shared<InputActionGroup>();
                            action_group->uid = action_group_item["uid"].get<int>();

                            if (action_group_item.contains("atomicActions") && action_group_item["atomicActions"].is_array()) {
                                for (auto& atomic_action_item : action_group_item["atomicActions"]) {
                                    AtomicAction atomic_action;
                                    atomic_action.target_device = DeviceManager::getInstance().getItem(
                                        atomic_action_item["deviceUid"].get<unsigned int>());
                                    atomic_action.operation = atomic_action_item["operation"].get<std::string>();
                                    atomic_action.parameter = atomic_action_item["parameter"].get<std::string>();

                                    action_group->atomic_actions.push_back(atomic_action);
                                }
                            }
                            ActionGroupManager::getInstance().addItem(action_group->uid, action_group);
                            input.action_groups.push_back(action_group);
                        }
                    }
                    board->inputs.push_back(input);
                }
            }
        }
    }

    ESP_LOGI(TAG, "解析所有设备的关联按钮");
    // 窗帘
    auto curtains = DeviceManager::getInstance().getDevicesOfType<Curtain>();
    for (auto& curtain : curtains) {
        for (auto& associated_button : curtain->associated_buttons) {
            auto panel_instance = PanelManager::getInstance().getItem(associated_button.panel_id);
            auto panel_button = panel_instance->buttons[associated_button.button_id];
            for (auto& action_group : panel_button->action_groups) {
                for (auto& atomic_action : action_group->atomic_actions) {
                    if (atomic_action.operation == "打开") {
                        curtain->open_buttons.push_back(panel_button);
                    } else if (atomic_action.operation == "关闭") {
                        curtain->close_buttons.push_back(panel_button);
                    } else if (atomic_action.operation == "反转") {
                        curtain->reverse_buttons.push_back(panel_button);
                    }
                }
            }
        }
    }

    ESP_LOGI(TAG, "所有配置已成功解析");
}

// 接收/发送json的tcp任务
void tcp_server_task(void *pvParameters) {
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(nullptr);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(nullptr);
    }

    if (listen(listen_sock, 1) != 0) {
        ESP_LOGE(TAG, "Socket unable to listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(nullptr);
    }

    ESP_LOGI(TAG, "Socket listening on port %d", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        uint32_t addr_len = sizeof(client_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            continue;
        }

        ESP_LOGI(TAG, "Socket accepted");

        std::string received_data;
        char recv_buf[1024];

        // 循环读取数据，直到客户端关闭连接
        while (1) {
            int len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed by client");
                break;
            } else {
                recv_buf[len] = '\0';
                received_data += recv_buf;

                // 当检测到换行（或协议规定的结束标记）就不再继续读取
                if (received_data.find('\n') != std::string::npos || 
                    received_data.find('\r') != std::string::npos) {
                    break;
                }
            }
        }

        // 检查是否是 "GET_FILE" 请求
        if (received_data == "GET_FILE\r" || received_data == "GET_FILE\n" || received_data == "GET_FILE") {
            ESP_LOGI(TAG, "GET_FILE request received");
            int file_fd = open(FILE_PATH, O_RDONLY);
            if (file_fd < 0) {
                ESP_LOGE(TAG, "Failed to open file for reading");
                std::string error_msg = "ERROR: File not found\n";
                send(sock, error_msg.c_str(), error_msg.size(), 0);
            } else {
                char file_buf[1024];
                int read_bytes;
                while ((read_bytes = read(file_fd, file_buf, sizeof(file_buf))) > 0) {
                    printf("send bytes\n");
                    send(sock, file_buf, read_bytes, 0);
                }
                close(file_fd);
                // 在数据末尾发送结束标志
                const char* end_marker = "\nEND_OF_JSON\n";
                send(sock, end_marker, strlen(end_marker), 0);
                printf("Send end_marker\n");
                
                ESP_LOGI(TAG, "File sent successfully");
            }
        } else {
            ESP_LOGI(TAG, "未接受到 GET_FILE, 视为json配置");
            // 保存接收到的数据到 SPIFFS 文件
            int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (file_fd < 0) {
                ESP_LOGE(TAG, "Failed to open file for writing");
            } else {
                if (write(file_fd, received_data.c_str(), received_data.size()) < 0) {
                    ESP_LOGE(TAG, "Failed to write to file");
                } else {
                    ESP_LOGI(TAG, "Data saved to file successfully");
                    // 写入文件后直接开始解析并应用
                    parseJson(received_data);
                }
                close(file_fd);
            }
        }
        close(sock);
    }
    close(listen_sock);
    vTaskDelete(nullptr);
}

static void monitor_task(void *pvParameter)
{
	while(1)
	{
        ESP_LOGI(TAG, "%d %d %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size( MALLOC_CAP_DMA ), heap_caps_get_free_size( MALLOC_CAP_SPIRAM ));
        UBaseType_t remaining_stack = uxTaskGetStackHighWaterMark(nullptr);
        ESP_LOGI("total", "Remaining stack size: %u", remaining_stack);
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
extern "C" void app_main(void)
{
    printf("Hello world!\n");
    init_spiffs();
    // uart_init_stm32();
    uart_init_rs485();
    // ethernet_init();

    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

    xTaskCreate([] (void *param) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 总之, 就是要稍微等一下, 不然第二次parseJson时会崩溃
        parseJson(read_json_to_string(FILE_PATH));
        wifi_init_sta();
        xTaskCreate(tcp_server_task, "tcp_server_task", 8192, nullptr, 5, nullptr);
        vTaskDelete(nullptr);
    }, "parse_json_task", 8192, nullptr, 3, nullptr);
}
