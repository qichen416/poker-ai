#pragma once
#include "poker/card.h"
#include <array>
#include <cstdint>

namespace poker {

enum class HandRank : uint8_t {
    HIGH_CARD = 1, ONE_PAIR = 2, TWO_PAIR = 3,
    THREE_KIND = 4, STRAIGHT = 5, FLUSH = 6,
    FULL_HOUSE = 7, FOUR_KIND = 8, STRAIGHT_FLUSH = 9
};

struct HandResult {
    uint8_t rank;
    std::array<uint8_t, 5> kickers;

    bool operator>(const HandResult& other) const;
    bool operator==(const HandResult& other) const;
};

class HandEvaluator {
public:
    HandEvaluator();
    HandResult evaluate_7cards(const std::array<Card, 7>& cards) const;
    std::vector<HandResult> evaluate_batch(const std::vector<std::array<Card, 7>>& hands) const;
private:
    std::array<uint32_t, 52> rank_bits_;
};

} // namespace poker
