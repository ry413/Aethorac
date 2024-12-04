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
                break;
            case ButtonPolitAction::LIGHT_OFF:
                panel->set_button_bl_state(id, false);
                break;
            case ButtonPolitAction::LIGHT_SHORT:
                panel->set_button_bl_state(id, true);
                // 设置定时器，1秒后熄灭
                // schedule_light_off(id, 1000);
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
                break;
            case ButtonOtherPolitAction::IGNORE:
                // 不做任何操作
                break;
        }
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
