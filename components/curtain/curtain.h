#ifndef CURTAIN_H
#define CURTAIN_H

#include <map>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../config_structs/config_structs.h"
#include "../board_config/board_config.h"

class Curtain : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    std::shared_ptr<BoardOutput> channel_open;
    std::shared_ptr<BoardOutput> channel_close;
    uint16_t run_duration;

    Curtain() {}

    ~Curtain() {
        if (actionTaskHandle != nullptr) {
            vTaskDelete(actionTaskHandle);
            actionTaskHandle = nullptr;
        }
    }

    void executeAction(const std::string& operation,
                       const std::variant<int, nullptr_t>& parameter,
                       PanelButton *source_button) override;

private:
    enum class State { CLOSED, OPEN, OPENING, CLOSING, STOPPED };   // 通常使用的"开/关"控制用的状态
    State state = State::CLOSED;

    enum class LastAction { NONE, OPENING, CLOSING };   // 用于, 用一个按键控制窗帘的时候, 即"反转"操作
    LastAction last_action = LastAction::NONE;

    PanelButton* reverse_button = nullptr;              // 三个按钮, "反转"与"开/关"只会存在一种
    PanelButton* open_button = nullptr;
    PanelButton* close_button = nullptr;

    PanelButton* action_button = nullptr;               // 当前动作的按钮（开、关或反转）


    TaskHandle_t actionTaskHandle = nullptr;  // 任务句柄

    // 处理"开"操作
    void handleOpenAction();

    // 处理"关"操作
    void handleCloseAction();

    // 处理"反转"操作
    void handleReverseAction();

    // 打开操作对应的继电器
    void startAction(std::shared_ptr<BoardOutput> channel, State newState, PanelButton* action_button);

    // 停止此时的动作的继电器
    void stopCurrentAction();

    // 成功打开/关闭窗帘, 关闭对应继电器, 并熄灭来源按钮的指示灯
    void completeAction();

    void updateButtonIndicator(PanelButton* button, bool state);
};

class CurtainManager : public ResourceManager<uint16_t, Curtain, CurtainManager> {
    friend class SingletonManager<CurtainManager>;
private:
    CurtainManager() = default;
};
#endif // CURTAIN_H