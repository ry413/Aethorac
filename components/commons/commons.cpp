#include "commons.h"
#include "esp_log.h"
#include "../rs485/rs485.h"
#include "../my_mqtt/my_mqtt.h"
#define TAG "commons"

static void executeAllAtomicActionTask(void* pvParameter) {
    ActionGroupBase* self = static_cast<ActionGroupBase*>(pvParameter);

    for (const auto& atomic_action : self->atomic_actions) {
        auto target_ptr = atomic_action.target_device.lock();
        if (target_ptr) {
            target_ptr->execute(atomic_action.operation, atomic_action.parameter);
        } else {
            ESP_LOGE(TAG, "target不存在");
        }
    }

    // 如果执行完这个动作组需要上报...那就上报
    if (self->require_report) {
        report_states();
        self->require_report = false;
    }

    // 任务结束后清空句柄并自我删除
    self->clearTaskHandle();
    vTaskDelete(NULL);
}

void ActionGroupBase::executeAllAtomicAction(std::string mode_name) {
    // 检查是否已有任务在运行
    if (task_handle != NULL) {
        ESP_LOGW(TAG, "动作已在执行中，跳过新任务创建");
        return;
    }

    std::string current_mode = LordManager::getInstance().getCurrMode();
    // 判断执行完后是否要进入某种模式
    if (!mode_name.empty()) {
        // 判断是否从无模式进入某种模式，或者从一种模式切换到另一种模式
        if (current_mode.empty() || current_mode != mode_name) {
            LordManager::getInstance().setCurrMode(mode_name);
            require_report = true;
        }
    }
    // 如果未传入模式
    else {
        // 判断刚刚是不是模式, 如果是的话, 说明现在要退出某种模式
        if (!current_mode.empty()) {
            LordManager::getInstance().setCurrMode("");
            require_report = true;
        }
    }

    // 创建新任务
    BaseType_t ret = xTaskCreate(
        executeAllAtomicActionTask,
        "ExecuteAtomicActions",
        4096,      // 根据实际需求调整栈大小
        this,      // 传入当前对象指针
        5,         // 优先级可根据实际需求调整
        &task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建动作执行任务失败");
        task_handle = NULL; // 创建失败，确保清空句柄
    }

}

void ActionGroupBase::clearTaskHandle() {
    task_handle = nullptr;
}

void ActionGroupBase::suicide() {
    printf("uid为%d的动作组已自杀\n", uid);
    if (task_handle != nullptr) {
        vTaskDelete(task_handle);
        task_handle = nullptr;
    }
}

void init_sntp() {
    setenv("TZ", "CST-8", 1);
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    printf("初始化SNTP\n");
}

time_t get_current_timestamp() {
    time_t now = time(nullptr);

    if (now < 1609459200) { // 如果时间戳小于 2021-01-01，表示时间未同步, 鬼知道为什么用这个时间, 随便
        ESP_LOGE(__func__, "时间未同步");
        return 0; // 返回0表示无效时间戳
    }

    return now;
}

// *************** 上报操作日志 ***************
void add_log_entry(const std::string& devicetype, uint16_t deviceid, const std::string& operation,
                   std::string param) {
    std::lock_guard<std::mutex> lock(log_mutex); // 确保线程安全
    nlohmann::json entry = {
        {"type", "log"},
        {"devicetype", devicetype},
        {"deviceid", std::to_string(deviceid)},
        {"operation", operation},
        {"param", param},
        {"tm", std::to_string(get_current_timestamp())}
    };
    log_array.push_back(entry);
}

std::vector<nlohmann::json> fetch_and_clear_logs() {
    std::lock_guard<std::mutex> lock(log_mutex); // 确保线程安全
    std::vector<nlohmann::json> current_logs = std::move(log_array); // 移动数据
    log_array.clear();
    return current_logs;
}

// *************** 当前Icon, 就是图示状态, 之类的 ***************

void add_state(const std::string& state) {
    std::lock_guard<std::mutex> lock(state_mutex);  // 加锁保护
    if (std::find(state_array.begin(), state_array.end(), state) == state_array.end()) {
        // 如果图标不存在，则添加
        state_array.push_back(state);
    }
}

bool remove_state(const std::string& state) {
    std::lock_guard<std::mutex> lock(state_mutex);  // 加锁保护
    auto it = std::find(state_array.begin(), state_array.end(), state);
    if (it != state_array.end()) {
        // 如果找到图标，则删除
        state_array.erase(it);
        return true;  // 删除成功
    }
    return false;  // 未找到，删除失败
}

void toggle_state(const std::string &state) {
    std::lock_guard<std::mutex> lock(state_mutex);
    auto it = std::find(state_array.begin(), state_array.end(), state);
    if (it != state_array.end()) {
        state_array.erase(it);  // 如果存在，则删除
    } else {
        state_array.push_back(state);  // 如果不存在，则添加
    }
}

std::vector<std::string> get_states() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return state_array;  // 返回副本
}