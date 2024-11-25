#ifndef CONFIG_STRUCTS_H
#define CONFIG_STRUCTS_H

extern "C" {
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// #include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include "esp_log.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <variant>

// struct GeneralConfig {
//     uint8_t hotel_id;
//     uint8_t building_number;
//     uint8_t floor_number;
//     uint8_t room_number;
//     uint8_t days;
//     uint8_t check_it_time;  
// };

// struct IOConfig {
//     uint8_t input_type;
//     uint8_t output_type;
//     uint8_t output_circuit;
// };

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

// ****************** 动作组 ******************
enum class ActionType {
    LAMP,
    AC,
    CURTAIN,
    RS485,
    RELAY,
    ACTION_GROUP,
    DELAY
};

// ****************** 面板 ******************
// 按钮按下时对此按钮指示灯的操作
enum class ButtonPolitAction {
    LIGHT_ON,           // 常亮
    LIGHT_SHORT,        // 短亮(1秒)
    LIGHT_OFF,          // 熄灭
    INGORE,             // 忽略
};
// 对同面板别的按钮的操作
enum class ButtonOtherPolitAction {
    LIGHT_OFF,          // 熄灭
    INGORE,             // 忽略
};
// // 面板上的单个按钮的配置
// struct btnConfig {
//     uint8_t btn_location;               // 仅用于在面板内分别四个按钮
//     uint8_t output_type;
//     uint8_t output_circuit;
// };
// // 单个面板的配置
// struct panelConfig {
//     uint8_t panel_id;
//     btnConfig btns[4];
// };
// // 面板总配置
// struct panelGlobalConfig {
//     // 一些通用配置
//     // 一些通用配置
//     std::vector<panelConfig> panel_list;
// };



// // 窗帘/窗纱的配置
// struct curtainConfig {
//     uint8_t curtain_id;
//     uint8_t open_relay;
//     uint8_t close_relay;
//     uint8_t duration;
// };
// // 窗帘总配置
// struct curtainGlobalConfig {
//     // ...
//     std::vector<curtainConfig> curtain_list;
// };

// struct InfraredConfig {
//     uint8_t infrared_id;
//     uint8_t enabled;
//     uint8_t circuit_0;
//     uint8_t circuit_1;
//     uint8_t circuit_2;
//     uint8_t relay;
//     uint8_t on_time;
// };

// struct RoomConfig {

//     GeneralConfig general_config;
//     std::vector<IOConfig> io_config;
//     ACGlobalConfig ac_global_config;
//     panelGlobalConfig panel_global_config;
//     curtainGlobalConfig curtain_global_config;
//     std::vector<InfraredConfig> infrared_config;
// };


// 基础模板类 SingletonManager，接受派生类作为模板参数
template <typename Derived>
class SingletonManager {
public:
    // 返回派生类的单例实例
    static Derived& getInstance() {
        static Derived instance; // 静态局部变量，保证线程安全
        return instance;
    }

    // 禁用拷贝构造和赋值运算
    SingletonManager(const SingletonManager&) = delete;
    SingletonManager& operator=(const SingletonManager&) = delete;

protected:
    SingletonManager() = default;
    ~SingletonManager() = default;
};

// ResourceManager 模板，提供通用资源管理功能，接受 KeyType、ValueType 和 Derived
template <typename KeyType, typename ValueType, typename Derived>
class ResourceManager : public SingletonManager<Derived> {
public:
    void addItem(const KeyType& id, std::shared_ptr<ValueType> item) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map[id] = item;
    }

    std::shared_ptr<ValueType> getItem(const KeyType& id) const {
        auto it = resource_map.find(id);
        return (it != resource_map.end()) ? it->second : nullptr;
    }

    void removeItem(const KeyType& id) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.erase(id);
    }

    const std::unordered_map<KeyType, std::shared_ptr<ValueType>>& getAllItems() const {
        return resource_map;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.clear();
    }

protected:
    ResourceManager() = default;
    ~ResourceManager() = default;

private:
    mutable std::mutex map_mutex;
    std::unordered_map<KeyType, std::shared_ptr<ValueType>> resource_map;

};

class IActionTarget {
public:
    virtual ~IActionTarget() = default;
    virtual void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) = 0;
};

#endif // CONFIG_STRUCTS_H