#include "stdio.h"
#include "board_config.h"
#include "esp_log.h"
extern "C" {
    #include "string.h"
}
#define TAG "BOARD_CONFIG"

// void BoardOutput::executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) {
//     // 这里source_button可能是空
//     uart_frame_t frame;
//     uint8_t cmd_type = 0x01;
//     uint8_t param1 = 0x00;

//     if (operation == "闭合") {
//         param1 = 0x01;
//         ESP_LOGI(TAG, "板%d 通道%d 已闭合", host_board_id, channel);
//     } else if (operation == "断开") {
//         param1 = 0x00;
//         ESP_LOGI(TAG, "板%d 通道%d 已断开", host_board_id, channel);
//     } else if (operation == "调光") {
//         ESP_LOGI(TAG, "板%d 通道%d 调光至 %d", host_board_id, channel, std::get<int>(parameter));
//     } else if (operation == "反转") {
//         ESP_LOGI(TAG, "板%d 通道%d 状态反转", host_board_id, channel);
//     } else {
//         ESP_LOGE(TAG, "未知操作: %s", operation.c_str());
//         return;
//     }
//     // build_frame(cmd_type, host_board_id, channel, param1, 0x00, &frame);
//     // send_frame(&frame);
// }

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
    ESP_LOGI(TAG, "板%d 通道%d 已闭合", host_board_id, channel);
}

void BoardOutput::disconnect() {
    ESP_LOGI(TAG, "板%d 通道%d 已断开", host_board_id, channel);
}
