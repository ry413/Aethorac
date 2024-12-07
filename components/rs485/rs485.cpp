#include "rs485.h"
#include "driver/uart.h"
#include <freertos/semphr.h>
#include <string>
#include "panel.h"
#include "esp_log.h"
#include "../board_config/board_config.h"

#define TAG "RS485"

std::vector<uint8_t> heartbeat_code;

// 定义互斥锁
static SemaphoreHandle_t rs485Mutex = nullptr;

// 初始化互斥锁
void initRS485Mutex() {
    rs485Mutex = xSemaphoreCreateMutex();
    if (rs485Mutex == nullptr) {
        printf("Failed to create RS485 mutex\n");
    }
}

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

    initRS485Mutex();
    ESP_LOGI(TAG, "RS485 UART Initialized");

    xTaskCreate([] (void* param) {
        // 初始使用正常的存活心跳
        wakeup_heartbeat();

        while (true) {
            // sendRS485CMD(heartbeat_code);   //     不用这个, 否则会一直刷屏
            
            if (xSemaphoreTake(rs485Mutex, portMAX_DELAY) == pdTRUE) {
                uart_write_bytes(RS485_UART_PORT, reinterpret_cast<const char*>(heartbeat_code.data()), heartbeat_code.size());
                vTaskDelay(100 / portTICK_PERIOD_MS);
                xSemaphoreGive(rs485Mutex);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            
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
        uint8_t byte;
        int frame_size = 8;
        enum {
            WAIT_FOR_HEADER,
            RECEIVE_DATA,
        } state = WAIT_FOR_HEADER;

        uint8_t buffer[frame_size];
        int byte_index = 0;

        while (1) {
            // 读取单个字节
            int len = uart_read_bytes(RS485_UART_PORT, &byte, 1, portMAX_DELAY);
            if (len > 0) {
                switch (state) {
                    case WAIT_FOR_HEADER:
                        if (byte == RS485_FRAME_HEADER || byte == STM32_FRAME_HEADER) {
                            state = RECEIVE_DATA;
                            byte_index = 0;
                            buffer[byte_index++] = byte;
                        } else {
                            ESP_LOGE(TAG, "错误帧头: 0x%02x", byte);
                        }
                        break;

                    case RECEIVE_DATA:
                        buffer[byte_index++] = byte;
                        if (byte_index == frame_size) {
                            if ((buffer[0] == RS485_FRAME_HEADER && buffer[frame_size - 1] == RS485_FRAME_FOOTER) ||
                                (buffer[0] == STM32_FRAME_HEADER && buffer[frame_size - 1] == STM32_FRAME_FOOTER))
                            // 接收到完整的数据包, 判断在里面做
                            handle_rs485_data(buffer, frame_size);
                            state = WAIT_FOR_HEADER; // 无论成功或失败，都重新等待下一帧
                        }
                        break;
                }
            } else if (len < 0) {
                ESP_LOGE(TAG, "UART 读取错误: %d", len);
            }
        }
        vTaskDelete(NULL);
    }, "485Receive task", 8192, NULL, 7, NULL);
}

void sendRS485CMD(const std::vector<uint8_t>& data) {
    if (xSemaphoreTake(rs485Mutex, portMAX_DELAY) == pdTRUE) {
        uart_write_bytes(RS485_UART_PORT, reinterpret_cast<const char*>(data.data()), data.size());
        printf("RS485 发送: ");
        for (size_t i = 0; i < data.size(); ++i) {
            printf("%02X ", data[i]);
        }
        printf("\n");

        // 100ms是很好的数值了, 80就不行了
        vTaskDelay(100 / portTICK_PERIOD_MS);
        xSemaphoreGive(rs485Mutex);
    } else {
        printf("Failed to acquire RS485 mutex\n");
    }
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

void wakeup_heartbeat() {
    heartbeat_code = pavectorseHexToFixedArray("7FC0FFFF0080BD7E");
}

void sleep_heartbeat() {
    ESP_LOGI(TAG, "进入睡眠");
    heartbeat_code = pavectorseHexToFixedArray("7FC0FFFF00003D7E");
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
                panel->buttons[i]->execute();
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
    // 临时的stm32. 干接点输入
    else if (data[1] == 0x07) {
        uint8_t board_id = data[2];
        uint8_t channel_num = data[3];
        uint8_t state = data[4];    // 
        // printf("干接点输入 0x%02x, 0x%02x, 0x%02x\n", board_id, channel_num, state);

        InputLevel input_level;
        // 通道闭合
        if (state == 0x01) {
            input_level = InputLevel::HIGH;
        }
        // 通道断开
        else {
            input_level = InputLevel::LOW;
        }

        // 遍历所有指定通道的BoardInput, 也就是说, 有可能有多个channel不同的BoardInput, 这里要执行所有同为指定通道的输出
        auto& all_boards = BoardManager::getInstance().getAllItems();
        for (const auto& [board_id, board] : all_boards) {
            auto& inputs = board->inputs;
            for (auto& input : inputs) {
                if (input.channel == channel_num) {
                    // 不过只执行指定电平的配置
                    if (input.level == input_level) {
                        input.execute();
                    }
                }
            }
        }    
    }
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

void RS485Command::execute(std::string operation, int parameter) {
    printf("发送485指令[%s]\n", name.c_str());
    if (operation == "发送") {
        sendRS485CMD(code);
    }
}
