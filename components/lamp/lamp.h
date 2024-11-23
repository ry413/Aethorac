#ifndef LAMP_H
#define LAMP_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "config_structs/config_structs.h"
#include "board_config/board_config.h"

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
            printf("灯[%s]调光至%d0%%\n", name.c_str(), std::get<int>(parameter));
        }
    }
};

class LampManager : public ResourceManager<uint16_t, Lamp, LampManager> {
    friend class SingletonManager<LampManager>;
private:
    LampManager() = default;
};

#endif // LAMP_H