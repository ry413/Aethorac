#ifndef MANAGER_BASE_H
#define MANAGER_BASE_H

#include <variant>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../commons/commons.h"

// 基础模板类 SingletonManager，接受派生类作为模板参数
template <typename Derived>
class SingletonManager {
public:
    // 返回派生类的单例实例
    static Derived& getInstance() {
        static Derived instance; // 静态局部变量，保证线程安全
        return instance;
    }

    // 禁用拷贝构造和赋值运算
    SingletonManager(const SingletonManager&) = delete;
    SingletonManager& operator=(const SingletonManager&) = delete;

protected:
    SingletonManager() = default;
    ~SingletonManager() = default;
};

// ResourceManager 模板，提供通用资源管理功能，接受 KeyType、ValueType 和 Derived
template <typename KeyType, typename ValueType, typename Derived>
class ResourceManager : public SingletonManager<Derived> {
public:
    void addItem(const KeyType& id, std::shared_ptr<ValueType> item) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map[id] = item;
    }

    std::shared_ptr<ValueType> getItem(const KeyType& id) const {
        auto it = resource_map.find(id);
        return (it != resource_map.end()) ? it->second : nullptr;
    }

    void removeItem(const KeyType& id) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.erase(id);
    }

    const std::unordered_map<KeyType, std::shared_ptr<ValueType>>& getAllItems() const {
        return resource_map;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.clear();
    }

protected:
    ResourceManager() = default;
    ~ResourceManager() = default;

private:
    mutable std::mutex map_mutex;
    std::unordered_map<KeyType, std::shared_ptr<ValueType>> resource_map;

};

class PanelButton;      // 前向声明
class BoardOutput;


// 管理所有设备
class DeviceManager : public SingletonManager<DeviceManager> {
public:
    void addItem(const uint16_t& id, std::shared_ptr<IDevice> item) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map[id] = item;
    }

    std::shared_ptr<IDevice> getItem(const uint16_t& id) const {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = resource_map.find(id);
        return (it != resource_map.end()) ? it->second : nullptr;
    }

    void removeItem(const uint16_t& id) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.erase(id);
    }

    const std::unordered_map<uint16_t, std::shared_ptr<IDevice>>& getAllItems() const {
        std::lock_guard<std::mutex> lock(map_mutex);
        return resource_map;
    }

    // 返回所有特定类型的设备
    template <typename T>
    std::vector<std::shared_ptr<T>> getDevicesOfType() const {
        static_assert(std::is_base_of<IDevice, T>::value, "T must derive from IDevice");

        std::vector<std::shared_ptr<T>> devices;
        std::lock_guard<std::mutex> lock(map_mutex);
        
        for (const auto& [id, device] : resource_map) {
            if (auto casted = std::dynamic_pointer_cast<T>(device)) {
                devices.push_back(casted);
            }
        }
        return devices;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map.clear();
    }

protected:
    DeviceManager() = default;
    ~DeviceManager() = default;

private:
    friend class SingletonManager<DeviceManager>;
    mutable std::mutex map_mutex;
    std::unordered_map<uint16_t, std::shared_ptr<IDevice>> resource_map;
};

class ActionGroupManager : public SingletonManager<ActionGroupManager> {
public:
    void addItem(const uint16_t& id, std::shared_ptr<ActionGroupBase> item) {
        std::lock_guard<std::mutex> lock(map_mutex);
        resource_map[id] = item;
    }

    std::shared_ptr<ActionGroupBase> getItem(const uint16_t& id) const {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = resource_map.find(id);
        return (it != resource_map.end()) ? it->second : nullptr;
    }

protected:
    ActionGroupManager() = default;
    ~ActionGroupManager() = default;

private:
    friend class SingletonManager<ActionGroupManager>;
    mutable std::mutex map_mutex;
    std::unordered_map<uint16_t, std::shared_ptr<ActionGroupBase>> resource_map;
};

#endif // MANAGER_BASE_H
