#ifndef AIR_CONDITIONER_H
#define AIR_CONDITIONER_H

#include <map>
#include <memory>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../board_config/board_config.h"

class BoardOutput;



class AirConBase : public IDevice {
public:
    uint8_t ac_id;
    ACType  ac_type;

    // void executeAction(const std::string& operation, 
    //                    const std::variant<int, nullptr_t>& parameter,
    //                    PanelButton* source_button) override;
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


#endif // AIR_CONDITIONER_H