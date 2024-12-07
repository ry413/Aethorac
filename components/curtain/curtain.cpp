#include "curtain.h"

void Curtain::execute(std::string operation, int parameter) {
    printf("窗帘[%s]收到操作[%s]\n", name.c_str(), operation.c_str());
    if (operation == "打开") {
        handleOpenAction();
    } else if (operation == "关闭") {
        handleCloseAction();
    } else if (operation == "反转") {
        handleReverseAction();
    }
}

void Curtain::handleOpenAction() {
    action_buttons = open_buttons;

    if (state == State::OPEN) {
        printf("[%s]已经彻底打开, 不做任何操作\n", name.c_str());
        // 熄灭指示灯
        updateButtonIndicator(action_buttons, false);
        return;
    } else if (state == State::CLOSED || state == State::STOPPED) {
        printf("开始打开[%s]...\n", name.c_str());
        startAction(output_open, State::OPENING, action_buttons);
        last_action = LastAction::OPENING;
    } else if (state == State::OPENING) {
        printf("停止打开[%s]\n", name.c_str());
        stopCurrentAction();
    } else if (state == State::CLOSING) {
        printf("停止关闭, 开始打开[%s]\n", name.c_str());
        // 熄灭“窗帘关”按钮的指示灯
        updateButtonIndicator(close_buttons, false);
        stopCurrentAction();
        startAction(output_open, State::OPENING, action_buttons);
        last_action = LastAction::OPENING;
    }
}

void Curtain::handleCloseAction() {
    action_buttons = close_buttons;

    if (state == State::CLOSED) {
        printf("[%s]已经彻底关闭, 不做任何操作\n", name.c_str());
        // 熄灭指示灯
        updateButtonIndicator(action_buttons, false);
        return;
    } else if (state == State::OPEN || state == State::STOPPED) {
        printf("开始关闭[%s]...\n", name.c_str());
        startAction(output_close, State::CLOSING, action_buttons);
        last_action = LastAction::CLOSING;
    } else if (state == State::CLOSING) {
        printf("停止关闭[%s]\n", name.c_str());
        stopCurrentAction();
    } else if (state == State::OPENING) {
        printf("停止打开, 开始关闭[%s]\n", name.c_str());
        // 熄灭“窗帘开”按钮的指示灯
        updateButtonIndicator(open_buttons, false);
        stopCurrentAction();
        startAction(output_close, State::CLOSING, action_buttons);
        last_action = LastAction::CLOSING;
    }
}

void Curtain::handleReverseAction() {
    action_buttons = reverse_buttons;
    if (state == State::CLOSED) {
        // 当前是关闭状态，开始打开
        printf("开始打开[%s]...\n", name.c_str());
        startAction(output_open, State::OPENING, action_buttons);
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
            startAction(output_close, State::CLOSING, action_buttons);
            last_action = LastAction::CLOSING;
        } else {
            // 上一次是关闭或未定义，开始打开
            printf("开始打开[%s]...\n", name.c_str());
            startAction(output_open, State::OPENING, action_buttons);
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
        startAction(output_close, State::CLOSING, action_buttons);
        last_action = LastAction::CLOSING;
    }
}

void Curtain::startAction(std::shared_ptr<BoardOutput> output, State newState, const std::vector<std::weak_ptr<PanelButton>>& action_buttons) {
    output->connect();
    state = newState;

    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }

    this->action_buttons = action_buttons;

    updateButtonIndicator(action_buttons, true);

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
        output_open->disconnect();
    } else if (state == State::CLOSING) {
        output_close->disconnect();
    }
    state = State::STOPPED;

    // 删除正在运行的任务
    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }

    // 熄灭指示灯
    updateButtonIndicator(action_buttons, false);
}

void Curtain::completeAction() {
    if (state == State::OPENING) {
        printf("[%s]已打开\n", name.c_str());
        output_open->disconnect();
        state = State::OPEN;
        // 熄灭指示灯
        updateButtonIndicator(action_buttons, false);
        // 重置 last_action
        last_action = LastAction::NONE;
    } else if (state == State::CLOSING) {
        printf("[%s]已关闭\n", name.c_str());
        output_close->disconnect();
        state = State::CLOSED;
        // 熄灭指示灯
        updateButtonIndicator(action_buttons, false);
        // 重置 last_action
        last_action = LastAction::NONE;
    }

    // 清理任务句柄
    actionTaskHandle = nullptr;
}

void Curtain::updateButtonIndicator(const std::vector<std::weak_ptr<PanelButton>>& buttons, bool state) {
    for (const auto& weak_button : buttons) {
        auto button_ptr = weak_button.lock();
        if (button_ptr) {
            auto panel = button_ptr->host_panel.lock();
            if (panel) {
                panel->set_button_bl_state(button_ptr->id, state);
                panel->publish_bl_state();
            }
        }
    }
}