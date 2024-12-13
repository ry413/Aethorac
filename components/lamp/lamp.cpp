#include "lamp.h"
#include "../board_config/board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void Lamp::execute(std::string operation, std::string parameter) {
    printf("灯[%s]收到操作[%s]\n", name.c_str(), operation.c_str());
    if (operation == "打开") {
        add_log_entry("light", uid, operation, parameter);
        output->connect();
        current_state = State::ON;
        updateButtonIndicator(true);
    } else if (operation == "关闭") {
        add_log_entry("light", uid, operation, parameter);
        output->disconnect();
        current_state = State::OFF;
        updateButtonIndicator(false);
    } else if (operation == "反转") {
        if (current_state == State::ON) {
            add_log_entry("light", uid, "关闭", parameter);
            current_state = State::OFF;
            updateButtonIndicator(false);
        } else {
            add_log_entry("light", uid, "打开", parameter);
            current_state = State::ON;
            updateButtonIndicator(true);
        }
        output->reverse();
    }
    // else if (operation == "调光") {
    //     ESP_LOGI(__func__, "调光至 %d0%%", parameter);
    //     if (parameter != 0) {
    //         updateButtonIndicator(true);
    //     } else {
    //         updateButtonIndicator(false);
    //     }
    // }
}

void Lamp::updateButtonIndicator(bool state) {
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
            } else {
                // ESP_LOGW("Indicator", "Button ID %d not found in panel %d", assoc.button_id, assoc.panel_id);
            }
        } else {
            // ESP_LOGW("Indicator", "Panel ID %d not found", assoc.panel_id);
        }
    }
}
