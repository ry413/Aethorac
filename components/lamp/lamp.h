#ifndef LAMP_H
#define LAMP_H

#include "../commons/commons.h"
#include "../board_config/board_config.h"
#include "../manager_base/manager_base.h"

class BoardOutput;

class Lamp : public IDevice {
public:
    LampType type;
    std::shared_ptr<BoardOutput> output;
    void execute(std::string operation, std::string parameter) override;

    bool isOn() const { return current_state == State::ON; }

    void updateButtonIndicator(bool state);

private:
    enum class State { OFF, ON };
    State current_state = State::OFF;

};
#endif // LAMP_H