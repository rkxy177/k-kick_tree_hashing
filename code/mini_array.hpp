#pragma once

#include <vector>
#include <cstddef>
#include <optional>

template <typename Element, typename Hasher>
class MiniArray { 
private:
    std::vector<std::optional<Element>> slots_; // 使用 std::optional 来表示槽位是否被占用
    Hasher hasher_;
    size_t size_;
    size_t capacity_;

    // 内部辅助：执行单次 "kick" 并尝试重新插入被踢出的元素这，通常是递归的或迭代的过程
    bool kick_and_reinsert(const Element& kicked_item, size_t current_slot, size_t depth);

public:
    explicit MiniArray(size_t capacity, const Hasher& hasher = Hasher())
        : hasher_(hasher), size_(0), capacity_(capacity) {
        slots.resize(capacity);
    }

    bool insert(const Element& item, size_t preferred_slot, size_t current_depth);

    // 直接访问接口：供 Local Query Router 使用 Router 计算出索引后，直接调用此方法获取数据
    const Element& get(size_t index) const {// 直接访问接口，不允许调用者修改数据
        if (!slots_[index].has_value()) {
            throw std::out_of_range("Accessing empty slot");
        }
        return slots_[index].value();
    }

    Element& get(size_t index) {// 直接访问接口，允许调用者修改数据
        if (!slots_[index].has_value()) {
            throw std::out_of_range("Accessing empty slot");
        }
        return slots_[index].value();
    }

    // 设置槽位内容（供内部 k-kick 移动元素时使用）
    void set(size_type index, const Element& item) {
        if (!slots_[index].has_value()) {
            size_++;
        }
        slots_[index] = item;
    }

    // 清空槽位（供删除或踢出时使用）
    void clear_slot(size_type index) {
        if (slots_[index].has_value()) {
            size_--;
            slots_[index].reset();
        }
    }

    bool is_empty(size_type index) const {// 检查槽位是否为空
        return !slots_[index].has_value();
    }

    size_t size() const { return size_; }
    size_t capacity() const { return slots_.size(); }
    double load_factor() const { return static_cast<double>(size_) / capacity_; }
    
};