#ifndef DELAY_ACTION_H
#define DELAY_ACTION_H

#include <map>
#include <memory>
#include "../config_structs/config_structs.h"

#define TAG "DELAY"

// 把延时弄成动作目标类
class DelayAction : public IActionTarget {
public:
    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override{
        int delay_time = std::get<int>(parameter);
        printf("延时 %d 秒...\n", delay_time);
        vTaskDelay(delay_time * 1000 / portTICK_PERIOD_MS);
    }
};

#endif // DELAY_ACTION_H