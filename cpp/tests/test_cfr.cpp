#include "poker/cfr_engine.h"
#include "poker/game_state.h"
#include <iostream>
#include <cmath>

using namespace poker;

int main() {
    CFREngine engine;
    GameState state;

    auto key = engine.make_info_set_key(state);
    auto strat = engine.get_strategy(key);

    double sum = 0;
    for (auto s : strat) sum += s;
    if (std::abs(sum - 1.0) >= 1e-6) {
        std::cerr << "FAIL: CFR strategy probabilities do not sum to one" << std::endl;
        return 1;
    }

    std::cout << "CFR engine basic test passed!" << std::endl;
    return 0;
}
