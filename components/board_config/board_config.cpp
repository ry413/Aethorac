#include "stdio.h"
#include "board_config.h"
#include "../stm32_comm/stm32_comm.h"
#include "esp_log.h"

#include "../rs485/rs485.h"
#include "../my_mqtt/my_mqtt.h"
#include "../lamp/lamp.h"
#include "../other_device/other_device.h"

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

void BoardInput::execute() {
    // 这里的注释参考PanelButton::execute
    if (is_sleep()) {
        wakeup_heartbeat();

        auto lamps = DeviceManager::getInstance().getDevicesOfType<Lamp>();
        for (auto& lamp : lamps) {
            if (lamp->isOn()) {
                lamp->updateButtonIndicator(true);
            }
        }

        auto others = DeviceManager::getInstance().getDevicesOfType<OtherDevice>();
        for (auto& other : others) {
            if (other->type == OtherDeviceType::OUTPUT_CONTROL) {
                if (other->isOn()) {
                    other->updateButtonIndicator(true);
                }
            }
        }
    }

    if (current_index < action_groups.size()) {
        action_groups[current_index]->executeAllAtomicAction(mode_name);

        current_index = (current_index + 1) % action_groups.size();
    }
}
