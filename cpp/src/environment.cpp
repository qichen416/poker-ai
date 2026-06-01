#include "poker/environment.h"
#include <algorithm>
#include <random>

namespace poker {

SelfPlayEnv::SelfPlayEnv() : rng_(std::random_device{}()) {}

GameState SelfPlayEnv::reset(uint64_t seed) {
    if (seed != 0) rng_.seed(seed);
    state_ = GameState();
    deal_cards();
    return state_;
}

void SelfPlayEnv::deal_cards() {
    auto deck = create_deck();
    std::shuffle(deck.begin(), deck.end(), rng_);
    state_.my_cards = {deck[0], deck[1]};
}

SelfPlayEnv::StepResult SelfPlayEnv::step(ActionType action, int amount) {
    StepResult res;
    res.state = state_;
    res.reward = 0.0;
    res.done = false;
    return res;
}

std::vector<SelfPlayEnv::StepResult> SelfPlayEnv::step_batch(
    const std::vector<ActionType>& actions,
    const std::vector<int>& amounts
) {
    std::vector<StepResult> results;
    results.reserve(actions.size());
    for (size_t i = 0; i < actions.size(); ++i) {
        results.push_back(step(actions[i], amounts[i]));
    }
    return results;
}

void SelfPlayEnv::set_opponent_policy(OpponentPolicy policy) {
    opponent_policy_ = policy;
}

void SelfPlayEnv::run_opponent_turn() {
    if (opponent_policy_) {
        // TODO
    }
}

} // namespace poker
