#include "lamp.h"

void Lamp::executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                         PanelButton* source_button) {
    if (operation == "开") {
        channel_power->executeAction(operation, nullptr, source_button);
    } else if (operation == "关") {
        channel_power->executeAction(operation, nullptr, source_button);
    } else if (operation == "调光") {
        uart_frame_t frame;
        uint8_t cmd_type = 0x03;

        int dimming_value = std::get<int>(parameter); // 获取调光值（1-10）
        uint8_t param1 = dimming_value;
        // build_frame(cmd_type, )  // ?
        
        // 调光灯只有一种操作UNOWEN

        printf("灯[%s]调光至%d0%%\n", name.c_str(), dimming_value);
    }
}