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
    PanelButton* open_button = nullptr;     // 开/关来源按钮
    PanelButton* close_button = nullptr;

    enum class State { CLOSED, OPEN, OPENING, CLOSING, STOPPED };
    State state = State::CLOSED;

    TaskHandle_t actionTaskHandle = nullptr;  // 任务句柄

    // 处理"开"操作
    void handleOpenAction();

    // 处理"关"操作
    void handleCloseAction();

    // 打开操作对应的继电器
    void startAction(std::shared_ptr<BoardOutput> channel, State newState);

    // 停止此时的动作的继电器
    void stopCurrentAction();

    // 成功打开/关闭窗帘, 关闭对应继电器, 并熄灭来源按钮的指示灯
    void completeAction();

    // 熄灭某个按钮的指示灯     // 窗帘类不管点亮指示灯, 只会关闭, 点亮交由Laminor的面板配置管理
    void off_button_bl(PanelButton *button);
};

class CurtainManager : public ResourceManager<uint16_t, Curtain, CurtainManager> {
    friend class SingletonManager<CurtainManager>;
private:
    CurtainManager() = default;
};
#endif // CURTAIN_H