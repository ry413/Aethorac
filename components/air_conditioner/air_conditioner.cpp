#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include "esp_log.h"
#include <atomic>

#include "air_conditioner.h"


// ACGlobalConfig ac_global_config;


// SinglePipeFCU::SinglePipeFCU(const ACUnitConfig& config)
//     : ac_config(config) {
//     target_temp.store(ac_global_config.deafult_target_temp);
//     curr_wind_speed = ac_global_config.default_wind_speed;
//     curr_mode = ac_global_config.default_mode;
// }

// SinglePipeFCU::~SinglePipeFCU() {}

// void SinglePipeFCU::onoff_AC(void) {
//     if (is_running.load()) {
//         printf("空调已关闭\n");
//         open_wind_relay(ACWindSpeed::CLOSE_ALL);
//         set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//         is_running.store(false);
//     } else {
//         printf("已打开空调, ID: %d\n", ac_config.ac_id);
//         xTaskCreate(static_task_wrapper, NULL, 4096, this, 3, NULL);
//         is_running.store(true);
//     }
// }

// void SinglePipeFCU::add_temp(void) {
//     uint8_t current_temp = target_temp.load();
//     if (current_temp < 30) {
//         target_temp.store(current_temp + 1);
//         attempt_adjust_temp();
//     }
// }

// void SinglePipeFCU::dec_temp(void) {
//     uint8_t current_temp = target_temp.load();
//     if (current_temp > 16) {
//         target_temp.store(current_temp - 1);
//         attempt_adjust_temp();
//     }
// }

// void SinglePipeFCU::next_wind_speed(void) {
//     curr_wind_speed.store(static_cast<ACWindSpeed>(
//         (static_cast<uint8_t>(curr_wind_speed.load()) + 1) % 4
//     ));
//     adjust_wind_speed();
// }

// void SinglePipeFCU::next_mode(void) {
//     curr_mode.store(static_cast<ACMode>(
//         (static_cast<uint8_t>(curr_mode.load()) + 1) % 3
//     ));

//     adjust_temp();
// }

// void SinglePipeFCU::attempt_adjust_temp() {
//     if (adjust_temp_task_handle != nullptr) {
//         vTaskDelete(adjust_temp_task_handle);
//         adjust_temp_task_handle = nullptr;
//     }

//     xTaskCreate([](void* param) {
//         vTaskDelay(2000 / portTICK_PERIOD_MS);

//         SinglePipeFCU* ac = static_cast<SinglePipeFCU*>(param);
//         ac->adjust_temp();
//         ac->adjust_temp_task_handle = nullptr;
//         vTaskDelete(NULL);
//     }, "AdjustTempTask", 3072, this, 1, &adjust_temp_task_handle);
// }

// void SinglePipeFCU::monitor_task(void) {
//     while (is_running.load()) {
//         adjust_temp();  // 每隔一段时间试图修正继电器状态

//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//     }
//     vTaskDelete(NULL);
// }

// void SinglePipeFCU::static_task_wrapper(void* param) {
//     SinglePipeFCU *ac = static_cast<SinglePipeFCU*>(param);
//     ac->monitor_task();
// }

// void SinglePipeFCU::open_wind_relay(ACWindSpeed ac_wind_speed) {
//     switch (ac_wind_speed) {
//         case ACWindSpeed::LOW:
//             set_relay_state(ac_config.low_speed_relay, true, "低风");
//             set_relay_state(ac_config.mid_speed_relay, false, "中风");
//             set_relay_state(ac_config.high_speed_relay, false, "高风");
//             break;
//         case ACWindSpeed::MEDIUM:
//             set_relay_state(ac_config.low_speed_relay, false, "低风");
//             set_relay_state(ac_config.mid_speed_relay, true, "中风");
//             set_relay_state(ac_config.high_speed_relay, false, "高风");
//             break;
//         case ACWindSpeed::HIGH:
//             set_relay_state(ac_config.low_speed_relay, false, "低风");
//             set_relay_state(ac_config.mid_speed_relay, false, "中风");
//             set_relay_state(ac_config.high_speed_relay, true, "高风");
//             break;
//         case ACWindSpeed::CLOSE_ALL:
//             set_relay_state(ac_config.low_speed_relay, false, "低风");
//             set_relay_state(ac_config.mid_speed_relay, false, "中风");
//             set_relay_state(ac_config.high_speed_relay, false, "高风");
//             break;
//         default:
//             ESP_LOGE(TAG, "哪来的错误?");
//             break;
//     }
// }

