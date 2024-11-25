#include "stdio.h"
#include "board_config.h"
#include "esp_log.h"
extern "C" {
    #include "string.h"
}
#define TAG ""

// void init_relay_states(void) {
//     for (int i = 0; i < MAX_RELAYS; i++) {
//         global_relay_states[i].store(false);
//     }
// }

// void set_relay_state(uint8_t relay, bool state, char *name) {
//     if (relay < MAX_RELAYS) {
//         if (global_relay_states[relay].load() != state) {
//             global_relay_states[relay].store(state);
//             printf("继电器 %s 已 %s\n", name, state ? "打开" : "关闭");
//             if (state) {
//                 if (strncmp(name, "低风", 3) == 0) {
//                     lv_obj_add_state(ui_Screen1Checkbox_low_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "中风", 3) == 0) {
//                     lv_obj_add_state(ui_Screen1Checkbox_mid_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "高风", 3) == 0) {
//                     lv_obj_add_state(ui_Screen1Checkbox_high_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "冷水阀", 4) == 0) {
//                     lv_obj_add_state(ui_Screen1Checkbox_cool_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "热水阀", 4) == 0) {
//                     lv_obj_add_state(ui_Screen1Checkbox_heat_cb, LV_STATE_CHECKED);
//                 }
//             } else {
//                 if (strncmp(name, "低风", 3) == 0) {
//                     lv_obj_clear_state(ui_Screen1Checkbox_low_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "中风", 3) == 0) {
//                     lv_obj_clear_state(ui_Screen1Checkbox_mid_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "高风", 3) == 0) {
//                     lv_obj_clear_state(ui_Screen1Checkbox_high_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "冷水阀", 4) == 0) {
//                     lv_obj_clear_state(ui_Screen1Checkbox_cool_cb, LV_STATE_CHECKED);
//                 } else if (strncmp(name, "热水阀", 4) == 0) {
//                     lv_obj_clear_state(ui_Screen1Checkbox_heat_cb, LV_STATE_CHECKED);
//                 }
//             }
//         }
//     } else {
//         ESP_LOGE(TAG, "无效的继电器编号: %d", relay);
//     }
// }
