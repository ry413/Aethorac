#ifndef PANEL_H
#define PANEL_H

#include <unordered_map>
#include <memory>
#include <vector>
#include "../commons/commons.h"
#include "../manager_base/manager_base.h"
#include "../rs485/rs485.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

class Panel;

// 按钮的动作组类, 一个按钮拥有许多动作组, 每次按下时执行一个动作组, 循环的
class PanelButtonActionGroup : public ActionGroupBase{
public:
    ButtonPolitAction pressed_polit_actions;
    ButtonOtherPolitAction pressed_other_polit_actions;
};

// 一个面板的按钮, 它与BoardInput算同级, 或者说同类
class PanelButton : public InputBase {
public:
    uint8_t id;
    std::weak_ptr<Panel> host_panel;        // 指向本按钮所在的面板
    std::vector<std::shared_ptr<PanelButtonActionGroup>> action_groups; // 此按钮所拥有的所有动作组
        
    void execute() override;

    // 执行指示灯策略
    void execute_polit_actions(uint8_t index);
    void schedule_light_off(uint32_t delay_ms);

    ~PanelButton() {
    if (light_off_timer != nullptr) {
        xTimerStop(light_off_timer, 0);
        xTimerDelete(light_off_timer, 0);
        light_off_timer = nullptr;
    }
}

private:
    TimerHandle_t light_off_timer = nullptr; // 定时器句柄
    static void light_off_timer_callback(TimerHandle_t xTimer);
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

    // 熄灭所有指示灯
    void turn_off_all_buttons();

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