// void SinglePipeFCU::adjust_wind_speed(void) {
//     if (curr_wind_speed == ACWindSpeed::AUTO) {
//         // 自动风时在 制冷/热, 则根据温差来决定风速
//         if (curr_mode == ACMode::COOLING || curr_mode == ACMode::HEATING) {
//             int temp_diff = abs(target_temp.load() - get_curr_temp());

//             if (temp_diff <= ac_global_config.low_diff) {               // 温差小, 设为低风
//                 open_wind_relay(ACWindSpeed::LOW);
//             } else if (temp_diff >= ac_global_config.high_diff) {       // 温差大, 设为高风
//                 open_wind_relay(ACWindSpeed::HIGH);
//             } else {                                                    // 否则中风
//                 open_wind_relay(ACWindSpeed::MEDIUM);
//             }
//         }
//         // 自动风时在 通风, 就打开配置中的风机
//         else if (curr_mode == ACMode::FAN) {
//             open_wind_relay(ac_global_config.auto_fun_wind_speed);
//         }
//     }
//     else {
//         // 手动设定风速时，直接根据当前风速调整继电器
//         open_wind_relay(curr_wind_speed);
//     }
// }

// // 根据当前温度与目标温度, 操作继电器
// void SinglePipeFCU::adjust_temp(void) {
//     uint8_t curr_temp_value = get_curr_temp();
//     uint8_t target_temp_value = this->target_temp.load();

//     switch (curr_mode) {
//         case ACMode::COOLING:
//             // 正在制冷时, 直到室温低于目标温度 - 阈值才停止
//             if (is_active.load()) {
//                 if (curr_temp_value <= target_temp_value - ac_global_config.threshold) {
//                     printf("室温[%d] <= (目标温度[%d] - 阈值[%d]), 停止制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     stop_on_reached_target();
//                     is_active.store(false);
//                 } else {
//                     printf("室温[%d] > (目标温度[%d] - 阈值[%d]), 保持制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     adjust_wind_speed();
//                 }
//             }
//             // 当不在制冷时, 直到室温高于目标温度 + 阈值才开始工作
//             else {
//                 if (curr_temp_value >= target_temp_value + ac_global_config.threshold) {
//                     printf("室温[%d] >= (目标温度[%d] - 阈值[%d]), 开始制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     set_relay_state(ac_config.cool_water_valve_relay, true, "冷水阀");
//                     adjust_wind_speed();
//                     is_active.store(true);
//                 } else {
//                     printf("室温[%d] < (目标温度[%d] + 阈值[%d]), 拒绝制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                 }
//             }
//             break;
//         case ACMode::HEATING:
//             // 正在制热时, 直到室温高于目标温度 + 阈值才停止
//             if (is_active.load()) {
//                 if (curr_temp_value >= target_temp_value + ac_global_config.threshold) {
//                     printf("室温[%d] >= (目标温度[%d] + 阈值[%d]), 停止制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     stop_on_reached_target();
//                     is_active.store(false);
//                 } else {
//                     printf("室温[%d] < (目标温度[%d] + 阈值[%d]), 保持制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     adjust_wind_speed();
//                 }
//             }
//             // 当不在制热时, 直到室温低于目标温度 - 阈值才开始工作
//             else {
//                 if (curr_temp_value <= target_temp_value - ac_global_config.threshold) {
//                     printf("室温[%d] <= (目标温度[%d] - 阈值[%d]), 开始制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     set_relay_state(ac_config.cool_water_valve_relay, true, "冷水阀");
//                     adjust_wind_speed();
//                     is_active.store(true);
//                 } else {
//                     printf("室温[%d] > (目标温度[%d] + 阈值[%d]), 拒绝制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                 }
//             }
//             break;
//         // 通风模式则关闭水阀, 只根据风速调整风速继电器
//         case ACMode::FAN:
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             adjust_wind_speed();
//             break;
//     }
// }

