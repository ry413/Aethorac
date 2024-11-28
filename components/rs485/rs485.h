#ifndef RS485_CONFIG_H
#define RS485_CONFIG_H

#include <string>
#include <vector>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"

#define RS485_UART_PORT   1
#define RS485_TX_PIN      6
#define RS485_RX_PIN      4
#define RS485_DE_GPIO_NUM 5
#define RS485_BAUD_RATE   9600
#define RS485_BUFFER_SIZE 1024 * 2
#define RS485_RX_BUFFER_SIZE 16 * 2

#define RS485_FRAME_HEADER 0x7F
#define RS485_FRAME_FOOTER 0x7E

#define CODE_SWITCH_REPORT  0x00        // 开关上报码
#define CODE_SWITCH_WRITE   0x01        // 开关写入码


void uart_init_rs485();
uint8_t calculate_checksum(const std::vector<uint8_t>& data);
void sendRS485CMD(const std::vector<uint8_t>& data);
void handle_rs485_data(uint8_t* data, int length);
void generate_response(uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4, uint8_t param5);
void print_binary(uint8_t value);
uint8_t find_zero_position(uint8_t input);

// 将指令码字符串解析成array
std::vector<uint8_t> pavectorseHexToFixedArray(const std::string& hexString);

class RS485Command : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    std::vector<uint8_t> code;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                       PanelButton* source_button) override {
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