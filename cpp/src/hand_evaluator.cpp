#include "poker/hand_evaluator.h"
#include <algorithm>
#include <array>

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

static HandResult evaluate_5cards(const std::array<Card, 5>& cards) {
    HandResult res;
    res.rank = 1;
    std::array<uint8_t, 5> r = {cards[0].rank, cards[1].rank, cards[2].rank, cards[3].rank, cards[4].rank};
    std::sort(r.begin(), r.end(), std::greater<uint8_t>());
    res.kickers = r;
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
