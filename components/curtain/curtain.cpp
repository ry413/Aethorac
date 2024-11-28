#include "curtain.h"

void Curtain::executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter,
                            PanelButton* source_button) {
    if (operation == "开") {
        this->open_button = source_button;
        handleOpenAction();
    } else if (operation == "关") {
        this->close_button = source_button;
        handleCloseAction();
    }
}


void Curtain::handleOpenAction() {
    if (state == State::OPEN) {
        printf("[%s]已经彻底打开, 不做任何操作\n", name.c_str());
        // 熄灭"窗帘开"按钮的指示灯
        off_button_bl(open_button);
    } else if (state == State::CLOSED || state == State::STOPPED) {
        printf("开始打开[%s]...\n", name.c_str());
        // 不管指示灯
        startAction(channel_open, State::OPENING);
    } else if (state == State::OPENING) {
        printf("停止打开[%s]\n", name.c_str());
        // 熄灭“窗帘开”按钮的指示灯
        stopCurrentAction();
    } else if (state == State::CLOSING) {
        printf("停止关闭, 开始打开[%s]\n", name.c_str());
        // 熄灭“窗帘关”按钮的指示灯
        stopCurrentAction();
        startAction(channel_open, State::OPENING);
    }
}

void Curtain::handleCloseAction() {
    if (state == State::CLOSED) {
        printf("[%s]已经彻底关闭, 不做任何操作\n", name.c_str());
        // 熄灭“窗帘关”按钮的指示灯
        off_button_bl(close_button);
    } else if (state == State::OPEN || state == State::STOPPED) {
        printf("开始关闭[%s]...\n", name.c_str());
        // 不管指示灯
        startAction(channel_close, State::CLOSING);
    } else if (state == State::CLOSING) {
        printf("停止关闭[%s]\n", name.c_str());
        // 熄灭"窗帘关"的指示灯
        stopCurrentAction();
    } else if (state == State::OPENING) {
        printf("停止打开, 开始关闭[%s]\n", name.c_str());
        // 熄灭“窗帘开”按钮的指示灯
        stopCurrentAction();
        startAction(channel_close, State::CLOSING);
    }
}

void Curtain::startAction(std::shared_ptr<BoardOutput> channel, State newState) {
    channel->executeAction("开", nullptr, nullptr);
    state = newState;

    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }

    // 创建新的任务来处理窗帘动作
    xTaskCreate([](void* param) {
        Curtain* self = static_cast<Curtain*>(param);
        // 延时运行时间
        vTaskDelay(self->run_duration * 1000 / portTICK_PERIOD_MS);
        // 调用完成动作
        self->completeAction();
        // 任务结束，自动删除
        vTaskDelete(nullptr);
    }, "CurtainActionTask", 4096, this, tskIDLE_PRIORITY, &actionTaskHandle);
}

void Curtain::stopCurrentAction() { 
    if (state == State::OPENING) {
        channel_open->executeAction("关", nullptr, open_button);
        off_button_bl(open_button);
    } else if (state == State::CLOSING) {
        channel_close->executeAction("关", nullptr, close_button);
        off_button_bl(close_button);
    }
    state = State::STOPPED;
    // 删除正在运行的任务
    if (actionTaskHandle != nullptr) {
        vTaskDelete(actionTaskHandle);
        actionTaskHandle = nullptr;
    }
}

void Curtain::completeAction() {
    if (state == State::OPENING) {
        printf("[%s]已打开\n", name.c_str());
        channel_open->executeAction("关", nullptr, open_button);
        state = State::OPEN;
        // 熄灭“窗帘开”按钮的指示灯
        off_button_bl(open_button);
    } else if (state == State::CLOSING) {
        printf("[%s]已关闭\n", name.c_str());
        channel_close->executeAction("关", nullptr, close_button);
        state = State::CLOSED;
        // 熄灭“窗帘关”按钮的指示灯
        off_button_bl(close_button);
    }

    actionTaskHandle = nullptr;
}

void Curtain::off_button_bl(PanelButton* button) {
    if (button) {
        auto panel = button->host_panel.lock();
        if (panel) {
            panel->set_button_bl_state(button->id, false);
            panel->publish_bl_state();
        }
    }
}