#include "rs485.h"
#include "driver/uart.h"
#include <string>
#include "panel.h"

#define TAG "RS485"

void uart_init_rs485() {
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(RS485_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(RS485_UART_PORT, RS485_TX_PIN, RS485_RX_PIN, RS485_DE_GPIO_NUM, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(RS485_UART_PORT, RS485_BUFFER_SIZE, RS485_BUFFER_SIZE, 0, NULL, 0));

    ESP_ERROR_CHECK(uart_set_mode(RS485_UART_PORT, UART_MODE_RS485_HALF_DUPLEX));

    ESP_LOGI(TAG, "RS485 UART Initialized");

    xTaskCreate([] (void* param) {
        std::vector<uint8_t> alive_code = pavectorseHexToFixedArray("7FC0FFFF0080BD7E");
        while (true) {
            // sendRS485CMD(alive_code);     不用这个, 否则会一直刷屏
            uart_write_bytes(RS485_UART_PORT, reinterpret_cast<const char*>(alive_code.data()), alive_code.size());
            vTaskDelay(200 / portTICK_PERIOD_MS);
            static int count = 0;
            // count++;
            // if (count > 50) {
            //     UBaseType_t remaining_stack = uxTaskGetStackHighWaterMark(NULL);
            //     ESP_LOGI("sent", "Remaining stack size: %u", remaining_stack);
            //     count = 0;
            // }
        }
    }, "495ALIVE task", 4096, NULL, 3, NULL);

    // 接收任务
    xTaskCreate([](void* param) {
        uint8_t* data = (uint8_t*)malloc(RS485_RX_BUFFER_SIZE); // 动态分配缓冲区
        if (data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate buffer for RS485 reception");
            vTaskDelete(NULL);
        }

        while (true) {
            int length = uart_read_bytes(RS485_UART_PORT, data, RS485_BUFFER_SIZE, 100 / portTICK_PERIOD_MS);
            if (length > 0) {
                ESP_LOGI(TAG, "RS485 接收: %02X %02X %02X %02X %02X %02X %02X %02X", 
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

                handle_rs485_data(data, length);
            }

            static int count = 0;
            // count++;
            // if (count > 50) {
            //     UBaseType_t remaining_stack = uxTaskGetStackHighWaterMark(NULL);
            //     ESP_LOGI("received", "Remaining stack size: %u", remaining_stack);
            //     count = 0;
            // }

        }

        free(data);
        vTaskDelete(NULL);
    }, "485Receive task", 8192, NULL, 3, NULL);
}

void sendRS485CMD(const std::vector<uint8_t>& data) {
    uart_write_bytes(RS485_UART_PORT, reinterpret_cast<const char*>(data.data()), data.size());
    printf("RS485 发送: ");
    for (int i = 0; i < data.size(); ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

std::vector<uint8_t> pavectorseHexToFixedArray(const std::string& hexString) {
    std::vector<uint8_t> result;
    size_t len = hexString.length();

    if (len % 2 != 0) {
        ESP_LOGE(TAG, "十六进制字符串长度必须是偶数");
        return result;
    }

    for (size_t i = 0; i < len; i += 2) {
        std::string byte_str = hexString.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        result.push_back(byte);
    }

    return result;
}

uint8_t calculate_checksum(const std::vector<uint8_t>& data) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < 6; ++i) {
        checksum += data[i];
    }
    return checksum & 0xFF;
}

// 终极处理函数
void handle_rs485_data(uint8_t* data, int length) {
    if (length != 8) {
        ESP_LOGE(TAG, "错误的指令长度: %d", length);
        return;
    }

    if (data[0] != RS485_FRAME_HEADER) {
        ESP_LOGE(TAG, "错误的帧头: %d", data[0]);
        return;
    }

    if (data[7] != RS485_FRAME_FOOTER) {
        ESP_LOGE(TAG, "错误的帧尾: %d", data[7]);
        return;
    }

    uint8_t checksum = calculate_checksum(std::vector<uint8_t>(data, data + 6));
    if (data[6] != checksum) {
        ESP_LOGE(TAG, "校验和错误: %d", checksum);
        return;
    }

    // ******************** 判断功能码 ********************

    // 开关上报
    if (data[1] == CODE_SWITCH_REPORT) {
        uint8_t panel_id = data[3];
        uint8_t target_buttons = data[4];
        uint8_t old_bl_state = data[5];
        std::shared_ptr<Panel> panel = PanelManager::getInstance().getItem(panel_id);
        if (panel == nullptr) {
            ESP_LOGE(TAG, "id为 %d 的面板不存在", panel_id);
            return;
        }

        // 更新指示灯们的状态
        panel->set_button_bl_states(old_bl_state);

        // 如果是0xFF, 说明哪个按钮都没按下, 所以重置所有按钮的标记, 这通常是released时会收到的
        if (target_buttons == 0xFF) {
            panel->set_button_operation_flags(0x00);
            return;
        }

        // 获取当前的按钮操作标记
        uint8_t operation_flags = panel->get_button_operation_flags();

        // 遍历每个按钮, 处理按下与释放
        for (int i = 0; i < 8; ++i) {
            uint8_t mask = 1 << i;
            bool is_pressed = !(target_buttons & mask);     // 当前是否按下
            bool is_operating = operation_flags & mask;     // 是否标记为"正在操作"

            if (is_pressed && !is_operating) {
                // 按钮按下, 且未被标记为"正在操作"
                panel->buttons[i]->press();
                operation_flags |= mask;                    // 设置"正在操作"标记
            } else if (!is_pressed && is_operating) {
                // 按钮释放，且之前被标记为"正在操作"
                operation_flags &= ~mask;    // 清除标记
            }
            // 如果按钮状态未变化, 或已经标记为"正在操作", 则不进行任何操作     // [这一行似乎很没存在感, 但是很重要]
        }

        // 更新按钮操作标记
        panel->set_button_operation_flags(operation_flags);
        
        // 发送指令码更新面板状态   // 现在不在这里操作指示灯了, 都下发给具体设备来操作
        // panel->publish_bl_state();
        
    }
    // else if
}

void generate_response(uint8_t param1, uint8_t param2, uint8_t param3, uint8_t param4, uint8_t param5) {
    std::vector<uint8_t> command = {
        RS485_FRAME_HEADER,
        param1,
        param2,
        param3,
        param4,
        param5,
        0x00,
        RS485_FRAME_FOOTER
    };

    command[6] = calculate_checksum(command);
    sendRS485CMD(command);
}

// 打印二进制
void print_binary(uint8_t value) {
    for (int i = 7; i >= 0; i--) { // 从最高位到最低位
        printf("%c", (value & (1 << i)) ? '1' : '0');
    }
}
// 获得uint8_t从右开始数的唯一0的位置
uint8_t find_zero_position(uint8_t input) {
    // 按位取反, 让唯一的 0 变成 1
    uint8_t inverted = ~input;

    // 使用 __builtin_ffs 查找第一个 1 的位置
    return __builtin_ffs(inverted); // 返回值从 1 开始计数
}