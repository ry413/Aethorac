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
#include "../components/rapidjson/document.h"
#include "../components/rapidjson/error/en.h"
#include "../components/rapidjson/stringbuffer.h"
#include "../components/rapidjson/writer.h"

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
    
    rapidjson::Document json_data;
    ESP_LOGI(TAG, "Free heap before parse: %ld bytes", esp_get_free_heap_size());
    if (json_data.Parse(json_str.c_str()).HasParseError()) {
        ESP_LOGE(TAG, "JSON parse error: %s (at offset %u)",
                 rapidjson::GetParseError_En(json_data.GetParseError()),
                 json_data.GetErrorOffset());
        return;
    }

    ESP_LOGI(TAG, "解析 板子 配置");
    // ****************** 板子 ******************
    if (json_data.HasMember("板子列表") && json_data["板子列表"].IsArray()) {
        const rapidjson::Value& board_list = json_data["板子列表"];
        for (rapidjson::SizeType i = 0; i < board_list.Size(); ++i) {
            const rapidjson::Value& item = board_list[i];
            auto board = std::make_shared<BoardConfig>();
            board->id = item["id"].GetUint();

            ESP_LOGI(TAG, "解析 板 %d 输出列表", board->id);
            if (item.HasMember("outputs") && item["outputs"].IsArray()) {
                const rapidjson::Value& outputs = item["outputs"];
                for (rapidjson::SizeType j = 0; j < outputs.Size(); ++j) {
                    const rapidjson::Value& output_item = outputs[j];
                    auto output = std::make_shared<BoardOutput>();
                    output->host_board_id = output_item["hostBoardId"].GetUint();
                    output->type = static_cast<OutputType>(output_item["type"].GetInt());
                    output->channel = output_item["channel"].GetUint();
                    output->uid = output_item["uid"].GetUint();
                    board->outputs[output->uid] = output;
                }
            }
            BoardManager::getInstance().addItem(board->id, board);
        }
    }

    ESP_LOGI(TAG, "解析 灯 配置");
    // ****************** 灯 ******************
    if (json_data.HasMember("灯列表") && json_data["灯列表"].IsArray()) {
        const rapidjson::Value& lamp_list = json_data["灯列表"];
        for (rapidjson::SizeType i = 0; i < lamp_list.Size(); ++i) {
            const rapidjson::Value& item = lamp_list[i];
            auto lamp = std::make_shared<Lamp>();
            lamp->uid = item["uid"].GetUint();
            lamp->type = static_cast<LampType>(item["type"].GetInt());
            lamp->name = item["name"].GetString();
            lamp->output = BoardManager::getInstance().getBoardOutput(
                item["outputUid"].GetUint());
            
            // 解析关联按钮
            if (item.HasMember("associatedButtons") && item["associatedButtons"].IsArray()) {
                const rapidjson::Value& buttons = item["associatedButtons"];
                for (rapidjson::SizeType j = 0; j < buttons.Size(); ++j) {
                    const rapidjson::Value& btn_item = buttons[j];
                    uint8_t panel_id = btn_item["panelId"].GetUint();
                    uint8_t button_id = btn_item["buttonId"].GetUint();

                    lamp->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(lamp->uid, lamp);
        }
    }

    // ESP_LOGI(TAG, "解析 空调 配置");
    // // ****************** 空调 ******************
    // auto& airConManager = AirConManager::getInstance();
    // if (json_data.HasMember("空调通用配置") && json_data["空调通用配置"].IsObject()) {
    //     const rapidjson::Value& ac_common_config = json_data["空调通用配置"];
    //     airConManager.default_target_temp = ac_common_config["defaultTemp"].GetUint();
    //     airConManager.default_mode = static_cast<ACMode>(ac_common_config["defaultMode"].GetInt());
    //     airConManager.default_wind_speed = static_cast<ACWindSpeed>(ac_common_config["defaultFanSpeed"].GetInt());
    //     airConManager.stopThreshold = ac_common_config["stopThreshold"].GetUint();
    //     airConManager.rework_threshold = ac_common_config["reworkThreshold"].GetUint();
    //     airConManager.stop_action = static_cast<ACStopAction>(ac_common_config["stopAction"].GetInt());
    //     airConManager.low_diff = ac_common_config["lowFanTempDiff"].GetUint();
    //     airConManager.high_diff = ac_common_config["highFanTempDiff"].GetUint();
    //     airConManager.auto_fun_wind_speed = static_cast<ACWindSpeed>(ac_common_config["autoVentSpeed"].GetInt());
    // }

    // if (json_data.HasMember("空调列表") && json_data["空调列表"].IsArray()) {
    //     const rapidjson::Value& ac_list = json_data["空调列表"];
    //     for (rapidjson::SizeType i = 0; i < ac_list.Size(); ++i) {
    //         const rapidjson::Value& item = ac_list[i];
    //         ACType ac_type = static_cast<ACType>(item["type"].GetInt());
    //         switch (ac_type) {
    //             case ACType::SINGLE_PIPE_FCU: {
    //                 auto ac = std::make_shared<SinglePipeFCU>();
    //                 ac->ac_id = item["id"].GetUint();
    //                 ac->ac_type = ac_type;
    //                 ac->power_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelPowerUid"].GetUint());
    //                 ac->low_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelLowUid"].GetUint());
    //                 ac->mid_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelMidUid"].GetUint());
    //                 ac->high_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelHighUid"].GetUint());
    //                 ac->water1_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelWater1Uid"].GetUint());
    //                 airConManager.addItem(ac->ac_id, ac);
    //                 break;
    //             }
    //             case ACType::DOUBLE_PIPE_FCU: {
    //                 auto ac = std::make_shared<DoublePipeFCU>();
    //                 ac->ac_id = item["id"].GetUint();
    //                 ac->ac_type = ac_type;
    //                 ac->power_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelPowerUid"].GetUint());
    //                 ac->low_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelLowUid"].GetUint());
    //                 ac->mid_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelMidUid"].GetUint());
    //                 ac->high_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelHighUid"].GetUint());
    //                 ac->water1_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelWater1Uid"].GetUint());
    //                 ac->water2_channel = BoardManager::getInstance().getBoardOutput(
    //                     item["channelWater2Uid"].GetUint());
    //                 airConManager.addItem(ac->ac_id, ac);
    //                 break;
    //             }
    //             case ACType::INFRARED: {
    //                 auto ac = std::make_shared<InfraredAC>();
    //                 ac->ac_id = item["id"].GetUint();
    //                 ac->ac_type = ac_type;
    //                 // Additional initializations if necessary
    //                 airConManager.addItem(ac->ac_id, ac);
    //                 break;
    //             }
    //             default:
    //                 ESP_LOGW(TAG, "Unknown AC type: %d", static_cast<int>(ac_type));
    //                 break;
    //         }
    //     }
    // }

    ESP_LOGI(TAG, "解析 窗帘 配置");
    // ****************** 窗帘 ******************
    if (json_data.HasMember("窗帘列表") && json_data["窗帘列表"].IsArray()) {
        const rapidjson::Value& curtain_list = json_data["窗帘列表"];
        for (rapidjson::SizeType i = 0; i < curtain_list.Size(); ++i) {
            const rapidjson::Value& item = curtain_list[i];
            auto curtain = std::make_shared<Curtain>();
            curtain->uid = item["uid"].GetUint();
            curtain->name = item["name"].GetString();
            curtain->output_open = BoardManager::getInstance().getBoardOutput(
                item["outputOpenUid"].GetUint());
            curtain->output_close = BoardManager::getInstance().getBoardOutput(
                item["outputCloseUid"].GetUint());
            curtain->run_duration = item["runDuration"].GetUint();

            // 解析关联按钮
            if (item.HasMember("associatedButtons") && item["associatedButtons"].IsArray()) {
                const rapidjson::Value& buttons = item["associatedButtons"];
                for (rapidjson::SizeType j = 0; j < buttons.Size(); ++j) {
                    const rapidjson::Value& btn_item = buttons[j];
                    uint8_t panel_id = btn_item["panelId"].GetUint();
                    uint8_t button_id = btn_item["buttonId"].GetUint();

                    curtain->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(curtain->uid, curtain);
        }
    }

    ESP_LOGI(TAG, "解析 RS485 配置");
    // ****************** RS485指令码 ******************
    if (json_data.HasMember("485指令码列表") && json_data["485指令码列表"].IsArray()) {
        const rapidjson::Value& command_list = json_data["485指令码列表"];
        for (rapidjson::SizeType i = 0; i < command_list.Size(); ++i) {
            const rapidjson::Value& item = command_list[i];
            auto command = std::make_shared<RS485Command>();
            command->uid = item["uid"].GetUint();
            command->name = item["name"].GetString();
            command->code = pavectorseHexToFixedArray(item["code"].GetString());

            // 这玩意拿关联按钮干什么?

            DeviceManager::getInstance().addItem(command->uid, command);
        }
    }

    ESP_LOGI(TAG, "解析 其他设备 配置");
    // ****************** 其他设备 ******************
    if (json_data.HasMember("其他设备列表") && json_data["其他设备列表"].IsArray()) {
        const rapidjson::Value& device_list = json_data["其他设备列表"];
        for (rapidjson::SizeType i = 0; i < device_list.Size(); ++i) {
            const rapidjson::Value& item = device_list[i];
            auto device = std::make_shared<OtherDevice>();
            device->uid = item["uid"].GetUint();
            device->type = static_cast<OtherDeviceType>(item["type"].GetInt());
            device->name = item["name"].GetString();
            if (device->type == OtherDeviceType::OUTPUT_CONTROL) {
                device->output = BoardManager::getInstance().getBoardOutput(
                        item["outputUid"].GetUint());
            }

            // 解析关联按钮
            if (item.HasMember("associatedButtons") && item["associatedButtons"].IsArray()) {
                const rapidjson::Value& buttons = item["associatedButtons"];
                for (rapidjson::SizeType j = 0; j < buttons.Size(); ++j) {
                    const rapidjson::Value& btn_item = buttons[j];
                    uint8_t panel_id = btn_item["panelId"].GetUint();
                    uint8_t button_id = btn_item["buttonId"].GetUint();

                    device->associated_buttons.push_back(AssociatedButton(panel_id, button_id));
                }
            }
            DeviceManager::getInstance().addItem(device->uid, device);
        }
    }

    ESP_LOGI(TAG, "解析 面板 配置");
    // ****************** 面板 ******************
    if (json_data.HasMember("面板列表") && json_data["面板列表"].IsArray()) {
        const rapidjson::Value& panel_list = json_data["面板列表"];
        for (rapidjson::SizeType i = 0; i < panel_list.Size(); ++i) {
            const rapidjson::Value& item = panel_list[i];
            auto panel = std::make_shared<Panel>();
            panel->id = item["id"].GetUint();
            panel->name = item["name"].GetString();

            if (item.HasMember("buttons") && item["buttons"].IsArray()) {
                const rapidjson::Value& buttons = item["buttons"];
                for (rapidjson::SizeType j = 0; j < buttons.Size(); ++j) {
                    const rapidjson::Value& button_item = buttons[j];
                    auto button = std::make_shared<PanelButton>();
                    button->id = button_item["id"].GetUint();
                    button->host_panel = panel;

                    if (button_item.HasMember("actionGroups") && button_item["actionGroups"].IsArray()) {
                        const rapidjson::Value& action_groups = button_item["actionGroups"];
                        for (rapidjson::SizeType k = 0; k < action_groups.Size(); ++k) {
                            const rapidjson::Value& action_group_item = action_groups[k];
                            auto action_group = std::make_shared<PanelButtonActionGroup>();
                            action_group->uid = action_group_item["uid"].GetInt();
                            action_group->pressed_polit_actions = static_cast<ButtonPolitAction>(action_group_item["pressedPolitAction"].GetInt());
                            action_group->pressed_other_polit_actions = static_cast<ButtonOtherPolitAction>(action_group_item["pressedOtherPolitAction"].GetInt()); 

                            if (action_group_item.HasMember("atomicActions") && action_group_item["atomicActions"].IsArray()) {
                                const rapidjson::Value& atomic_actions = action_group_item["atomicActions"];
                                for (rapidjson::SizeType l = 0; l < atomic_actions.Size(); ++l) {
                                    const rapidjson::Value& atomic_action_item = atomic_actions[l];
                                    AtomicAction atomic_action;
                                    atomic_action.target_device = DeviceManager::getInstance().getItem(atomic_action_item["deviceUid"].GetUint());
                                    atomic_action.operation = atomic_action_item["operation"].GetString();
                                    atomic_action.parameter = atomic_action_item["parameter"].GetInt();

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

    // 到这一步再解析板子的输入配置, 因为现在才有设备实例存在
    ESP_LOGI(TAG, "解析 板子 配置");
    // ****************** 板子 ******************
    if (json_data.HasMember("板子列表") && json_data["板子列表"].IsArray()) {
        const rapidjson::Value& board_list = json_data["板子列表"];
        for (rapidjson::SizeType i = 0; i < board_list.Size(); ++i) {
            const rapidjson::Value& item = board_list[i];
            // 重新根据json找到已存在的板子实例
            std::shared_ptr<BoardConfig> board = BoardManager::getInstance().getItem(item["id"].GetUint());

            ESP_LOGI(TAG, "解析 板 %d 输入列表", board->id);
            if (item.HasMember("inputs") && item["inputs"].IsArray()) {
                const rapidjson::Value& inputs = item["inputs"];
                for (rapidjson::SizeType j = 0; j < inputs.Size(); ++j) {
                    const rapidjson::Value& input_item = inputs[j];
                    BoardInput input;
                    input.host_board_id = input_item["hostBoardId"].GetUint();
                    input.channel = input_item["channel"].GetUint();
                    input.level = static_cast<InputLevel>(input_item["level"].GetInt());

                    if (input_item.HasMember("actionGroups") && input_item["actionGroups"].IsArray()) {
                        const rapidjson::Value& action_groups = input_item["actionGroups"];
                        for (rapidjson::SizeType k = 0; k < action_groups.Size(); ++k) {
                            const rapidjson::Value& action_group_item = action_groups[k];
                            auto action_group = std::make_shared<InputActionGroup>();
                            action_group->uid = action_group_item["uid"].GetInt();

                            if (action_group_item.HasMember("atomicActions") && action_group_item["atomicActions"].IsArray()) {
                                const rapidjson::Value& atomic_actions = action_group_item["atomicActions"];
                                for (rapidjson::SizeType l = 0; l < atomic_actions.Size(); ++l) {
                                    const rapidjson::Value& atomic_action_item = atomic_actions[l];
                                    AtomicAction atomic_action;
                                    atomic_action.target_device = DeviceManager::getInstance().getItem(atomic_action_item["deviceUid"].GetUint());
                                    atomic_action.operation = atomic_action_item["operation"].GetString();
                                    atomic_action.parameter = atomic_action_item["parameter"].GetInt();

                                    action_group->atomic_actions.push_back(atomic_action);
                                }
                            }
                            ActionGroupManager::getInstance().addItem(action_group->uid, action_group);
                            input.action_groups.push_back(action_group);
                        }
                    }
                    // 把输入添加到这个板子里就够了
                    board->inputs.push_back(input);
                }
            }
        }
    }

    ESP_LOGI(TAG, "解析所有设备的关联按钮");
    // 窗帘
    auto curtains = DeviceManager::getInstance().getDevicesOfType<Curtain>();
    for (auto& curtain : curtains) {
        // 遍历此窗帘的所有关联按钮
        for (auto& associated_button : curtain->associated_buttons) {
            // 遍历这个按钮的所有动作组
            auto& panel_button = PanelManager::getInstance().getItem(
                associated_button.panel_id)->buttons[associated_button.button_id];
            for (auto& action_group : panel_button->action_groups) {
                // 遍历这个动作组的所有原子级动作
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
    wifi_init_sta();
    // uart_init_stm32();
    uart_init_rs485();
    // ethernet_init();
    xTaskCreate(tcp_server_task, "tcp_server_task", 8192, nullptr, 5, nullptr);

    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

    // xTaskCreate([] (void *param) {
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);  // 总之, 就是要稍微等一下, 不然第二次parseJson时会崩溃
    //     parseJson(read_json_to_string(FILE_PATH));
    //     vTaskDelete(nullptr);
    // }, "parse_json_task", 8192, nullptr, 3, nullptr);
}
