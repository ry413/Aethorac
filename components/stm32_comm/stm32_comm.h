#ifndef STM32_COMM_H
#define STM32_COMM_H

#include <stdint.h>

#define UART_NUM UART_NUM_1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_BAUD_RATE 115200

#define FRAME_HEADER 0x55
#define FRAME_FOOTER 0xAA

typedef struct {
    uint8_t header;        // 帧头
    uint8_t cmd_type;      // 命令类型
    uint8_t board_id;      // 板子 ID
    uint8_t channel;       // 通道号
    uint8_t param1;        // 参数 1
    uint8_t param2;        // 参数 2
    uint8_t checksum;      // 校验和
    uint8_t footer;        // 帧尾
} uart_frame_t;

// 串口初始化
void uart_init_stm32();

// 构造指令帧
void build_frame(uint8_t cmd_type, uint8_t board_id, uint8_t channel, uint8_t param1, uint8_t param2, uart_frame_t *frame);

// 发送指令帧
void send_frame(uart_frame_t *frame);

// 串口接收任务
void uart_receive_task(void *pvParameters);

// 处理接收到的字节
void handle_response(uart_frame_t *frame);

#endif // STM32_COMM_H