#ifndef CURTAIN_H
#define CURTAIN_H

#include <map>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "../config_structs/config_structs.h"

class Curtain : public IActionTarget {
public:
    uint16_t uid;
    std::string name;
    std::shared_ptr<BoardOutput> channel_open;
    std::shared_ptr<BoardOutput> channel_close;
    uint16_t run_duration;

    Curtain() {
        timer = xTimerCreate("CurtainTimer", pdMS_TO_TICKS(1000), pdFALSE, this, timerCallback);
    }

    ~Curtain() {
        if (timer) {
            xTimerDelete(timer, 0);
        }
    }

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "开") {
            handleOpenAction();
        } else if (operation == "关") {
            handleCloseAction();
        }
    }

private:
    enum class State { CLOSED, OPEN, OPENING, CLOSING, STOPPED };
    State state = State::CLOSED;
    TimerHandle_t timer = nullptr;

    // 定时器超时, 表示窗帘开/关完成
    static void timerCallback(TimerHandle_t xTimer) {
        auto* self = static_cast<Curtain*>(pvTimerGetTimerID(xTimer));
        self->completeAction();
    }

    // 处理"开"操作
    void handleOpenAction() {
        if (state == State::OPEN) {
            printf("[%s]已经彻底打开, 不做任何操作\n", name.c_str());
            return;
        } else if (state == State::CLOSED || state == State::STOPPED) {
            printf("开始打开[%s]...\n", name.c_str());
            startAction(channel_open, State::OPENING);
        } else if (state == State::OPENING) {
            printf("停止打开[%s]\n", name.c_str());
            stopCurrentAction();
        } else if (state == State::CLOSING) {
            printf("停止关闭, 开始打开[%s]\n", name.c_str());
            stopCurrentAction();
            startAction(channel_open, State::OPENING);
        }
    }

    // 处理"关"操作
    void handleCloseAction() {
        if (state == State::CLOSED) {
            printf("[%s]已经彻底关闭, 不做任何操作\n", name.c_str());
            return;
        } else if (state == State::OPEN || state == State::STOPPED) {
            printf("开始关闭[%s]...\n", name.c_str());
            startAction(channel_close, State::CLOSING);
        } else if (state == State::CLOSING) {
            printf("停止关闭[%s]\n", name.c_str());
            stopCurrentAction();
        } else if (state == State::OPENING) {
            printf("停止打开, 开始关闭[%s]\n", name.c_str());
            stopCurrentAction();
            startAction(channel_close, State::CLOSING);
        }
    }

    void startAction(std::shared_ptr<BoardOutput> channel, State newState) {
        channel->executeAction("开", nullptr);
        state = newState;
        xTimerChangePeriod(timer, pdMS_TO_TICKS(run_duration * 1000), 0);
        xTimerStart(timer, 0);
    }

    void stopCurrentAction() {
        if (state == State::OPENING) {
            channel_open->executeAction("关", nullptr);
        } else if (state == State::CLOSING) {
            channel_close->executeAction("关", nullptr);
        }
        state = State::STOPPED;
        xTimerStop(timer, 0);
    }

    void completeAction() {
        if (state == State::OPENING) {
            printf("[%s]已打开\n", name.c_str());
            channel_open->executeAction("关", nullptr);
            state = State::OPEN;
        } else if (state == State::CLOSING) {
            printf("[%s]已关闭\n", name.c_str());
            channel_close->executeAction("关", nullptr);
            state = State::CLOSED;
        }
    }
};

class CurtainManager : public ResourceManager<uint16_t, Curtain, CurtainManager> {
    friend class SingletonManager<CurtainManager>;
private:
    CurtainManager() = default;
};
#endif // CURTAIN_H