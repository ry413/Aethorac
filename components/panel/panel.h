#ifndef PANEL_H
#define PANEL_H

#include <unordered_map>
#include <memory>
#include <vector>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../action_group/action_group.h"
#include "../rs485/rs485.h"

class Panel;
class ActionGroup;

class PanelButton : std::enable_shared_from_this<PanelButton> {
public:
    uint8_t id;
    std::weak_ptr<Panel> host_panel;   // 指向所在的面板

    // 这三个vector长度相同, 一一对应
    std::vector<std::shared_ptr<ActionGroup>> action_group_list;
    std::vector<ButtonPolitAction> pressed_polit_actions;               // 执行完一个动作组后, 本按钮的指示灯的行为
    std::vector<ButtonOtherPolitAction> pressed_other_polit_actions;    // 同时, 所有其他按钮的行为

    // 按下按钮
    void press();

    // 执行指示灯策略
    void execute_polit_actions(uint8_t index);

private:
    uint8_t current_index = 0;      // 此时按下会执行第几个动作
    
};

class Panel {
public:
    uint8_t id;
    std::string name;
    std::unordered_map<uint8_t, std::shared_ptr<PanelButton>> buttons;

    // ******************* 驱动层 *******************
    
    // 设置所有整个面板所有按钮的指示灯
    void set_button_bl_states(uint8_t state) { button_bl_states = state; }
    uint8_t get_button_bl_states(void) const { return button_bl_states; }

    void set_button_operation_flags(uint8_t flags) { button_operation_flags = flags; }
    uint8_t get_button_operation_flags(void) const { return button_operation_flags; }

    // 翻转某一位的指示灯
    void toggle_button_bl_state(int index);

    // 设置指定按钮的指示灯状态
    void set_button_bl_state(uint8_t button_id, bool state);

    // 熄灭除指定按钮外的所有按钮的指示灯
    void turn_off_other_buttons(uint8_t exclude_button_id);

    // 终端函数, 发送指令更新面板状态
    void publish_bl_state(void);

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