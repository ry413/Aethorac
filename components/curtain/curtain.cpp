#include "curtain.h"

void Curtain::executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                            PanelButton* source_button) {
    if (operation == "开") {
        this->open_button = source_button;
        handleOpenAction();
    } else if (operation == "关") {
        this->close_button = source_button;
        handleCloseAction();
    } else if (operation == "反转") {
        this->reverse_button = source_button;
        handleReverseAction();
    }
}


void Curtain::handleOpenAction() {
    action_button = open_button;

    if (state == State::OPEN) {
        printf("[%s]已经彻底打开, 不做任何操作\n", name.c_str());
        // 熄灭指示灯
        updateButtonIndicator(action_button, false);
        return;
    } else if (state == State::CLOSED || state == State::STOPPED) {
        printf("开始打开[%s]...\n", name.c_str());
        startAction(channel_open, State::OPENING, action_button);
        last_action = LastAction::OPENING;
    } else if (state == State::OPENING) {
        printf("停止打开[%s]\n", name.c_str());
        stopCurrentAction();
    } else if (state == State::CLOSING) {
        printf("停止关闭, 开始打开[%s]\n", name.c_str());
        // 熄灭“窗帘关”按钮的指示灯
        if (close_button) {
            updateButtonIndicator(close_button, false);
        }
        stopCurrentAction();
        vTaskDelay(100 / portTICK_PERIOD_MS);   // 慢点发
        startAction(channel_open, State::OPENING, action_button);
        last_action = LastAction::OPENING;
    }
}

void Curtain::handleCloseAction() {
    action_button = close_button;

    if (state == State::CLOSED) {
        printf("[%s]已经彻底关闭, 不做任何操作\n", name.c_str());
        // 熄灭指示灯
        updateButtonIndicator(action_button, false);
        return;
    } else if (state == State::OPEN || state == State::STOPPED) {
        printf("开始关闭[%s]...\n", name.c_str());
        startAction(channel_close, State::CLOSING, action_button);
        last_action = LastAction::CLOSING;
    } else if (state == State::CLOSING) {
        printf("停止关闭[%s]\n", name.c_str());
        stopCurrentAction();
    } else if (state == State::OPENING) {
        printf("停止打开, 开始关闭[%s]\n", name.c_str());
        // 熄灭“窗帘开”按钮的指示灯
        if (open_button) {
            updateButtonIndicator(open_button, false);
        }
        stopCurrentAction();
        vTaskDelay(100 / portTICK_PERIOD_MS);   // 慢点发
        startAction(channel_close, State::CLOSING, action_button);
        last_action = LastAction::CLOSING;
    }
}

void Curtain::handleReverseAction() {
    if (state == State::CLOSED) {
        // 当前是关闭状态，开始打开
        printf("开始打开[%s]...\n", name.c_str());
        startAction(channel_open, State::OPENING, reverse_button);
        last_action = LastAction::OPENING;
    } else if (state == State::OPENING) {
        // 正在打开，停止打开
        printf("停止打开[%s]\n", name.c_str());
        stopCurrentAction();
        // 保持 last_action 为 OPENING
    } else if (state == State::STOPPED) {
        // 已停止，根据上一次动作方向决定下一步
        if (last_action == LastAction::OPENING) {
            // 上一次是打开，反转为关闭
            printf("开始关闭[%s]...\n", name.c_str());
            startAction(channel_close, State::CLOSING, reverse_button);
            last_action = LastAction::CLOSING;
        } else {
            // 上一次是关闭或未定义，开始打开
            printf("开始打开[%s]...\n", name.c_str());
            startAction(channel_open, State::OPENING, reverse_button);
            last_action = LastAction::OPENING;
        }
    } else if (state == State::CLOSING) {
        // 正在关闭，停止关闭
        printf("停止关闭[%s]\n", name.c_str());
        stopCurrentAction();
        // 保持 last_action 为 CLOSING
    } else if (state == State::OPEN) {
        // 当前是打开状态，开始关闭
        printf("开始关闭[%s]...\n", name.c_str());
        startAction(channel_close, State::CLOSING, reverse_button);
        last_action = LastAction::CLOSING;
    }
}

void Curtain::startAction(std::shared_ptr<BoardOutput> channel, State newState, PanelButton* action_button) {
    channel->executeAction("开", nullptr, nullptr);
    state = newState;

    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }

    this->action_button = action_button;

    updateButtonIndicator(action_button, true);

    // 创建新的任务来处理窗帘动作
    xTaskCreate([](void* param) {
        Curtain* self = static_cast<Curtain*>(param);
        // 延时运行时间
        vTaskDelay(self->run_duration * 1000 / portTICK_PERIOD_MS);
        // 调用完成动作
        self->completeAction();
        // 任务结束，自动删除
        vTaskDelete(nullptr);
    }, "CurtainActionTask", 4096, this, 5, &actionTaskHandle);
}

void Curtain::stopCurrentAction() { 
    if (state == State::OPENING) {
        channel_open->executeAction("关", nullptr, nullptr);
    } else if (state == State::CLOSING) {
        channel_close->executeAction("关", nullptr, nullptr);
    }
    state = State::STOPPED;

    // 删除正在运行的任务
    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }

    // 熄灭指示灯
    if (action_button) {
        updateButtonIndicator(action_button, false);
    }
}

void Curtain::completeAction() {
    if (state == State::OPENING) {
        printf("[%s]已打开\n", name.c_str());
        channel_open->executeAction("关", nullptr, nullptr);
        state = State::OPEN;
        // 熄灭指示灯
        if (action_button) {
            updateButtonIndicator(action_button, false);
        }
        // 重置 last_action
        last_action = LastAction::NONE;
    } else if (state == State::CLOSING) {
        printf("[%s]已关闭\n", name.c_str());
        channel_close->executeAction("关", nullptr, nullptr);
        state = State::CLOSED;
        // 熄灭指示灯
        if (action_button) {
            updateButtonIndicator(action_button, false);
        }
        // 重置 last_action
        last_action = LastAction::NONE;
    }

    // 清理任务句柄
    actionTaskHandle = nullptr;
}

void Curtain::updateButtonIndicator(PanelButton* button, bool state) {
    if (button) {
        auto panel = button->host_panel.lock();
        if (panel) {
            panel->set_button_bl_state(button->id, state);
            panel->publish_bl_state();
        }
    }
}