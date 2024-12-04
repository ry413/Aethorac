#include "delay_action.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"


// void DelayAction::executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
//                                 PanelButton* source_button) {
//     int delay_time = std::get<int>(parameter);
//     printf("延时 %d 秒...\n", delay_time);
//     vTaskDelay(delay_time * 1000 / portTICK_PERIOD_MS);
// }