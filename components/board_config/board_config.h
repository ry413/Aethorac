#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "config_structs/config_structs.h"

#define TAG "BOARD_CONFIG"

// 一个输出通道
class BoardOutput : public IActionTarget{
public:
    uint8_t host_board_id;
    OutputType type;
    uint8_t channel;
    uint16_t uid;

    void executeAction(const std::string& operation, const std::variant<int, nullptr_t>& parameter) override {
        if (operation == "开") {
            ESP_LOGI(TAG, "板%d 通道%d 已闭合", host_board_id, channel);
        } else if (operation == "关") {
            ESP_LOGI(TAG, "板%d 通道%d 已断开", host_board_id, channel);
        }
    };
};

// 一个输入通道
class BoardInput {
public:
    uint8_t host_board_id;
    uint8_t channel;
    InputLevel level;
    uint16_t action_group_uid;
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
    std::shared_ptr<BoardOutput> getBoardOutput(uint16_t uid) {
        for (const auto& [board_id, board] : getAllItems()) {
            auto it = board->outputs.find(uid);
            if (it != board->outputs.end()) {
                return it->second; // 返回共享指针
            }
        }
        return nullptr; // 未找到，返回空指针
    }
};
#endif // BOARD_CONFIG_H