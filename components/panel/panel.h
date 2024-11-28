#ifndef PANEL_H
#define PANEL_H


#include <unordered_map>
#include <memory>
#include "../config_structs/config_structs.h"
#include "../action_group/action_group.h"

#define TAG "PANEL"

class Panel;

class PanelButton {
public:
    uint8_t id;
    std::weak_ptr<Panel> host_panel;   // 指向所在的面板

    // 这三个vector长度相同, 一一对应
    std::vector<std::shared_ptr<ActionGroup>> action_group_list;
    std::vector<ButtonPolitAction> pressed_polit_actions;               // 执行完一个动作组后, 本按钮的指示灯的行为
    std::vector<ButtonOtherPolitAction> pressed_other_polit_actions;    // 同时, 所有其他按钮的行为

    // 按下按钮
    void press() {
        // 还真要检查, 防止动作组列表是空的
        if (current_index < action_group_list.size()) {
            printf("触发第 %d 个动作组\n", current_index + 1);
            action_group_list[current_index]->execute();

            // 执行指示灯策略
            execute_polit_actions(current_index);

            current_index = (current_index + 1) % action_group_list.size();
        }
    };

    // 执行指示灯策略
    void execute_polit_actions(uint8_t index);

private:
    uint8_t current_index = 0;      // 此时按下会执行第几个动作
    
};

class Panel {
public:
    uint8_t id;
    std::string name;
    std::unordered_map<uint8_t, PanelButton> buttons;

    // ******************* 驱动层 *******************
    void set_button_bl_states(uint8_t state) { button_bl_states = state; }
    uint8_t get_button_bl_states(void) const { return button_bl_states; }

    void set_button_operation_flags(uint8_t flags) { button_operation_flags = flags; }
    uint8_t get_button_operation_flags(void) const { return button_operation_flags; }

    // 翻转某一位的指示灯
    void toggle_button_bl_state(int index) {
        uint8_t state = get_button_bl_states();
        state ^= (1 << index);
        set_button_bl_states(state);
    }

    // 设置指定按钮的指示灯状态
    void set_button_bl_state(uint8_t button_id, bool state) {
        uint8_t bl_states = get_button_bl_states();
        if (state) {
            bl_states |= (1 << button_id);
        } else {
            bl_states &= ~(1 << button_id);
        }
        set_button_bl_states(bl_states);
    }

    // 熄灭除指定按钮外的所有按钮的指示灯
    void turn_off_other_buttons(uint8_t exclude_button_id) {
        uint8_t bl_states = get_button_bl_states();
        bl_states &= (1 << exclude_button_id);  // 保留指定按钮的状态，其余清零
        set_button_bl_states(bl_states);
    }
private:
    // ******************* 驱动层 *******************
    uint8_t button_bl_states = 0x00;        // 所有按钮的背光状态, 1亮0灭
    uint8_t button_operation_flags = 0x00;  // 按钮们的"正在操作"标记

};


class PanelManager : public ResourceManager<uint8_t, Panel, PanelManager> {
    friend class SingletonManager<PanelManager>;
private:
    PanelManager() = default;
};

#endif // PANEL_H