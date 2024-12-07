#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <variant>
#include "../config_structs/config_structs.h"
#include "../manager_base/manager_base.h"
#include "../stm32_comm/stm32_comm.h"
#include "../panel/panel.h"

// 一个输出通道
class BoardOutput {
public:
    uint8_t host_board_id;
    OutputType type;
    uint8_t channel;
    uint16_t uid;

    // 闭合与断开电路
    void connect();
    void disconnect();
    void reverse();

private:
    enum class State { CONNECTED, DISCONNECTED };
    State current_state = State::DISCONNECTED;

};

class InputActionGroup {
public:
    std::vector<AtomicAction> atomic_actions;

    void executeAllAtomicAction(void);

    void clearTaskHandle();

private:
    TaskHandle_t task_handle = nullptr;
};

// 一个输入通道, 它与PanelButton算同级, 或者说同类
class BoardInput {
public:
    uint8_t host_board_id;
    uint8_t channel;
    InputLevel level;

    std::vector<InputActionGroup> action_groups;

    // 执行本输入的逻辑
    void execute();

private:
    uint8_t current_index = 0;  // 此时按下会执行第几个动作组
};

// 一整个板子
class BoardConfig {
public:
    uint8_t id;
    std::unordered_map<uint16_t, std::shared_ptr<BoardOutput>> outputs;
    std::vector<BoardInput> inputs; // 没人会需要它的, 所以不用map
};

// 板子管理类
class BoardManager : public ResourceManager<uint16_t, BoardConfig, BoardManager> {
    friend class SingletonManager<BoardManager>;
private:
    BoardManager() = default;

public:
    std::shared_ptr<BoardOutput> getBoardOutput(uint16_t uid);
};
#endif // BOARD_CONFIG_H