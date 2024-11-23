extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "ui.h"

extern void lvgl_task(void *pvParamater);
}

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <map>
#include "../../libs/json.hpp"


// #include "config_structs.h"
#include "board_config.h"
#include "lamp.h"
#include "air_conditioner.h"
#include "rs485/rs485.h"
#include "action_group/action_group.h"
#include "panel.h"
#include "delay_action/delay_action.h"
#include "curtain/curtain.h"
#include "wifi.h"
#include "lwip/sockets.h"

#define PORT 8080
#define TAG "main.cpp"

void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            std::cerr << "Failed to mount or format SPIFFS" << std::endl;
        } else if (ret == ESP_ERR_NOT_FOUND) {
            std::cerr << "Failed to find SPIFFS partition" << std::endl;
        } else {
            std::cerr << "Failed to initialize SPIFFS: " << esp_err_to_name(ret) << std::endl;
        }
    }
}

std::string read_json_to_string(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }

    // 读取整个文件内容到std::string
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    return content;
}


std::shared_ptr<IActionTarget> getActionTarget(ActionType type, uint16_t uid) {
    switch (type) {
        case ActionType::LAMP:
            return LampManager::getInstance().getItem(uid);
        case ActionType::AC:
            return AirConManager::getInstance().getItem(uid);
        case ActionType::CURTAIN:
            return CurtainManager::getInstance().getItem(uid);
        case ActionType::RS485:
            return RS485Manager::getInstance().getItem(uid);
        case ActionType::RELAY:
            return BoardManager::getInstance().getBoardOutput(uid);
        case ActionType::ACTION_GROUP:
            return ActionGroupManager::getInstance().getItem(uid);
        case ActionType::DELAY:
            ESP_LOGE(__func__, "这里不应该传入DELAY");
            return nullptr;
        default:
            ESP_LOGE(__func__, "无法识别的类型");
            return nullptr;
    }
}

