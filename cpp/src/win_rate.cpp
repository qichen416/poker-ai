#include "poker/win_rate.h"
#include "poker/hand_evaluator.h"
#include <random>
#include <algorithm>
#include <thread>
#include <future>
#include <vector>

namespace poker {

double WinRateCalculator::calculate(
    const std::array<Card, 2>& my_cards,
    const std::vector<Card>& community,
    int num_opponents,
    int n_simulations) const {

    HandEvaluator eval;
    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<Card> deck = []() {
        auto d = create_deck();
        return std::vector<Card>(d.begin(), d.end());
    }();

    std::array<bool, 52> used{};
    used.fill(false);
    for (const auto& c : my_cards) used[c.index()] = true;
    for (const auto& c : community) used[c.index()] = true;

    int wins = 0;
    int total = 0;

    for (int sim = 0; sim < n_simulations; ++sim) {
        std::array<Card, 7> my_hand;
        my_hand[0] = my_cards[0];
        my_hand[1] = my_cards[1];

        std::vector<Card> remaining;
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

        bool win = true;
        for (int o = 0; o < num_opponents; ++o) {
            std::array<Card, 7> opp_hand;
            opp_hand[0] = opp_hands[o][0];
            opp_hand[1] = opp_hands[o][1];
            for (int i = 0; i < 5; ++i) opp_hand[2+i] = my_hand[2+i];
            auto opp_res = eval.evaluate_7cards(opp_hand);
            if (opp_res > my_res) {
                win = false;
                break;
            }
        }
        if (win) ++wins;
        ++total;
    }

    return total > 0 ? static_cast<double>(wins) / total : 0.0;
}

std::vector<double> WinRateCalculator::calculate_batch(
    const std::vector<std::array<Card, 2>>& my_cards_list,
    const std::vector<std::vector<Card>>& community_list,
    int num_opponents,
    int n_simulations_each) const {

    std::vector<double> results;
    results.reserve(my_cards_list.size());
    for (size_t i = 0; i < my_cards_list.size(); ++i) {
        results.push_back(calculate(my_cards_list[i], community_list[i], num_opponents, n_simulations_each));
    }
    return results;
}

} // namespace poker
