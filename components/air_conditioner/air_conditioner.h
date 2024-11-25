#ifndef AIR_CONDITIONER_H
#define AIR_CONDITIONER_H

#include <map>
#include <memory>
#include "../config_structs/config_structs.h"
#include "../board_config/board_config.h"

#define TAG "AIR_CON"

class AirConBase : public IActionTarget {
public:
    uint8_t ac_id;
    ACType  ac_type;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        
    }
};

// 单管空调
class SinglePipeFCU : public AirConBase {
public:
    // const ACUnitConfig& get_ac_config(void) { return ac_config; }
    // bool get_running(void) { return is_running.load(); }
    // uint8_t get_target_temp(void) { return target_temp.load(); }
    // ACWindSpeed get_wind_speed(void) { return curr_wind_speed.load(); }
    // ACMode get_mode(void) { return curr_mode.load(); }

    // virtual void onoff_AC(void);
    // void add_temp(void);
    // void dec_temp(void);
    // void next_wind_speed(void);
    // void next_mode(void);
    std::shared_ptr<BoardOutput> power_channel;
    std::shared_ptr<BoardOutput> low_channel;
    std::shared_ptr<BoardOutput> mid_channel;
    std::shared_ptr<BoardOutput> high_channel;
    std::shared_ptr<BoardOutput> water1_channel;
    
    // ACUnitConfig ac_config;
    // std::atomic<bool> is_running{false};                    // 空调是否在运行
    // std::atomic<bool> is_active{false};                     // 空调是否在制冷/制热
    // std::atomic<uint8_t> target_temp;                       // 此时设定的目标温度
    // std::atomic<ACWindSpeed> curr_wind_speed{ACWindSpeed::AUTO};        // 当前风速
    // std::atomic<ACMode> curr_mode{ACMode::COOLING};                     // 当前模式
    // TaskHandle_t adjust_temp_task_handle = nullptr;         // 空调运作的实际逻辑任务句柄

    // void monitor_task(void);                                // 持续监控空调与室温状态并进行操作的任务
    // static void static_task_wrapper(void *param);           // 上面这个任务的包装

    // void open_wind_relay(ACWindSpeed ac_wind_speed);  // 打开指定风机
    // void adjust_wind_speed(void);                           // 根据当前所设风速与模式, 操作继电器
    // void attempt_adjust_temp();         // 虚拟系统的防抖实现, 实际上不需要这个, 也不需要很多东西
    // virtual void adjust_temp(void);                         // 根据当前温度与目标温度, 操作继电器
    // virtual uint8_t get_curr_temp(void);                    // 获得当前温度
    // virtual void stop_on_reached_target(void);              // 根据配置, 达到目标温度后做一些操作
};

class DoublePipeFCU : public SinglePipeFCU {
public:
    std::shared_ptr<BoardOutput> water2_channel;
};

class InfraredAC : public AirConBase {

};



class AirConManager : public ResourceManager<uint16_t, AirConBase, AirConManager> {
    friend class SingletonManager<AirConManager>;
private:
    AirConManager() = default;
public:
    uint8_t default_target_temp;        // 默认目标温度
    ACMode default_mode;                // 默认模式
    ACWindSpeed default_wind_speed;     // 默认风速
    uint8_t stopThreshold;              // 超出目标温度后停止工作的阈值
    uint8_t rework_threshold;           // 回温后重新开始工作的阈值
    ACStopAction stop_action;           // 达到目标温度停止工作应该怎么停
    uint8_t low_diff;                   // 风速: 自动时, 进入低风所需小于等于的温差
    uint8_t high_diff;                  // 温差大于等于以进入高风
    ACWindSpeed auto_fun_wind_speed;    // (风速: AUTO, 模式: 通风) 这个状态时, 应该开什么风速
};

// // 双管空调
// class DoublePipeFCU : public SinglePipeFCU {
// public:
//     DoublePipeFCU(const ACUnitConfig& config) : SinglePipeFCU(config) {}
//     void onoff_AC(void) override;

// private:
//     void adjust_temp(void) override;
//     void stop_on_reached_target(void) override;
// };


// // 红外空调
// class InfraredAC : public SinglePipeFCU {
// public:
//     InfraredAC(const ACUnitConfig& config) : SinglePipeFCU(config) {}
//     void onoff_AC(void) override;

// private:
//     uint8_t get_curr_temp() override;
//     void adjust_temp(void) override;
// };



// extern std::map<uint8_t, std::shared_ptr<SinglePipeFCU>> ac_map;
// extern ACGlobalConfig ac_global_config;

// const char *AC_wind_speed_to_str(ACWindSpeed wind_speed);
// const char *AC_mode_to_str(ACMode mode);

#endif // AIR_CONDITIONER_H