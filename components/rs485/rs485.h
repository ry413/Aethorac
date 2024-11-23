#ifndef RS485_CONFIG_H
#define RS485_CONFIG_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <array>
#include "config_structs/config_structs.h"

#define TAG "RS485"

// 将指令码字符串解析成array
std::array<uint8_t, 8> parseHexToFixedArray(const std::string& hexString);

class RS485Command : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    std::array<uint8_t, 8> code;
    
    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "发送") {
            printf("已发送指令码[%s]\n", name.c_str());
        }
    }
};


class RS485Manager : public ResourceManager<uint16_t, RS485Command, RS485Manager> {
    friend class SingletonManager<RS485Manager>;
private:
    RS485Manager() = default;
};
#endif // RS485_CONFIG_H