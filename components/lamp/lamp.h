#ifndef LAMP_H
#define LAMP_H

#include "../config_structs/config_structs.h"
#include "../board_config/board_config.h"
#include "../manager_base/manager_base.h"

class BoardOutput;

class Lamp : public IActionTarget {
public:
    uint16_t uid;
    LampType type;
    std::string name;
    std::shared_ptr<BoardOutput> channel_power;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                       PanelButton* source_button) override;
};

class LampManager : public ResourceManager<uint16_t, Lamp, LampManager> {
    friend class SingletonManager<LampManager>;
private:
    LampManager() = default;
};

#endif // LAMP_H