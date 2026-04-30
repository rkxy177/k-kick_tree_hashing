#ifndef LOCAL_QUERY_ROUTER_HPP
#define LOCAL_QUERY_ROUTER_HPP

#include "mini_array.hpp"
#include "lookup_table.hpp"
#include <cstdint>
#include <vector>
#include <cassert>

// 配置参数
static constexpr int MAX_KEYS = 32;         // O(log n / log log n)
static constexpr int BLOCK_BITS = 8;        // 每个块的位宽

// LUT entry
struct RouterOp {
    int next_offset;   // 跳转位偏移
    int value_idx;     // 查找到的值索引
    bool found;
};

// Local Query Router
class LocalQueryRouter {
private:

    // 数据结构（论文对应）

    // E1: trie bitstream（用 MiniArray 表示）
    MiniArray trie_bits;

    // E2: 存储值
    MiniArray values;

    int size_ = 0;

    // LUT：存储对节点操作的查询结果
    static LookupTable<uint16_t, RouterOp, 16> router_lut;

    // hash -> 二进制路径映射
    uint64_t key_to_path(uint64_t key) const {
        return key * 11400714819323198485ULL;
    }

    // 初始化 LUT：预计算节点操作
    static void init_lut() {
        static bool initialized = false;
        if (initialized) return;

        // 初始化 router_lut
        router_lut.initialize([](uint16_t key) {
            RouterOp op{};

            // 提取 key 的 path 和 trie chunk
            uint8_t path = key & 0xFF;
            uint8_t chunk = (key >> 8) & 0xFF;

            // 核心操作逻辑：LUT 模拟论文中的前缀匹配与节点跳转
            int match = 0;

            for (int i = 0; i < 8; i++) {
                if ((chunk >> i) & 1) {
                    if ((path >> i) & 1) {
                        match++;
                    } else {
                        break;
                    }
                }
            }

            // 若匹配成功，返回 f-value 索引；否则计算偏移量继续跳转
            if (match >= 2) {
                op.found = true;
                op.value_idx = match - 1;
                op.next_offset = -1;  // 叶节点已匹配，无需再跳
            } else {
                op.found = false;
                op.next_offset = 2;  // 跳过 2 位进行下一轮匹配（模拟 block traversal）
            }

            return op;
        });

        initialized = true;
    }

public:

    // 构造函数
    LocalQueryRouter() {
        init_lut();
    }

    // 查询函数
    int query(uint64_t key) {
        uint64_t path = key_to_path(key);

        int offset = 0;

        while (offset < 64) {
            // 获取当前 Trie 块数据
            uint64_t chunk = trie_bits.access(0);  // 简化：这里只是单块（扩展可加多块）
            uint8_t trie_chunk = chunk & 0xFF;

            uint8_t path_chunk = (path >> offset) & 0xFF;

            // 生成 LUT 查询键
            uint16_t lut_key = (trie_chunk << 8) | path_chunk;

            // 查询 LUT 获取操作指令
            auto op = router_lut.query(lut_key);

            // 如果找到值，直接返回
            if (op.found) {
                return values.access(op.value_idx);
            }

            // 否则根据偏移量跳转
            offset += op.next_offset;
        }

        return -1;  // 未找到
    }

    // 插入函数
    bool insert(uint64_t key, uint16_t value) {
        // 如果已满，返回 false
        if (size_ >= MAX_KEYS) return false;

        uint64_t path = key_to_path(key);

        // 插入逻辑：先扩展 Trie 结构
        trie_bits.push_back(path & 0xFF, 8);  // 这里只考虑了 8 位块（根据实际可以扩展）

        // 插入值
        values.push_back(value, 16);  // 这里假设每个值占 16 位

        size_++;
        return true;
    }

    // 是否已满
    bool is_full() const {
        return size_ >= MAX_KEYS;
    }

    // 当前大小
    int size() const {
        return size_;
    }
};

// 静态 LUT 初始化
LookupTable<uint16_t, RouterOp, 16> LocalQueryRouter::router_lut;

#endif // LOCAL_QUERY_ROUTER_HPP