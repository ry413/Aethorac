#ifndef OTHER_DEVICE_H
#define OTHER_DEVICE_H

#include "../commons/commons.h"
#include "../board_config/board_config.h"
#include "../manager_base/manager_base.h"

class BoardOutput;

// 噢, 我知道这个类简直跟Lamp一模一样

class OtherDevice : public IDevice {
public:
    OtherDeviceType type;
    std::shared_ptr<BoardOutput> output;
    void execute(std::string operation, std::string parameter) override;
    bool isOn() const { return current_state == State::ON; }

    void updateButtonIndicator(bool state);
private:
    enum class State { OFF, ON };
    State current_state = State::OFF;
};
#endif // OTHER_DEVICE_H