#include "poker/win_rate.h"
#include "poker/hand_evaluator.h"
#include <random>
#include <algorithm>
#include <thread>
#include <future>
#include <vector>
#include <stdexcept>

namespace poker {

double WinRateCalculator::calculate(
    const std::array<Card, 2>& my_cards,
    const std::vector<Card>& community,
    int num_opponents,
    int n_simulations) const {

    if (num_opponents < 1) {
        throw std::invalid_argument("num_opponents must be at least one");
    }
    if (n_simulations <= 0) {
        throw std::invalid_argument("n_simulations must be positive");
    }
    if (community.size() > 5) {
        throw std::invalid_argument("community cannot contain more than five cards");
    }

    HandEvaluator eval;
    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<Card> deck = []() {
        auto d = create_deck();
        return std::vector<Card>(d.begin(), d.end());
    }();

    std::array<bool, 52> used{};
    used.fill(false);
    // 已知牌必须唯一，否则剩余牌堆和模拟概率都会失真。
    auto mark_used = [&used](const Card& card) {
        if (used[card.index()]) {
            throw std::invalid_argument("known cards must be unique");
        }
        used[card.index()] = true;
    };
    for (const auto& c : my_cards) mark_used(c);
    for (const auto& c : community) mark_used(c);

    const int cards_needed = num_opponents * 2 + (5 - static_cast<int>(community.size()));
    const int cards_remaining = 52 - 2 - static_cast<int>(community.size());
    if (cards_needed > cards_remaining) {
        throw std::invalid_argument("not enough cards for the requested opponents");
    }

    double equity = 0.0;
    int total = 0;

    for (int sim = 0; sim < n_simulations; ++sim) {
        std::array<Card, 7> my_hand;
        my_hand[0] = my_cards[0];
        my_hand[1] = my_cards[1];

        std::vector<Card> remaining;
        // 每次模拟从相同的合法剩余牌集合重新洗牌，避免跨模拟污染牌堆。
        for (const auto& c : deck) {
            if (!used[c.index()]) remaining.push_back(c);
        }
        std::shuffle(remaining.begin(), remaining.end(), rng);

        size_t idx = 0;
        std::vector<std::array<Card, 2>> opp_hands(num_opponents);
        for (int o = 0; o < num_opponents; ++o) {
            opp_hands[o] = {remaining[idx], remaining[idx+1]};
            idx += 2;
        }

        int comm_size = static_cast<int>(community.size());
        for (int i = 0; i < comm_size; ++i) {
            my_hand[2 + i] = community[i];
        }
        for (int i = comm_size; i < 5; ++i) {
            my_hand[2 + i] = remaining[idx++];
        }

        auto my_res = eval.evaluate_7cards(my_hand);

        bool beaten = false;
        int tied_opponents = 0;
        for (int o = 0; o < num_opponents; ++o) {
            std::array<Card, 7> opp_hand;
            opp_hand[0] = opp_hands[o][0];
            opp_hand[1] = opp_hands[o][1];
            for (int i = 0; i < 5; ++i) opp_hand[2+i] = my_hand[2+i];
            auto opp_res = eval.evaluate_7cards(opp_hand);
            if (opp_res > my_res) {
                beaten = true;
                break;
            } else if (opp_res == my_res) {
                ++tied_opponents;
            }
        }
        if (!beaten) {
            // 返回的是底池权益而非“未输概率”：N 人并列时英雄只占 1/N。
            equity += 1.0 / static_cast<double>(tied_opponents + 1);
        }
        ++total;
    }

    return equity / static_cast<double>(total);
}

std::vector<double> WinRateCalculator::calculate_batch(
    const std::vector<std::array<Card, 2>>& my_cards_list,
    const std::vector<std::vector<Card>>& community_list,
    int num_opponents,
    int n_simulations_each) const {

    if (my_cards_list.size() != community_list.size()) {
        throw std::invalid_argument("hand and community batches must have the same size");
    }
    std::vector<double> results;
    results.reserve(my_cards_list.size());
    for (size_t i = 0; i < my_cards_list.size(); ++i) {
        results.push_back(calculate(my_cards_list[i], community_list[i], num_opponents, n_simulations_each));
    }
    return results;
}

} // namespace poker
