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
    std::shared_ptr<Panel> host_panel;   // 指向所在的面板

    // 这三个vector里是一对一对一的
    std::vector<std::shared_ptr<ActionGroup>> action_group_list;
    std::vector<ButtonPolitAction> pressed_polit_actions;           // 执行完一个动作组后, 本按钮的指示灯的行为
    std::vector<ButtonOtherPolitAction> pressed_other_polit_actions;      // 同时, 所有其他按钮的行为

    bool polit_lamp_state = false;      // 指示灯状态

    // 按下按钮
    void press();

    void change_polit_lamp(bool state);

private:
    uint8_t current_index = 0;      // 此时按下会执行第几个动作
    
};

class Panel {
public:
    uint8_t id;
    std::string name;
    std::unordered_map<uint8_t, PanelButton> buttons;

    void change_all_button_polit(bool state) {
        for (auto& button : buttons) {
            button.second.change_polit_lamp(state);
        }
    }
};


class PanelManager : public ResourceManager<uint8_t, Panel, PanelManager> {
    friend class SingletonManager<PanelManager>;
private:
    PanelManager() = default;
};

#endif // PANEL_H