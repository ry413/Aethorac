#ifndef AIR_CONDITIONER_H
#define AIR_CONDITIONER_H

#include <map>
#include <memory>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../board_config/board_config.h"

class BoardOutput;

class AirConBase : public IActionTarget {
public:
    uint8_t ac_id;
    ACType  ac_type;

    void executeAction(const std::string& operation, 
                       const std::variant<int, nullptr_t>& parameter,
                       PanelButton* source_button) override;
};

// 单管空调
class SinglePipeFCU : public AirConBase {
public:
    std::shared_ptr<BoardOutput> power_channel;
    std::shared_ptr<BoardOutput> low_channel;
    std::shared_ptr<BoardOutput> mid_channel;
    std::shared_ptr<BoardOutput> high_channel;
    std::shared_ptr<BoardOutput> water1_channel;
};

// 双管空调
class DoublePipeFCU : public SinglePipeFCU {
public:
    std::shared_ptr<BoardOutput> water2_channel;
};

// 红外空调
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

#endif // AIR_CONDITIONER_H