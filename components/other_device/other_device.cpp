#include "../board_config/board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "other_device.h"
#include "../rs485/rs485.h"

void OtherDevice::execute(std::string operation, std::string parameter) {
    printf("[%s]收到操作[%s]\n", name.c_str(), operation.c_str());
    switch (type) {
        case OtherDeviceType::OUTPUT_CONTROL:
            if (operation == "打开") {
                add_log_entry("other", uid, "打开", "");
                output->connect();
                current_state = State::ON;
                updateButtonIndicator(true);
            } else if (operation == "关闭") {
                add_log_entry("other", uid, "关闭", "");
                output->disconnect();
                current_state = State::OFF;
                updateButtonIndicator(false);
            } else if (operation == "反转") {
                output->reverse();

                if (current_state == State::ON) {
                    add_log_entry("other", uid, "关闭", "");
                    current_state = State::OFF;
                    updateButtonIndicator(false);
                } else {
                    add_log_entry("other", uid, "打开", "");
                    current_state = State::ON;
                    updateButtonIndicator(true);
                }
            }
            break;

        case OtherDeviceType::HEARTBEAT_STATE:
            // 如果收到睡眠操作, 改变心跳包
            if (operation == "睡眠") {
                // 还要关闭所有指示灯
                auto all_panel = PanelManager::getInstance().getAllItems();
                for (const auto& panel : all_panel) {
                    panel.second->turn_off_all_buttons();
                    panel.second->publish_bl_state();
                }
                sleep_heartbeat();
            }
            break;
        case OtherDeviceType::DELAYER:
            if (operation == "延时") {
                printf("延时%d秒\n", std::stoi(parameter));
                vTaskDelay(std::stoi(parameter) * 1000 / portTICK_PERIOD_MS);
            }
            break;
        case OtherDeviceType::ACTION_GROUP_MANAGER:
            if (operation == "销毁") {
                auto actionGroup = ActionGroupManager::getInstance().getItem(std::stoi(parameter));
                actionGroup->suicide();
            }
            break;
        case OtherDeviceType::STATE_SETTER:
            if (operation == "设置状态为") {
                add_state(parameter);
            } else if (operation == "清除状态") {
                remove_state(parameter);
            } else if (operation == "反转状态") {
                toggle_state(parameter);
            }
            break;
    }
}

void OtherDevice::updateButtonIndicator(bool state) {
    // 同时开关此灯所有关联的按钮的指示灯
    for (const auto& assoc : associated_buttons) {
        // 根据面板 ID 获取面板
        auto panel = PanelManager::getInstance().getItem(assoc.panel_id);
        if (panel) {
            // 根据按钮 ID 获取按钮
            auto it = panel->buttons.find(assoc.button_id);
            if (it != panel->buttons.end()) {
                auto& button = it->second;
                // ESP_LOGI("Indicator", "Updating indicator for panel %d, button %d to %s",
                //          assoc.panel_id, assoc.button_id, state ? "ON" : "OFF");
                // 设置按钮的背光状态
                panel->set_button_bl_state(button->id, state);
                panel->publish_bl_state();
                // vTaskDelay(50 / portTICK_PERIOD_MS);
            } else {
                // ESP_LOGW("Indicator", "Button ID %d not found in panel %d", assoc.button_id, assoc.panel_id);
            }
        } else {
            // ESP_LOGW("Indicator", "Panel ID %d not found", assoc.panel_id);
        }
    }
}
