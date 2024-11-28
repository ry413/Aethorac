#ifndef ACTION_GROUP_H
#define ACTION_GROUP_H

#include <map>
#include <memory>
#include <variant>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../lamp/lamp.h"
#include "../air_conditioner/air_conditioner.h"
#include "../rs485/rs485.h"
#include "../rapidjson/document.h"

class Action {
public:
    ActionType type;
    std::weak_ptr<IActionTarget> target;
    std::string operation;
    std::variant<int, nullptr_t> parameter;         // 这个暂时只有int, 调光等级和延时秒数
    PanelButton* source_button = nullptr;           // 触发动作的来源按, 传裸指针得了

    // 执行这个Action
    void execute();
};

class ActionGroup : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    rapidjson::Document actions_json;
    std::vector<Action> action_list;
    TaskHandle_t task_handle = nullptr;

    // 本动作组被作为目标
    void executeAction(const std::string& operation, 
                       const std::variant<int, nullptr_t>& parameter,
                       PanelButton* source_button) override;

    // 动作组主动触发时调用, 比如面板按钮什么的
    void execute(PanelButton* source_button = {});

private:
    PanelButton* source_button = nullptr;   // 触发动作组的按钮来源

};


class ActionGroupManager : public ResourceManager<uint16_t, ActionGroup, ActionGroupManager> {
    friend class SingletonManager<ActionGroupManager>;
private:
    ActionGroupManager() = default;
};

#endif // ACTION_GROUP_H