#include "stdio.h"
#include "esp_log.h"
#include "../../ui/ui.h"
#include "rs485.h"
extern "C" {
    #include "string.h"
}
#define TAG "RS485"


std::array<uint8_t, 8> parseHexToFixedArray(const std::string& hexString) {
    if (hexString.length() != 16) {
        ESP_LOGE(TAG, "485指令码长度错误: %d\n", hexString.length());
    }

    std::array<uint8_t, 8> byteArray;
    for (size_t i = 0; i < 8; ++i) {
        std::string byteString = hexString.substr(i * 2, 2);
        byteArray[i] = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    }

    return byteArray;
}