// 解析json配置
void parseJson(const std::string& json_str) {
    auto json_data = nlohmann::json::parse(json_str);
    ESP_LOGI(TAG, "解析 板子 配置");
    // ****************** 板子 ******************
    for (const auto& item : json_data["板子列表"]) {
        auto board = std::make_shared<BoardConfig>();
        board->id = item["id"].get<uint8_t>();

        ESP_LOGI(TAG, "解析 板 %d 输出列表...", board->id);
        for (const auto& output_item : item["outputs"]) {
            auto output = std::make_shared<BoardOutput>();
            output->host_board_id = output_item["hostBoardId"].get<uint8_t>();
            output->type = static_cast<OutputType>(output_item["type"].get<int>());
            output->channel = output_item["channel"].get<uint8_t>();
            output->uid = output_item["uid"].get<uint16_t>();
            board->outputs[output->uid] = output;
        }
        ESP_LOGI(TAG, "解析 板 %d 输入列表...", board->id);
        for (const auto& input_item : item["inputs"]) {
            BoardInput input;
            input.host_board_id = input_item["hostBoardId"].get<uint8_t>();
            input.channel = input_item["channel"].get<uint8_t>();
            input.level = static_cast<InputLevel>(input_item["level"].get<int>());
            input.action_group_uid = input_item["actionGroupUid"].get<uint16_t>();
            board->inputs.push_back(input);
        }
        BoardManager::getInstance().addItem(board->id, board);
    }

    ESP_LOGI(TAG, "解析 灯 配置");
    // ****************** 灯 ******************
    for (const auto& item : json_data["灯列表"]) {
        auto lamp = std::make_shared<Lamp>();
        lamp->uid = item["uid"].get<uint16_t>();
        lamp->type = item["type"].get<LampType>();
        lamp->name = item["name"].get<std::string>();
        lamp->channel_power = BoardManager::getInstance().getBoardOutput(
            item["channelPowerUid"].get<uint16_t>());
        LampManager::getInstance().addItem(lamp->uid, lamp);
    }
    
    ESP_LOGI(TAG, "解析 空调 配置");
    // ****************** 空调 ******************
    auto& airConManager = AirConManager::getInstance();
    airConManager.default_target_temp = json_data["空调通用配置"]["defaultTemp"].get<uint8_t>();
    airConManager.default_mode = json_data["空调通用配置"]["defaultMode"].get<ACMode>();
    airConManager.default_wind_speed = json_data["空调通用配置"]["defaultFanSpeed"].get<ACWindSpeed>();
    airConManager.stopThreshold = json_data["空调通用配置"]["stopThreshold"].get<uint8_t>();
    airConManager.rework_threshold = json_data["空调通用配置"]["reworkThreshold"].get<uint8_t>();
    airConManager.stop_action = json_data["空调通用配置"]["stopAction"].get<ACStopAction>();
    airConManager.low_diff = json_data["空调通用配置"]["lowFanTempDiff"].get<uint8_t>();
    airConManager.high_diff = json_data["空调通用配置"]["highFanTempDiff"].get<uint8_t>();
    airConManager.auto_fun_wind_speed = json_data["空调通用配置"]["autoVentSpeed"].get<ACWindSpeed>();

    for (const auto& item : json_data["空调列表"]) {
        ACType ac_type = item["type"].get<ACType>();
        switch (ac_type) {
            case ACType::SINGLE_PIPE_FCU: {
                auto ac = std::make_shared<SinglePipeFCU>();
                ac->ac_id = item["id"].get<uint8_t>();
                ac->ac_type = ac_type;
                ac->power_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelPowerUid"].get<uint16_t>());
                ac->low_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelLowUid"].get<uint16_t>());
                ac->mid_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelMidUid"].get<uint16_t>());
                ac->high_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelHighUid"].get<uint16_t>());
                ac->water1_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelWater1Uid"].get<uint16_t>());
                airConManager.addItem(ac->ac_id, ac);
                break;
            }
            case ACType::DOUBLE_PIPE_FCU: {
                auto ac = std::make_shared<DoublePipeFCU>();
                ac->ac_id = item["id"].get<uint8_t>();
                ac->ac_type = ac_type;
                ac->power_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelPowerUid"].get<uint16_t>());
                ac->low_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelLowUid"].get<uint16_t>());
                ac->mid_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelMidUid"].get<uint16_t>());
                ac->high_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelHighUid"].get<uint16_t>());
                ac->water1_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelWater1Uid"].get<uint16_t>());
                ac->water2_channel = BoardManager::getInstance().getBoardOutput(
                    item["channelWater2Uid"].get<uint16_t>());
                airConManager.addItem(ac->ac_id, ac);
                break;
            }
            case ACType::INFRARED: {
                auto ac = std::make_shared<InfraredAC>();
                ac->ac_id = item["id"].get<uint8_t>();
                ac->ac_type = ac_type;
            }
        }
    }
    
    ESP_LOGI(TAG, "解析 窗帘 配置");
    // ****************** 窗帘 ******************
    for (const auto& item : json_data["窗帘列表"]) {
        auto curtain = std::make_shared<Curtain>();
        curtain->uid = item["uid"].get<uint16_t>();
        curtain->name = item["name"].get<std::string>();
        curtain->channel_open = BoardManager::getInstance().getBoardOutput(
            item["channelOpenUid"].get<uint16_t>());
        curtain->channel_close = BoardManager::getInstance().getBoardOutput(
            item["channelCloseUid"].get<uint16_t>());
        curtain->run_duration = item["runDuration"].get<uint16_t>();
        CurtainManager::getInstance().addItem(curtain->uid, curtain);
    }

    ESP_LOGI(TAG, "解析 485 配置");
    // ****************** 485 ******************
    for (const auto& item : json_data["485指令码列表"]) {
        auto command = std::make_shared<RS485Command>();
        command->uid = item["uid"].get<uint16_t>();
        command->name = item["name"].get<std::string>();
        command->code = parseHexToFixedArray(item["code"].get<std::string>());
        RS485Manager::getInstance().addItem(command->uid, command);
    }

    ESP_LOGI(TAG, "解析 动作组 配置");
    // ****************** 动作组 ******************
    for (const auto& item : json_data["动作组列表"]) {
        auto action_group = std::make_shared<ActionGroup>();
        action_group->uid = item["uid"].get<uint16_t>();
        action_group->name = item["name"].get<std::string>();
        action_group->actions_json = item["actionList"];
        ActionGroupManager::getInstance().addItem(action_group->uid, action_group);
    }
    // 分两遍解析
    for (const auto& [_, action_group] : ActionGroupManager::getInstance().getAllItems()) {
        for (const auto& action_item : action_group->actions_json) {
            Action action;
            action.type = static_cast<ActionType>(action_item["type"].get<int>());
            action.operation = action_item["operation"].get<std::string>();

            if (action.type == ActionType::DELAY) {
                action.target = std::make_shared<DelayAction>();
            } else {
                uint16_t target_uid = action_item["targetUID"].get<uint16_t>();
                action.target = getActionTarget(action.type, target_uid);
            }

            if (action.operation == "调光"
                || action.operation == "延时") {
                action.parameter = action_item["parameter"].get<int>();
            }

            action_group->action_list.push_back(std::move(action));
        }
        action_group->actions_json.clear();
    }

    ESP_LOGI(TAG, "解析 面板 配置");
    // ****************** 面板 ******************
    for (const auto& item : json_data["面板列表"]) {
        auto panel = std::make_shared<Panel>();
        panel->id = item["id"].get<uint8_t>();
        panel->name = item["name"].get<std::string>();

        for (const auto& button_item : item["buttons"]) {
            PanelButton button;
            button.id = button_item["id"].get<uint8_t>();
            button.host_panel = panel;

            for (const auto& action_uid : button_item["actionGroupUids"]) {
                button.action_group_list.push_back(
                    ActionGroupManager::getInstance().getItem(action_uid.get<uint16_t>())
                );
            }
            for (const auto& pressedPolitAction : button_item["pressedPolitActions"]) {
                button.pressed_polit_actions.push_back(pressedPolitAction.get<ButtonPolitAction>());
            }
            for (const auto& pressedOtherPolitAction : button_item["pressedOtherPolitActions"]) {
                button.pressed_other_polit_actions.push_back(pressedOtherPolitAction.get<ButtonOtherPolitAction>());
            }

            panel->buttons[button.id] = button;
        }
        PanelManager::getInstance().addItem(panel->id, panel);
    }


    // ****************** 窗帘 ******************
    
    
    // for (const auto& item : json_data["窗帘列表"]) {
    //     curtainConfig curtain;
    //     curtain.curtain_id = item["ID"].get<uint8_t>();
    //     curtain.open_relay = item["开继电器"].get<uint8_t>();
    //     curtain.close_relay = item["关继电器"].get<uint8_t>();
    //     curtain.duration = item["运行时长"].get<uint8_t>();
    //     config.curtain_global_config.curtain_list.push_back(curtain);
    // }



    // for (const auto& item : json_data["空调模式"]) {

    // }

    // for (const auto& item : json_data["红外配置"]) {
    //     InfraredConfig infrared;
    //     infrared.infrared_id = item["红外ID"].get<uint8_t>();
    //     infrared.enabled = item["启用"].get<uint8_t>();
    //     infrared.circuit_0 = item["回路0"].get<uint8_t>();
    //     infrared.circuit_1 = item["回路1"].get<uint8_t>();
    //     infrared.circuit_2 = item["回路2"].get<uint8_t>();
    //     infrared.relay = item["继电器"].get<uint8_t>();
    //     infrared.on_time = item["开启时长"].get<uint8_t>();
    //     config.infrared_config.push_back(infrared);
    // }

    // return config;
    ESP_LOGI(TAG, "所有配置完成\n");
}

