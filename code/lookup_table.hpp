#ifndef LOOKUP_TABLE_HPP
#define LOOKUP_TABLE_HPP

#include <vector>
#include <cstdint>
#include <type_traits>
#include <stdexcept>

// 模板参数：
// KeyType: 输入类型（必须是整数类型）
// ValueType: 输出类型
// KEY_BITS: 使用的低位bit数
template <typename KeyType, typename ValueType, int KEY_BITS>
class LookupTable {
    static_assert(std::is_integral<KeyType>::value, 
                  "KeyType must be an integral type");

    static_assert(KEY_BITS > 0 && KEY_BITS <= 64,
                  "KEY_BITS must be in (0, 64]");

public:
    static constexpr uint64_t TABLE_SIZE = 1ULL << KEY_BITS;
    static constexpr uint64_t MASK = TABLE_SIZE - 1;

    LookupTable() : table(TABLE_SIZE) {}

    // 初始化（使用模板函数，避免 std::function 开销）
    template <typename Func>
    void initialize(Func f) {
        for (uint64_t i = 0; i < TABLE_SIZE; ++i) {
            KeyType input = static_cast<KeyType>(i);
            table[i] = f(input);
        }
    }

    // 查询（O(1)）
    const ValueType& query(KeyType input) const {
        uint64_t key = static_cast<uint64_t>(input) & MASK;
        return table[key];
    }

    ValueType& query(KeyType input) {
        uint64_t key = static_cast<uint64_t>(input) & MASK;
        return table[key];
    }

    // 获取表大小
    constexpr uint64_t size() const {
        return TABLE_SIZE;
    }

private:
    std::vector<ValueType> table;
};

#endif // LOOKUP_TABLE_HPP