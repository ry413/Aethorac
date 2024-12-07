#include "stdio.h"
#include "board_config.h"
#include "../stm32_comm/stm32_comm.h"
#include "esp_log.h"
extern "C" {
    #include "string.h"
}
#include "../rs485/rs485.h"

#define TAG "BOARD_CONFIG"

static void sendRS485CMDByFrame(uart_frame_t frame) {
    std::vector<uint8_t> data = {
        frame.header,
        frame.cmd_type,
        frame.board_id,
        frame.channel,
        frame.param1,
        frame.param2,
        frame.checksum,
        frame.footer
    };

    sendRS485CMD(data);
}

std::shared_ptr<BoardOutput> BoardManager::getBoardOutput(uint16_t uid) {
    for (const auto& [board_id, board] : getAllItems()) {
        auto it = board->outputs.find(uid);
        if (it != board->outputs.end()) {
            return it->second; // 返回共享指针
        }
    }
    return nullptr; // 未找到，返回空指针
}

void BoardOutput::connect() {
    uart_frame_t frame;
    uint8_t cmd_type = 0x01;
    if (type == OutputType::RELAY) {
        cmd_type = 0x01;
    } else if (type == OutputType::DRY_CONTACT) {
        cmd_type = 0x05;
    }
    uint8_t param1 = 0x01;

    build_frame(cmd_type, host_board_id, channel, param1, 0x00, &frame);
    sendRS485CMDByFrame(frame);
    current_state = State::CONNECTED;
    ESP_LOGI(TAG, "板%d 通道%d 已闭合", host_board_id, channel);
}

void BoardOutput::disconnect() {
    uart_frame_t frame;
    uint8_t cmd_type = 0x01;
    if (type == OutputType::RELAY) {
        cmd_type = 0x01;
    } else if (type == OutputType::DRY_CONTACT) {
        cmd_type = 0x05;
    }
    uint8_t param1 = 0x00;

    build_frame(cmd_type, host_board_id, channel, param1, 0x00, &frame);
    sendRS485CMDByFrame(frame);
    current_state = State::DISCONNECTED;
    ESP_LOGI(TAG, "板%d 通道%d 已断开", host_board_id, channel);
}

void BoardOutput::reverse() {
    if (current_state == State::CONNECTED) {
        disconnect();
    } else {
        connect();
    }
}


static void executeAllAtomicActionTask(void *pvParameter) {
    InputActionGroup* self = static_cast<InputActionGroup*>(pvParameter);

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

void InputActionGroup::executeAllAtomicAction(void) {
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

    // for (const auto& atomic_action : atomic_actions) {
    //     auto target_ptr = atomic_action.target_device.lock();
    //     if (target_ptr) {
    //         target_ptr->execute(atomic_action.operation, atomic_action.parameter);
    //     } else {
    //         ESP_LOGE(TAG, "target不存在");
    //     }
    // }
}

void InputActionGroup::clearTaskHandle() {
    task_handle = nullptr;
}

void BoardInput::execute() {
    wakeup_heartbeat();

    if (current_index < action_groups.size()) {
        action_groups[current_index].executeAllAtomicAction();

        current_index = (current_index + 1) % action_groups.size();
    }
}
