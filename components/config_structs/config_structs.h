#ifndef CONFIG_STRUCTS_H
#define CONFIG_STRUCTS_H

extern "C" {
#include "stdio.h"
}


// ****************** 板子 ******************
// 输出类型
enum class OutputType {
    RELAY,
    DRY_CONTACT,
};

// 输入电平
enum class InputLevel {
    LOW,
    HIGH,
    LOW_HIGH,
    HIGH_LOW
};

// ****************** 灯 ******************
enum class LampType {
    SWITCH_LIGHT,
    DIMMABLE_LIGHT
};

// ****************** 其他设备 ******************
enum class OtherDeviceType {
    OUTPUT_CONTROL,
    HEARTBEAT_STATE
};

// ****************** 空调 ******************
enum class ACWindSpeed : uint8_t {
    LOW,
    MEDIUM,
    HIGH,
    AUTO,

    // 这个应仅给open_wind_relay()使用
    CLOSE_ALL
};

enum class ACMode: uint8_t {
    COOLING,
    HEATING,
    FAN,
};

enum class ACType: uint8_t {
    SINGLE_PIPE_FCU,
    DOUBLE_PIPE_FCU,
    INFRARED
};

enum class ACStopAction: uint8_t {
    CLOSE_ALL,
    CLOSE_VALVE,
    CLOSE_FAN,
    CLOSE_NONE
};

// ****************** 面板 ******************
// 按钮按下时对此按钮指示灯的操作
enum class ButtonPolitAction {
    LIGHT_ON,           // 常亮
    LIGHT_SHORT,        // 短亮(1秒)
    LIGHT_OFF,          // 熄灭
    IGNORE,             // 忽略
};
// 对同面板别的按钮的操作
enum class ButtonOtherPolitAction {
    LIGHT_OFF,          // 熄灭
    IGNORE,             // 忽略
};

#endif // CONFIG_STRUCTS_H