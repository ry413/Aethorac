#include "commons.h"
#include "esp_log.h"
#include "../rs485/rs485.h"
#define TAG "commons"

static void executeAllAtomicActionTask(void* pvParameter) {
    ActionGroupBase* self = static_cast<ActionGroupBase*>(pvParameter);

    for (const auto& atomic_action : self->atomic_actions) {
        auto target_ptr = atomic_action.target_device.lock();
        if (target_ptr) {
            target_ptr->execute(atomic_action.operation, atomic_action.parameter);
        } else {
            ESP_LOGE(TAG, "target不存在");
        }
    }

    // 任务结束后清空句柄并自我删除
    self->clearTaskHandle();
    vTaskDelete(NULL);
}

void ActionGroupBase::executeAllAtomicAction(void) {
    // 检查是否已有任务在运行
    if (task_handle != NULL) {
        ESP_LOGI(TAG, "动作已在执行中，跳过新任务创建");
        return;
    }

    // 创建新任务
    BaseType_t ret = xTaskCreate(
        executeAllAtomicActionTask,
        "ExecuteAtomicActions",
        4096,      // 根据实际需求调整栈大小
        this,      // 传入当前对象指针
        5,         // 优先级可根据实际需求调整
        &task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建动作执行任务失败");
        task_handle = NULL; // 创建失败，确保清空句柄
    }

}

void ActionGroupBase::clearTaskHandle() {
    task_handle = nullptr;
}
