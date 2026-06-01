#include "poker/cfr_engine.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

namespace poker {

CFREngine::CFREngine() {}

std::string CFREngine::make_info_set_key(const GameState& state) const {
    return "default";
}

ActionProbs CFREngine::get_strategy(const std::string& key) const {
    auto it = regret_sum_.find(key);
    if (it == regret_sum_.end()) {
        ActionProbs uniform;
        uniform.fill(1.0 / 6.0);
        return uniform;
    }
    ActionProbs strategy;
    double sum = 0;
    for (int i = 0; i < 6; ++i) {
        strategy[i] = std::max(0.0, it->second[i]);
        sum += strategy[i];
    }
    if (sum > 0) {
        for (auto& s : strategy) s /= sum;
    } else {
        strategy.fill(1.0 / 6.0);
    }
    return strategy;
}

ActionProbs CFREngine::get_average_strategy(const std::string& key) const {
    auto it = strategy_sum_.find(key);
    if (it == strategy_sum_.end()) {
        ActionProbs uniform;
        uniform.fill(1.0 / 6.0);
        return uniform;
    }
    ActionProbs avg;
    double sum = 0;
    for (int i = 0; i < 6; ++i) {
        sum += it->second[i];
    }
    if (sum > 0) {
        for (int i = 0; i < 6; ++i) avg[i] = it->second[i] / sum;
    } else {
        avg.fill(1.0 / 6.0);
    }
    return avg;
}

void CFREngine::train_iterations(int n, const GameState& root) {
    for (int i = 0; i < n; ++i) {
        train_iteration(root);
    }
}

void CFREngine::train_iteration(const GameState& root) {
    // TODO: implement MCCFR
}

void CFREngine::save_strategy(const std::string& filepath) const {
    // TODO: serialize
}

void CFREngine::load_strategy(const std::string& filepath) {
    // TODO: deserialize
}

} // namespace poker
