#pragma once
#include "poker/card.h"
#include <array>
#include <vector>

namespace poker {

class WinRateCalculator {
public:
    double calculate(
        const std::array<Card, 2>& my_cards,
        const std::vector<Card>& community,
        int num_opponents,
        int n_simulations
    ) const;
    std::vector<double> calculate_batch(
        const std::vector<std::array<Card, 2>>& my_cards_list,
        const std::vector<std::vector<Card>>& community_list,
        int num_opponents,
        int n_simulations_each
    ) const;
};

} // namespace poker
