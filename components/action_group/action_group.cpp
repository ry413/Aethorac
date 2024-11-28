#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include "esp_log.h"
#include <atomic>

#include "action_group.h"

#define TAG "ACTION_GROUP"

void Action::execute() {
    auto target_ptr = target.lock();
    if (target_ptr) {
        target_ptr->executeAction(operation, parameter, source_button);
    } else {
        ESP_LOGE(TAG, "target不存在");
    }
}

void ActionGroup::executeAction(const std::string &operation, const std::variant<int, nullptr_t> &parameter,
                                PanelButton* source_button) {
    if (operation == "调用") {
        // 被'调用'则相当于主动调用本动作组
        execute(source_button);
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

void ActionGroup::execute(PanelButton* source_button) {
    if (task_handle != nullptr) {
        ESP_LOGE(TAG, "动作组[%s] 正在执行中", name.c_str());
        return;
    }
    // 异步依次调用所有动作
    printf("开始执行动作组[%s]\n", name.c_str());
    this->source_button = source_button;    // 保存按钮

    xTaskCreate([] (void *parameter) {
        auto *group = static_cast<ActionGroup *>(parameter);
        // printf("动作组有%d个动作", group->action_list.size());
        for (auto& action : group->action_list) {
            action.source_button = group->source_button;    // 传递按钮裸指针
            action.execute();
        }
        group->task_handle = nullptr;
        vTaskDelete(NULL);
    }, "ActionGroupTask", 4096, this, 1, &task_handle);
}
