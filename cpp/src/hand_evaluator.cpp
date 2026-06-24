#include "poker/hand_evaluator.h"
#include <algorithm>
#include <array>
#include <vector>

namespace poker {

static constexpr int COMBINATIONS[21][5] = {
    {0,1,2,3,4}, {0,1,2,3,5}, {0,1,2,3,6},
    {0,1,2,4,5}, {0,1,2,4,6}, {0,1,2,5,6},
    {0,1,3,4,5}, {0,1,3,4,6}, {0,1,3,5,6},
    {0,1,4,5,6}, {0,2,3,4,5}, {0,2,3,4,6},
    {0,2,3,5,6}, {0,2,4,5,6}, {0,3,4,5,6},
    {1,2,3,4,5}, {1,2,3,4,6}, {1,2,3,5,6},
    {1,2,4,5,6}, {1,3,4,5,6}, {2,3,4,5,6}
};

HandEvaluator::HandEvaluator() {}

// ──────────────────────────────────────────────
//  evaluate_5cards  —  9 种牌型完整判断
// ──────────────────────────────────────────────
//
//  牌型等级（HandRank 枚举）:
//    9 = 同花顺 (STRAIGHT_FLUSH)
//    8 = 四条   (FOUR_KIND)
//    7 = 葫芦   (FULL_HOUSE)
//    6 = 同花   (FLUSH)
//    5 = 顺子   (STRAIGHT)
//    4 = 三条   (THREE_KIND)
//    3 = 两对   (TWO_PAIR)
//    2 = 一对   (ONE_PAIR)
//    1 = 高牌   (HIGH_CARD)
//
//  kickers 数组按优先级从高到低填充，未用位置填 0。
//  比较规则：先比 rank，rank 相同则逐元素比 kickers。
// ──────────────────────────────────────────────

static HandResult evaluate_5cards(const std::array<Card, 5>& cards) {
    HandResult res;
    res.kickers.fill(0);

    // 1. 提取 5 张牌的 rank / suit，rank 降序排列
    std::array<uint8_t, 5> ranks;
    for (int i = 0; i < 5; ++i)
        ranks[i] = cards[i].rank;
    std::sort(ranks.begin(), ranks.end(), std::greater<uint8_t>());

    // 2. 判断同花（5 张 suit 相同）
    bool is_flush = true;
    for (int i = 1; i < 5; ++i) {
        if (cards[i].suit != cards[0].suit) {
            is_flush = false;
            break;
        }
    }

    // 3. 判断顺子（连续 5 张）—— 注意轮子 A-2-3-4-5
    bool is_straight = true;
    for (int i = 0; i < 4; ++i) {
        if (ranks[i] != ranks[i + 1] + 1) {
            is_straight = false;
            break;
        }
    }
    bool is_wheel = false;                 // A-2-3-4-5 (rank: 12,3,2,1,0)
    if (!is_straight &&
        ranks[0] == 12 && ranks[1] == 3 && ranks[2] == 2 &&
        ranks[3] == 1  && ranks[4] == 0) {
        is_straight = true;
        is_wheel    = true;
    }

    // 4. 统计每种 rank 的出现次数，按 (次数↓, rank↓) 排序
    std::array<uint8_t, 13> cnt{};
    for (auto r : ranks) cnt[r]++;

    std::vector<std::pair<uint8_t, uint8_t>> grp;  // (rank, count)
    for (int r = 12; r >= 0; --r) {
        if (cnt[r] > 0)
            grp.emplace_back(static_cast<uint8_t>(r), cnt[r]);
    }
    std::sort(grp.begin(), grp.end(),
        [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first > b.first;
        });

    // 5. 根据牌型填充 rank 与 kickers
    if (is_flush && is_straight) {               // ── 同花顺 ──
        res.rank = static_cast<uint8_t>(HandRank::STRAIGHT_FLUSH);
        res.kickers[0] = is_wheel ? uint8_t{3} : ranks[0];

    } else if (grp[0].second == 4) {              // ── 四条 ──
        res.rank = static_cast<uint8_t>(HandRank::FOUR_KIND);
        res.kickers[0] = grp[0].first;            // 四条 rank
        res.kickers[1] = grp[1].first;            // 踢脚

    } else if (grp[0].second == 3 && grp[1].second == 2) {  // ── 葫芦 ──
        res.rank = static_cast<uint8_t>(HandRank::FULL_HOUSE);
        res.kickers[0] = grp[0].first;            // 三条 rank
        res.kickers[1] = grp[1].first;            // 一对 rank

    } else if (is_flush) {                        // ── 同花 ──
        res.rank = static_cast<uint8_t>(HandRank::FLUSH);
        for (int i = 0; i < 5; ++i)
            res.kickers[i] = ranks[i];

    } else if (is_straight) {                     // ── 顺子 ──
        res.rank = static_cast<uint8_t>(HandRank::STRAIGHT);
        res.kickers[0] = is_wheel ? uint8_t{3} : ranks[0];

    } else if (grp[0].second == 3) {              // ── 三条 ──
        res.rank = static_cast<uint8_t>(HandRank::THREE_KIND);
        res.kickers[0] = grp[0].first;            // 三条 rank
        res.kickers[1] = grp[1].first;            // 高踢脚
        res.kickers[2] = grp[2].first;            // 低踢脚

    } else if (grp[0].second == 2 && grp[1].second == 2) {  // ── 两对 ──
        res.rank = static_cast<uint8_t>(HandRank::TWO_PAIR);
        res.kickers[0] = grp[0].first;            // 高对
        res.kickers[1] = grp[1].first;            // 低对
        res.kickers[2] = grp[2].first;            // 踢脚

    } else if (grp[0].second == 2) {              // ── 一对 ──
        res.rank = static_cast<uint8_t>(HandRank::ONE_PAIR);
        res.kickers[0] = grp[0].first;            // 对子 rank
        res.kickers[1] = grp[1].first;            // 踢脚 1
        res.kickers[2] = grp[2].first;            // 踢脚 2
        res.kickers[3] = grp[3].first;            // 踢脚 3

    } else {                                      // ── 高牌 ──
        res.rank = static_cast<uint8_t>(HandRank::HIGH_CARD);
        for (int i = 0; i < 5; ++i)
            res.kickers[i] = ranks[i];
    }

    return res;
}

HandResult HandEvaluator::evaluate_7cards(const std::array<Card, 7>& cards) const {
    HandResult best;
    best.rank = 0;
    for (int i = 0; i < 21; ++i) {
        std::array<Card, 5> sub;
        for (int j = 0; j < 5; ++j) sub[j] = cards[COMBINATIONS[i][j]];
        auto res = evaluate_5cards(sub);
        if (res.rank > best.rank || (res.rank == best.rank && res > best)) {
            best = res;
        }
    }
    return best;
}

std::vector<HandResult> HandEvaluator::evaluate_batch(
    const std::vector<std::array<Card, 7>>& hands) const {
    std::vector<HandResult> results;
    results.reserve(hands.size());
    for (const auto& h : hands) {
        results.push_back(evaluate_7cards(h));
    }
    return results;
}

bool HandResult::operator>(const HandResult& other) const {
    if (rank != other.rank) return rank > other.rank;
    for (int i = 0; i < 5; ++i) {
        if (kickers[i] != other.kickers[i]) return kickers[i] > other.kickers[i];
    }
    return false;
}

bool HandResult::operator==(const HandResult& other) const {
    if (rank != other.rank) return false;
    return kickers == other.kickers;
}

} // namespace poker
