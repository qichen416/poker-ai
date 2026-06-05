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

static HandResult evaluate_5cards(const std::array<Card, 5>& cards) {
    int rank_count[13] = {0};
    int suit_count[4] = {0};
    for (const auto& c : cards) {
        rank_count[c.rank]++;
        suit_count[c.suit]++;
    }

    // Flush check
    bool is_flush = false;
    for (int s = 0; s < 4; ++s) {
        if (suit_count[s] == 5) { is_flush = true; break; }
    }

    // Straight check (including A-2-3-4-5 wheel)
    bool is_straight = false;
    int straight_high = 0;
    for (int high = 12; high >= 3; --high) {
        if (rank_count[high] && rank_count[high-1] && rank_count[high-2] &&
            rank_count[high-3] && rank_count[high-4]) {
            is_straight = true;
            straight_high = high;
            break;
        }
    }
    if (!is_straight && rank_count[12] && rank_count[0] && rank_count[1] &&
        rank_count[2] && rank_count[3]) {
        is_straight = true;
        straight_high = 3; // 5-high straight (wheel)
    }

    // Count pairs, trips, quads
    int pairs = 0, trips = 0, quads = 0;
    int pair_ranks[2] = {-1, -1};
    int trip_rank = -1, quad_rank = -1;
    for (int r = 12; r >= 0; --r) {
        if (rank_count[r] == 4) { quads = 1; quad_rank = r; }
        else if (rank_count[r] == 3) { trips = 1; trip_rank = r; }
        else if (rank_count[r] == 2) {
            if (pairs < 2) pair_ranks[pairs] = r;
            pairs++;
        }
    }

    // Determine hand rank
    HandResult res;
    if (is_flush && is_straight) {
        res.rank = static_cast<uint8_t>(HandRank::STRAIGHT_FLUSH);
    } else if (quads == 1) {
        res.rank = static_cast<uint8_t>(HandRank::FOUR_KIND);
    } else if (trips == 1 && pairs >= 1) {
        res.rank = static_cast<uint8_t>(HandRank::FULL_HOUSE);
    } else if (is_flush) {
        res.rank = static_cast<uint8_t>(HandRank::FLUSH);
    } else if (is_straight) {
        res.rank = static_cast<uint8_t>(HandRank::STRAIGHT);
    } else if (trips == 1) {
        res.rank = static_cast<uint8_t>(HandRank::THREE_KIND);
    } else if (pairs >= 2) {
        res.rank = static_cast<uint8_t>(HandRank::TWO_PAIR);
    } else if (pairs == 1) {
        res.rank = static_cast<uint8_t>(HandRank::ONE_PAIR);
    } else {
        res.rank = static_cast<uint8_t>(HandRank::HIGH_CARD);
    }

    // Build kickers
    std::vector<uint8_t> kickers;
    auto add_remaining = [&]() {
        for (int r = 12; r >= 0; --r) {
            for (int c = 0; c < rank_count[r]; ++c) {
                kickers.push_back(static_cast<uint8_t>(r));
            }
        }
    };

    switch (static_cast<HandRank>(res.rank)) {
    case HandRank::STRAIGHT_FLUSH:
    case HandRank::STRAIGHT:
        kickers.push_back(static_cast<uint8_t>(straight_high));
        break;
    case HandRank::FOUR_KIND:
        kickers.push_back(static_cast<uint8_t>(quad_rank));
        add_remaining();
        break;
    case HandRank::FULL_HOUSE:
        kickers.push_back(static_cast<uint8_t>(trip_rank));
        kickers.push_back(static_cast<uint8_t>(pair_ranks[0]));
        break;
    case HandRank::FLUSH:
    case HandRank::HIGH_CARD:
        add_remaining();
        break;
    case HandRank::THREE_KIND:
        kickers.push_back(static_cast<uint8_t>(trip_rank));
        add_remaining();
        break;
    case HandRank::TWO_PAIR:
        kickers.push_back(static_cast<uint8_t>(pair_ranks[0]));
        kickers.push_back(static_cast<uint8_t>(pair_ranks[1]));
        add_remaining();
        break;
    case HandRank::ONE_PAIR:
        kickers.push_back(static_cast<uint8_t>(pair_ranks[0]));
        add_remaining();
        break;
    }

    for (int i = 0; i < 5 && i < static_cast<int>(kickers.size()); ++i) {
        res.kickers[i] = kickers[i];
    }
    for (int i = static_cast<int>(kickers.size()); i < 5; ++i) {
        res.kickers[i] = 0;
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
