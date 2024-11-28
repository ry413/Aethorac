#ifndef MANAGER_BASE_H
#define MANAGER_BASE_H

#include <variant>
#include <mutex>
#include <unordered_map>
#include <memory>

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

class IActionTarget {
public:
    virtual ~IActionTarget() = default;
    virtual void executeAction(const std::string& operation,
                               const std::variant<int, nullptr_t>& parameter,
                               PanelButton* source_button) = 0;
};

#endif // MANAGER_BASE_H
