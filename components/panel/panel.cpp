#include "panel.h"
#include "esp_log.h"

#define TAG "PANEL"

void PanelButton::press() {
    if (current_index < action_groups.size()) {
        action_groups[current_index].executeAllAtomicAction();

        // 执行指示灯策略
        execute_polit_actions(current_index);

        current_index = (current_index + 1) % action_groups.size();
    }
}

void PanelButton::execute_polit_actions(uint8_t index) {
    auto panel = host_panel.lock();
    if (!panel) {
        ESP_LOGE(TAG, "host_panel 不存在");
    }

    // 处理本按钮的指示灯行为
    if (index < action_groups.size()) {
        ButtonPolitAction action = action_groups[index].pressed_polit_actions;
        switch (action) {
            case ButtonPolitAction::LIGHT_ON:
                panel->set_button_bl_state(id, true);
                panel->publish_bl_state();
                break;
            case ButtonPolitAction::LIGHT_OFF:
                panel->set_button_bl_state(id, false);
                panel->publish_bl_state();
                break;
            case ButtonPolitAction::LIGHT_SHORT:
                panel->set_button_bl_state(id, true);
                panel->publish_bl_state();
                // 1秒后熄灭
                schedule_light_off(1000);
                break;
            case ButtonPolitAction::IGNORE:
                // 不做任何操作
                break;
        }
    }
    // 处理其他按钮的指示灯行为
    if (index < action_groups.size()) {
        ButtonOtherPolitAction action = action_groups[index].pressed_other_polit_actions;
        switch (action) {
            case ButtonOtherPolitAction::LIGHT_OFF:
                panel->turn_off_other_buttons(id);
                panel->publish_bl_state();
                break;
            case ButtonOtherPolitAction::IGNORE:
                // 不做任何操作
                break;
        }
    }
}

void PanelButton::schedule_light_off(uint32_t delay_ms) {
    if (light_off_timer == nullptr) {
        // 创建新的定时器
        light_off_timer = xTimerCreate(
            "LightOffTimer",                        // 定时器名称
            pdMS_TO_TICKS(delay_ms),                // 定时周期
            pdFALSE,                                // 不自动重载
            (void*)this,                            // 定时器 ID，传递当前对象指针
            light_off_timer_callback                // 回调函数
        );
        if (light_off_timer == nullptr) {
            ESP_LOGE(TAG, "Failed to create light_off_timer for button %d", id);
            return;
        }
    } else {
        // 如果定时器已存在，先停止定时器
        xTimerStop(light_off_timer, 0);
        // 更新定时器周期
        xTimerChangePeriod(light_off_timer, pdMS_TO_TICKS(delay_ms), 0);
    }
    // 重置并启动定时器
    xTimerReset(light_off_timer, 0);
}

void PanelButton::light_off_timer_callback(TimerHandle_t xTimer) {
    // 获取 PanelButton 对象指针
    PanelButton* button = static_cast<PanelButton*>(pvTimerGetTimerID(xTimer));
    if (button != nullptr) {
        auto panel = button->host_panel.lock();
        if (panel) {
            // 熄灭指示灯
            panel->set_button_bl_state(button->id, false);
            panel->publish_bl_state();
        }
        // 定时器是一次性的，无需删除或重置
    }
}

void Panel::toggle_button_bl_state(int index) {
    uint8_t state = get_button_bl_states();
    state ^= (1 << index);
    set_button_bl_states(state);
}

void Panel::set_button_bl_state(uint8_t button_id, bool state) {
    uint8_t bl_states = get_button_bl_states();
    if (state) {
        bl_states |= (1 << button_id);
    } else {
        bl_states &= ~(1 << button_id);
    }
    set_button_bl_states(bl_states);
}

void Panel::turn_off_other_buttons(uint8_t exclude_button_id) {
    uint8_t bl_states = get_button_bl_states();
    bl_states &= (1 << exclude_button_id);  // 保留指定按钮的状态，其余清零
    set_button_bl_states(bl_states);
}

void Panel::publish_bl_state(void) {               // 第五位(0xFF)传什么都没事, 面板不在乎
    generate_response(CODE_SWITCH_WRITE, 0x00, id, 0xFF, get_button_bl_states());
}

void PanelButtonActionGroup::executeAllAtomicAction(void) {
    for (const auto& atomic_action : atomic_actions) {
        auto target_ptr = atomic_action.target_device.lock();
        if (target_ptr) {
            target_ptr->execute(atomic_action.operation, atomic_action.parameter);
        } else {
            ESP_LOGE(TAG, "target不存在");
        }
    }
}
