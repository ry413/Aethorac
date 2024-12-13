#ifndef commons_H
#define commons_H

extern "C" {
#include "stdio.h"
}

#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "../rapidjson/document.h"
#include "../rapidjson/error/en.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/writer.h"
#include <memory>
#include "../json.hpp"
#include <ctime>
#include <esp_sntp.h>
#include <esp_log.h>


// ****************** 板子 ******************
// 输出类型
enum class OutputType {
    RELAY,
    DRY_CONTACT,
    LIGHT_MODULATOR
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
    HEARTBEAT_STATE,
    DELAYER,
    ACTION_GROUP_MANAGER,
    STATE_SETTER
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


class AssociatedButton {
public:
    uint8_t panel_id;
    uint8_t button_id;

    AssociatedButton(uint8_t panel_id, uint8_t button_id)
        : panel_id(panel_id), button_id(button_id) {}
};

// 所有设备的基类
class IDevice {
public:
    std::string name;
    uint16_t uid;
    std::vector<AssociatedButton> associated_buttons;

    virtual ~IDevice() = default;

    virtual void execute(std::string operation, std::string parameter) = 0;
};


// 最原子级的一条操作, 某种意义上
class AtomicAction {
public:
    std::weak_ptr<IDevice> target_device;       // 本操作的目标设备
    std::string operation;                      // 操作名, 直接交由某个设备处理
    std::string parameter;                      // 有什么是字符串不能表示的呢
};

// 动作组基类
class ActionGroupBase {
public:
    uint16_t uid;
    std::vector<AtomicAction> atomic_actions;

    void executeAllAtomicAction(std::string mode_name);

    void clearTaskHandle();

    void suicide();

    bool require_report = false;        // 表示执行完本动作组后, 是否需要上报状态, 用于情景模式
private:
    TaskHandle_t task_handle = nullptr;
};

class InputBase {
public:
    virtual void execute() = 0;
    std::string mode_name = "";  // 如果有这个的话, 说明此输入是情景模式, 就可以上报给后台

protected:
    uint8_t current_index = 0;  // 此时按下会执行第几个动作组
};


// *************** 时间 ***************
void init_sntp();
time_t get_current_timestamp();

// *************** 上报操作日志 ***************
static std::vector<nlohmann::json> log_array;
static std::mutex log_mutex;
void add_log_entry(const std::string& devicetype, uint16_t deviceid,
                   const std::string& operation, std::string param);
std::vector<nlohmann::json> fetch_and_clear_logs();

// *************** 当前房间的图示状态 ***************
static std::vector<std::string> state_array;
static std::mutex state_mutex;
void add_state(const std::string& state);
bool remove_state(const std::string& state);
void toggle_state(const std::string &state);
std::vector<std::string> get_states();

#endif // commons_H