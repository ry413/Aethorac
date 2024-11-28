#include "panel.h"

void PanelButton::execute_polit_actions(uint8_t index) {
    auto panel = host_panel.lock();
    if (!panel) {
        ESP_LOGE(TAG, "host_panel 不存在");
    }

    // 处理本按钮的指示灯行为
    if (index < pressed_polit_actions.size()) {
        ButtonPolitAction action = pressed_polit_actions[index];
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
    if (index < pressed_other_polit_actions.size()) {
        ButtonOtherPolitAction action = pressed_other_polit_actions[index];
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
