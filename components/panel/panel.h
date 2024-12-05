#ifndef PANEL_H
#define PANEL_H

#include <unordered_map>
#include <memory>
#include <vector>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../rs485/rs485.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

class Panel;

// 最原子级的一条操作, 某种意义上
class AtomicAction {
public:
    std::weak_ptr<IDevice> target_device;       // 本操作的目标设备
    std::string operation;                      // 操作名, 直接交由某个设备处理
    int parameter;                              // 有什么是数字不能表示的呢
};

// 按钮的动作组类, 一个按钮拥有许多动作组, 每次按下时执行一个动作组, 循环的
class PanelButtonActionGroup {
public:
    // 一个动作组里有多条原子动作, 在按下按钮时会一并执行
    std::vector<AtomicAction> atomic_actions;
    ButtonPolitAction pressed_polit_actions;
    ButtonOtherPolitAction pressed_other_polit_actions;

    // 执行atomic_actions里所有动作
    void executeAllAtomicAction(void);
};

class PanelButton : std::enable_shared_from_this<PanelButton> {
public:
    uint8_t id;
    std::weak_ptr<Panel> host_panel;        // 指向本按钮所在的面板
    std::vector<PanelButtonActionGroup> action_groups; // 此按钮所拥有的所有动作组

    // 按下按钮
    void press();

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
    uint8_t current_index = 0;      // 此时按下会执行第几个动作
    
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