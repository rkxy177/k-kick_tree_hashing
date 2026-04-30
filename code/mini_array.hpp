#ifndef MINI_ARRAY_HPP
#define MINI_ARRAY_HPP

#include "lookup_table.hpp"
#include <cstdint>
#include <cassert>

static constexpr int WORD_BITS = 64;
static constexpr int MAX_BITS = 64;

// bitmap LUT: 8-bit block rank
struct Rank8 {
    uint8_t rank[8]; // prefix rank
};

// select LUT: 找第k个1的位置
struct Select8 {
    int pos[8];
};

// =============================
// MiniArray
// =============================
class MiniArray {
private:

    uint64_t data = 0;     // packed bits
    uint64_t bitmap = 0;   // element boundary
    int bit_len = 0;
    int elem_count = 0;

    // LUT
    static LookupTable<uint8_t, Rank8, 8> rank_lut;
    static LookupTable<uint8_t, Select8, 8> select_lut;

    // =========================
    // LUT 初始化
    // =========================
    static void init_lut() {
        static bool initialized = false;
        if (initialized) return;

        // rank LUT
        rank_lut.initialize([](uint8_t x) {
            Rank8 r{};
            int cnt = 0;
            for (int i = 0; i < 8; i++) {
                if (x & (1 << i)) cnt++;
                r.rank[i] = cnt;
            }
            return r;
        });

        // select LUT
        select_lut.initialize([](uint8_t x) {
            Select8 s{};
            int idx = 0;
            for (int i = 0; i < 8; i++) {
                if (x & (1 << i)) {
                    s.pos[idx++] = i;
                }
            }
            for (; idx < 8; idx++) s.pos[idx] = -1;
            return s;
        });

        initialized = true;
    }

    // =========================
    // rank(bit_pos)
    // =========================
    int rank(int pos) const {
        int res = 0;
        for (int i = 0; i < pos; i += 8) {
            uint8_t block = (bitmap >> i) & 0xFF;
            if (i + 8 <= pos) {
                res += rank_lut.query(block).rank[7];
            } else {
                int up = pos - i - 1;
                if (up >= 0)
                    res += rank_lut.query(block).rank[up];
            }
        }
        return res;
    }

    // =========================
    // select(k)
    // =========================
    int select(int k) const {
        int cnt = 0;
        for (int i = 0; i < bit_len; i += 8) {
            uint8_t block = (bitmap >> i) & 0xFF;
            int ones = rank_lut.query(block).rank[7];

            if (cnt + ones > k) {
                int local = k - cnt;
                return i + select_lut.query(block).pos[local];
            }
            cnt += ones;
        }
        return -1;
    }

    // =========================
    // 读取第 i 个元素
    // =========================
    uint64_t get_bits(int l, int r) const {
        int len = r - l + 1;
        uint64_t mask = ((1ULL << len) - 1);
        return (data >> l) & mask;
    }

    // =========================
    // 插入 bits
    // =========================
    void insert_bits(int pos, uint64_t val, int len) {
        uint64_t low = data & ((1ULL << pos) - 1);
        uint64_t high = data >> pos;

        data = low | (val << pos) | (high << (pos + len));

        uint64_t b_low = bitmap & ((1ULL << pos) - 1);
        uint64_t b_high = bitmap >> pos;

        bitmap = b_low | (b_high << (pos + len));

        bit_len += len;
    }

public:

    MiniArray() {
        init_lut();
    }

    // =========================
    // access(i)
    // =========================
    uint64_t access(int i) const {
        assert(i < elem_count);

        int end = select(i);
        int start = (i == 0) ? 0 : select(i - 1) + 1;

        return get_bits(start, end);
    }

    // =========================
    // insert(i, val, len)
    // =========================
    void insert(int i, uint64_t val, int len) {
        assert(bit_len + len <= WORD_BITS);

        int pos = (i == 0) ? 0 : select(i - 1) + 1;

        insert_bits(pos, val, len);

        // 更新 bitmap
        uint64_t low = bitmap & ((1ULL << (pos + len - 1)) - 1);
        uint64_t high = bitmap >> (pos + len - 1);

        bitmap = low | (1ULL << (pos + len - 1)) | (high << 1);

        elem_count++;
    }

    // =========================
    // push_back
    // =========================
    void push_back(uint64_t val, int len) {
        insert(elem_count, val, len);
    }

    // =========================
    // update(i)
    // =========================
    void update(int i, uint64_t val) {
        int end = select(i);
        int start = (i == 0) ? 0 : select(i - 1) + 1;
        int len = end - start + 1;

        uint64_t mask = ((1ULL << len) - 1);

        data &= ~(mask << start);
        data |= (val & mask) << start;
    }

    // =========================
    // delete(i)
    // =========================
    void erase(int i) {
        int end = select(i);
        int start = (i == 0) ? 0 : select(i - 1) + 1;
        int len = end - start + 1;

        uint64_t low = data & ((1ULL << start) - 1);
        uint64_t high = data >> (end + 1);

        data = low | (high << start);

        uint64_t b_low = bitmap & ((1ULL << start) - 1);
        uint64_t b_high = bitmap >> (end + 1);

        bitmap = b_low | (b_high << start);

        bit_len -= len;
        elem_count--;
    }

    int size() const {
        return elem_count;
    }
};

// =============================
// 静态 LUT
// =============================
LookupTable<uint8_t, Rank8, 8> MiniArray::rank_lut;
LookupTable<uint8_t, Select8, 8> MiniArray::select_lut;

#endif