// uint8_t SinglePipeFCU::get_curr_temp(void) { 
//     char temp_str[3];
//     lv_roller_get_selected_str(ui_Screen1Roller_curr_true_temp, temp_str, sizeof(temp_str));
//     uint8_t temp = static_cast<uint8_t>(atoi(temp_str));
//     return temp;
// }

// void SinglePipeFCU::stop_on_reached_target(void) {
//     switch (ac_global_config.reached_target_action) {
//         case ACReachedTargetAction::CLOSE_ALL:
//             open_wind_relay(ACWindSpeed::CLOSE_ALL);
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             break;
//         case ACReachedTargetAction::CLOSE_VALVE:
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             break;
//         case ACReachedTargetAction::CLOSE_FAN:
//             open_wind_relay(ACWindSpeed::CLOSE_ALL);
//             break;
//         case ACReachedTargetAction::CLOSE_NONE:
//         default:
//             break;
//     }
// }

// void DoublePipeFCU::onoff_AC(void) {
//     if (is_running.load()) {
//         printf("空调已关闭\n");
//         open_wind_relay(ACWindSpeed::CLOSE_ALL);
//         set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//         set_relay_state(ac_config.heat_water_valve_relay, false, "热水阀");
//         is_running.store(false);
//     } else {
//         printf("已打开空调, ID: %d\n", ac_config.ac_id);
//         xTaskCreate(static_task_wrapper, NULL, 4096, this, 3, NULL);
//         is_running.store(true);
//     }
// }

// void DoublePipeFCU::adjust_temp(void) {
//     uint8_t curr_temp_value = get_curr_temp();
//     uint8_t target_temp_value = this->target_temp.load();

//     switch (curr_mode) {
//         case ACMode::COOLING:
//             // 正在制冷时, 直到室温低于目标温度 - 阈值才停止
//             if (is_active.load()) {
//                 if (curr_temp_value <= target_temp_value - ac_global_config.threshold) {
//                     printf("室温[%d] <= (目标温度[%d] - 阈值[%d]), 停止制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     stop_on_reached_target();
//                     is_active.store(false);
//                 } else {
//                     printf("室温[%d] > (目标温度[%d] - 阈值[%d]), 保持制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     adjust_wind_speed();
//                 }
//             }
//             // 当不在制冷时, 直到室温高于目标温度 + 阈值才开始工作
//             else {
//                 if (curr_temp_value >= target_temp_value + ac_global_config.threshold) {
//                     printf("室温[%d] >= (目标温度[%d] - 阈值[%d]), 开始制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     set_relay_state(ac_config.cool_water_valve_relay, true, "冷水阀");
//                     adjust_wind_speed();
//                     is_active.store(true);
//                 } else {
//                     printf("室温[%d] < (目标温度[%d] + 阈值[%d]), 拒绝制冷\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                 }
//             }
//             break;
//         case ACMode::HEATING:
//             // 正在制热时, 直到室温高于目标温度 + 阈值才停止
//             if (is_active.load()) {
//                 if (curr_temp_value >= target_temp_value + ac_global_config.threshold) {
//                     printf("室温[%d] >= (目标温度[%d] + 阈值[%d]), 停止制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     stop_on_reached_target();
//                     is_active.store(false);
//                 } else {
//                     printf("室温[%d] < (目标温度[%d] + 阈值[%d]), 保持制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     adjust_wind_speed();
//                 }
//             }
//             // 当不在制热时, 直到室温低于目标温度 - 阈值才开始工作
//             else {
//                 if (curr_temp_value <= target_temp_value - ac_global_config.threshold) {
//                     printf("室温[%d] <= (目标温度[%d] - 阈值[%d]), 开始制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                     set_relay_state(ac_config.heat_water_valve_relay, true, "热水阀");
//                     adjust_wind_speed();
//                     is_active.store(true);
//                 } else {
//                     printf("室温[%d] > (目标温度[%d] + 阈值[%d]), 拒绝制热\n", curr_temp_value, target_temp_value, ac_global_config.threshold);
//                 }
//             }
//             break;
//         // 通风模式则关闭水阀, 只根据风速调整风速继电器
//         case ACMode::FAN:
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             set_relay_state(ac_config.heat_water_valve_relay, false, "热水阀");
//             adjust_wind_speed();
//             break;
//     }
// }

