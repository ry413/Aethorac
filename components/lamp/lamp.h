#ifndef LAMP_H
#define LAMP_H

#include "../config_structs/config_structs.h"
#include "../board_config/board_config.h"

#define TAG "LAMP"

class Lamp : public IActionTarget {
public:
    uint16_t uid;
    LampType type;
    std::string name;
    std::shared_ptr<BoardOutput> channel_power;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "开") {
            channel_power->executeAction(operation, nullptr);
        } else if (operation == "关") {
            channel_power->executeAction(operation, nullptr);
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
};

class LampManager : public ResourceManager<uint16_t, Lamp, LampManager> {
    friend class SingletonManager<LampManager>;
private:
    LampManager() = default;
};

#endif // LAMP_H