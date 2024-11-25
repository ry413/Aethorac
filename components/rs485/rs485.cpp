#include "rs485.h"
#include "driver/uart.h"
#include <string>

void uart_init_rs485() {
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(RS485_UART_PORT, &uart_config);
    uart_set_pin(RS485_UART_PORT, RS485_TX_PIN, RS485_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(RS485_UART_PORT, RS485_BUFFER_SIZE, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "RS485 UART Initialized");
}

void sendRS485CMD(const std::vector<uint8_t>& data) {
    uart_write_bytes(RS485_UART_PORT, reinterpret_cast<const char*>(data.data()), data.size());

    ESP_LOGI(TAG, "RS485 Data Sent:");
    for (size_t i = 0; i < data.size(); ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

std::vector<uint8_t> pavectorseHexToFixedArray(const std::string& hexString) {
    std::vector<uint8_t> result;
    size_t len = hexString.length();

    if (len % 2 != 0) {
        ESP_LOGE(TAG, "十六进制字符串长度必须是偶数");
    }

    for (size_t i = 0; i < len; i++) {
        std::string byte_str = hexString.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        result.push_back(byte);
    }

    return result;
}
