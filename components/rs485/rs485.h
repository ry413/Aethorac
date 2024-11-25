#ifndef RS485_CONFIG_H
#define RS485_CONFIG_H

#include "../config_structs/config_structs.h"

#define TAG "RS485"

#define RS485_UART_PORT   2
#define RS485_TX_PIN      17
#define RS485_RX_PIN      16
#define RS485_BAUD_RATE   115200
#define RS485_BUFFER_SIZE 1024


void uart_init_rs485();
void sendRS485CMD(const std::vector<uint8_t>& data);


// 将指令码字符串解析成array
std::vector<uint8_t> pavectorseHexToFixedArray(const std::string& hexString);

class RS485Command : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    std::vector<uint8_t> code;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "发送") {
            sendRS485CMD(code);
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