#pragma once
#include "poker/game_state.h"
#include "poker/card.h"
#include <vector>
#include <random>
#include <functional>

namespace poker {

using OpponentPolicy = std::function<ActionType(const GameState&)>;

class SelfPlayEnv {
public:
    SelfPlayEnv();
    GameState reset(uint64_t seed = 0);
    struct StepResult {
        GameState state;
        double reward;
        bool done;
    };
    StepResult step(ActionType action, int amount);
    std::vector<StepResult> step_batch(
        const std::vector<ActionType>& actions,
        const std::vector<int>& amounts
    );
    void set_opponent_policy(OpponentPolicy policy);
private:
    GameState state_;
    std::mt19937 rng_;
    OpponentPolicy opponent_policy_;
    void deal_cards();
    void run_opponent_turn();
};

} // namespace poker