// void DoublePipeFCU::stop_on_reached_target(void) {
//     switch (ac_global_config.reached_target_action) {
//         case ACReachedTargetAction::CLOSE_ALL:
//             open_wind_relay(ACWindSpeed::CLOSE_ALL);
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             set_relay_state(ac_config.heat_water_valve_relay, false, "热水阀");
//             break;
//         case ACReachedTargetAction::CLOSE_VALVE:
//             set_relay_state(ac_config.cool_water_valve_relay, false, "冷水阀");
//             set_relay_state(ac_config.heat_water_valve_relay, false, "热水阀");
//             break;
//         case ACReachedTargetAction::CLOSE_FAN:
//             open_wind_relay(ACWindSpeed::CLOSE_ALL);
//             break;
//         case ACReachedTargetAction::CLOSE_NONE:
//         default:
//             break;
//     }
// }

// // 红外空调
// void InfraredAC::onoff_AC(void) {
//     if (is_running.load()) {
//         printf("发送红外指令, 关闭红外空调\n");
//         is_running.store(false);
//     } else {
//         printf("发送红外指令, 打开红外空调\n");
//         adjust_temp();
//         is_running.store(true);
//     }
// }

// uint8_t InfraredAC::get_curr_temp() {
//     char temp_str[3];
//     lv_roller_get_selected_str(ui_Screen1Roller_curr_true_temp, temp_str, sizeof(temp_str));
//     uint8_t temp = static_cast<uint8_t>(atoi(temp_str));
//     printf("当前温度: %d\n", temp);
//     return temp;
// }

// void InfraredAC::adjust_temp(void) {
//     int temp_diff = target_temp.load() - get_curr_temp();
//     ESP_LOGI(TAG, "diff: %d", temp_diff);
//     if (temp_diff != 0) {
//         printf("发送红外指令, 将温度调节到 %d\n", target_temp.load());
//     }
// }


// const char *AC_wind_speed_to_str(ACWindSpeed wind_speed) {
//     switch (wind_speed) {
//         case ACWindSpeed::LOW: 
//             return "低风";
//         case ACWindSpeed::MEDIUM: 
//             return "中风";
//         case ACWindSpeed::HIGH: 
//             return "高风";
//         case ACWindSpeed::AUTO: 
//             return "自动";
//         default:
//             ESP_LOGE(TAG, "错误的风速: %d", static_cast<uint8_t>(wind_speed));
//             return "错误";
//     }
// }

// const char *AC_mode_to_str(ACMode mode) {
//     switch (mode) {
//         case ACMode::COOLING:
//             return "制冷";
//         case ACMode::HEATING: 
//             return "制热";
//         case ACMode::FAN:
//             return "通风";
//         default:
//             ESP_LOGE(TAG, "错误的模式: %d", static_cast<uint8_t>(mode));
//             return "错误";
//     }
// }