void tcp_server_task(void *pvParameters) {
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE("tcp_server", "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        }
    };
    bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_sock, 1);

    while (1) {
        struct sockaddr_in client_addr;
        uint32_t addr_len = sizeof(client_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE("tcp_server", "Unable to accept connection: errno %d", errno);
            continue;
        }

        ESP_LOGI("tcp_server", "Socket accepted");

        std::string received_data;
        char recv_buf[1024];

        // 循环读取数据，直到接收完成
        while (1) {
            int len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (len < 0) {
                ESP_LOGE("tcp_server", "Recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI("tcp_server", "Connection closed");
                break;
            } else {
                // 将接收到的数据添加到 std::string 对象
                recv_buf[len] = '\0'; // 将缓冲区数据末尾设为 NULL
                received_data += recv_buf; // 追加数据到 std::string
            }
        }

        BoardManager::getInstance().clear();
        LampManager::getInstance().clear();
        AirConManager::getInstance().clear();
        RS485Manager::getInstance().clear();
        ActionGroupManager::getInstance().clear();
        PanelManager::getInstance().clear();
        
        parseJson(received_data);

        // 关闭当前 socket
        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

static void monitor_task(void *pvParameter)
{
	while(1)
	{
        ESP_LOGI(TAG, "%d %d %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size( MALLOC_CAP_DMA ), heap_caps_get_free_size( MALLOC_CAP_SPIRAM ));
        UBaseType_t remaining_stack = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI("total", "Remaining stack size: %u", remaining_stack);
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

extern "C" void app_main(void) {

    // xTaskCreate(monitor_task, "monitor_task", 2304, NULL, 5, NULL);
    xTaskCreate(lvgl_task, "lvgl_task", 8192, NULL, 5, NULL);
    wifi_init_sta();
    xTaskCreate(tcp_server_task, "tcp_server_task", 8192, NULL, 5, NULL);
    
    // init_spiffs();
    // xTaskCreate([] (void *pvParameter) {
    //     parseJson(read_json_to_string("/spiffs/rcu_config.json"));
    //     vTaskDelete(NULL);
    // }, "parseJson", 8192, NULL, 5, NULL);


}
