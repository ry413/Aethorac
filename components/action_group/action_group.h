#ifndef ACTION_GROUP_H
#define ACTION_GROUP_H

#include <map>
#include <memory>
#include <variant>
#include "../config_structs/config_structs.h"
#include "../lamp/lamp.h"
#include "../air_conditioner/air_conditioner.h"
#include "../rs485/rs485.h"
#include "../rapidjson/document.h"

#define TAG "ACTION_GROUP"

class Action{
public:
    ActionType type;
    std::shared_ptr<IActionTarget> target;
    std::string operation;
    std::variant<int, nullptr_t> parameter;        // 这个暂时只有int, 调光等级和延时秒数

    void execute() {
        if (target) {
            printf("执行中\n");

            target->executeAction(operation, parameter);
        } else {
            ESP_LOGE(TAG, "target不存在");
        }
    }
};

class ActionGroup : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    rapidjson::Document actions_json;
    std::vector<Action> action_list;
    TaskHandle_t task_handle = nullptr;

    // 本动作组被作为目标
    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "调用") {
            // 被'调用'则相当于主动调用本动作组
            execute();
        } else if (operation == "销毁") {
            // 被'销毁'则删除本动作组可能存在的任务
            if (task_handle != nullptr) {
                vTaskDelete(task_handle);
                task_handle = nullptr;
                printf("已销毁动作组任务[%s]\n", name.c_str());
            } else {
                ESP_LOGW(TAG, "动作组[%s]任务不存在, 无法销毁", name.c_str());
            }
        }
    }

    // 动作组主动触发时调用
    void execute() {
        if (task_handle != nullptr) {
            ESP_LOGE(TAG, "动作组[%s] 正在执行中", name.c_str());
            return;
        }
        // 异步依次调用所有动作
        printf("开始执行动作组[%s]\n", name.c_str());
        xTaskCreate([] (void *parameter) {
            auto *group = static_cast<ActionGroup *>(parameter);
            printf("动作组有%d个动作", group->action_list.size());
            for (auto& action : group->action_list) {
                action.execute();
            }
            group->task_handle = nullptr;
            vTaskDelete(NULL);
        }, "ActionGroupTask", 4096, this, 1, &task_handle);
    }

};


class ActionGroupManager : public ResourceManager<uint16_t, ActionGroup, ActionGroupManager> {
    friend class SingletonManager<ActionGroupManager>;
private:
    ActionGroupManager() = default;
};

#endif // ACTION_GROUP_H