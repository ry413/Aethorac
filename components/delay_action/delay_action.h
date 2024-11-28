#ifndef DELAY_ACTION_H
#define DELAY_ACTION_H

#include "../manager_base/manager_base.h"


// 把延时弄成动作目标类
class DelayAction : public IActionTarget {
public:
    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                       PanelButton* source_button) override;
};

#endif // DELAY_ACTION_H