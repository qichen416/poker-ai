#pragma once
#include "poker/game_state.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <array>

namespace poker {

using ActionProbs = std::array<double, 6>;

class CFREngine {
public:
    CFREngine();
    std::string make_info_set_key(const GameState& state) const;
    ActionProbs get_strategy(const std::string& info_set_key) const;
    ActionProbs get_average_strategy(const std::string& info_set_key) const;
    void train_iteration(const GameState& root_state);
    void train_iterations(int n, const GameState& root_state);
    void save_strategy(const std::string& filepath) const;
    void load_strategy(const std::string& filepath);
    size_t num_info_sets() const { return regret_sum_.size(); }
private:
    std::unordered_map<std::string, std::array<double, 6>> regret_sum_;
    std::unordered_map<std::string, std::array<double, 6>> strategy_sum_;
    double mccfr_traverse(const GameState& state, int player, double p0, double p1);
};

} // namespace poker
