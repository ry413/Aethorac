#include "panel.h"


#define TAG "panel"

void PanelButton::press() {
    // 还真要检查, 防止动作组列表是空的
    if (current_index < action_group_list.size()) {
        printf("触发第 %d 个动作组\n", current_index + 1);
        action_group_list[current_index]->execute();


        // 先操作面板内[所有]按钮的指示灯
        switch (pressed_other_polit_actions[current_index]) {
            case ButtonOtherPolitAction::LIGHT_OFF:
                host_panel->change_all_button_polit(false);
                break;
            case ButtonOtherPolitAction::INGORE:
            default:
                break;
        }

        // 然后操作本按钮的指示灯
        switch (pressed_polit_actions[current_index]) {
            case ButtonPolitAction::LIGHT_ON:
                change_polit_lamp(true);
                break;
            case ButtonPolitAction::LIGHT_SHORT:
                change_polit_lamp(true);
                vTaskDelay(1000 / portTICK_PERIOD_MS);  // 直接在这等待
                change_polit_lamp(false);
                break;
            case ButtonPolitAction::LIGHT_OFF:
                change_polit_lamp(false);
                break;
            case ButtonPolitAction::INGORE:
            default:
                break;
        }


        current_index = (current_index + 1) % action_group_list.size();
    }
}

void PanelButton::change_polit_lamp(bool state) {
    polit_lamp_state = state;
    if (state) {
        // printf("点亮面板[%s]的id[%d]按钮的指示灯\n", host_panel->name.c_str(), id);
    } else {
        // printf("熄灭面板[%s]的id[%d]按钮的指示灯\n", host_panel->name.c_str(), id);
    }
}
