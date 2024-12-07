#include "stm32_comm.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TAG "UART_COMM"

void uart_init_stm32() {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // 配置串口参数
    uart_param_config(UART_NUM, &uart_config);
    // 设置串口引脚
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 安装驱动程序
    uart_driver_install(UART_NUM, 2048, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART initialized");
}

// 将头六位相加取低8字节作为校验和
uint8_t calculate_checksum(uart_frame_t *frame) {
    uint8_t checksum = 0;
    checksum += frame->header;
    checksum += frame->cmd_type;
    checksum += frame->board_id;
    checksum += frame->channel;
    checksum += frame->param1;
    checksum += frame->param2;

    return checksum & 0xFF;
}

// *********************** 发送 ***********************

// 构造指令帧
void build_frame(uint8_t cmd_type, uint8_t board_id, uint8_t channel, uint8_t param1, uint8_t param2, uart_frame_t *frame) {
    frame->header = STM32_FRAME_HEADER;           // 帧头
    frame->cmd_type = cmd_type;             // 命令类型
    frame->board_id = board_id;             // 板子 ID
    frame->channel = channel;               // 通道号
    frame->param1 = param1;                 // 参数 1
    frame->param2 = param2;                 // 参数 2
    frame->checksum = calculate_checksum(frame); // 计算校验和
    frame->footer = STM32_FRAME_FOOTER;           // 帧尾
}

// 发送指令帧
void send_frame(uart_frame_t *frame) {
    // 将结构体转换为字节数组
    uint8_t *data = (uint8_t *)frame;
    size_t frame_size = sizeof(uart_frame_t);

    // 发送数据
    uart_write_bytes(UART_NUM, (const char *)data, frame_size);

    // 打印发送的数据
    ESP_LOGI(TAG, "发送帧:");
    for (int i = 0; i < frame_size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

// *********************** 接收 ***********************

// 接收任务
void uart_receive_task(void *pvParameters) {
    uint8_t byte;
    enum {
        WAIT_FOR_HEADER,
        RECEIVE_DATA,
    } state = WAIT_FOR_HEADER;

    uart_frame_t frame;
    uint8_t *frame_ptr = (uint8_t *)&frame;
    int byte_index = 0;
    size_t frame_size = sizeof(uart_frame_t);

    while (1) {
        // 读取单个字节
        int len = uart_read_bytes(UART_NUM, &byte, 1, portMAX_DELAY);
        if (len > 0) {
            switch (state) {
                case WAIT_FOR_HEADER:
                    if (byte == STM32_FRAME_HEADER) {
                        state = RECEIVE_DATA;
                        byte_index = 0;
                        frame_ptr[byte_index++] = byte;
                    }
                    break;

                case RECEIVE_DATA:
                    frame_ptr[byte_index++] = byte;
                    if (byte_index == frame_size) {
                        // 接收到完整的数据包
                        if (frame.footer == STM32_FRAME_FOOTER && frame.checksum == calculate_checksum(&frame)) {
                            // 处理数据包
                            handle_response(&frame);
                        } else {
                            ESP_LOGE(TAG, "数据包错误");
                        }
                        state = WAIT_FOR_HEADER; // 无论成功或失败，都重新等待下一帧
                    }
                    break;
            }
        } else if (len < 0) {
            ESP_LOGE(TAG, "UART 读取错误: %d", len);
        }
    }
}

void handle_response(uart_frame_t *frame) {
    switch (frame->cmd_type) {
        case 0x02: // 继电器响应
            ESP_LOGI(TAG, "继电器响应：板%d 通道%d 状态 %s", frame->board_id, frame->channel, frame->param1 ? "开" : "关");
            break;
        case 0x04: // 调光响应
            ESP_LOGI(TAG, "调光响应：板%d 通道%d 当前亮度 %d", frame->board_id, frame->channel, frame->param1);
            break;
        // 根据需要添加其他响应处理
        default:
            ESP_LOGE(TAG, "未知响应命令类型: 0x%02X", frame->cmd_type);
            break;
    }
}