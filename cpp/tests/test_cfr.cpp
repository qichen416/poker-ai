#include "poker/cfr_engine.h"
#include "poker/game_state.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace poker;

int main() {
    CFREngine engine;
    GameState state;

    auto key = engine.make_info_set_key(state);
    auto strat = engine.get_strategy(key);

    double sum = 0;
    for (auto s : strat) sum += s;
    assert(std::abs(sum - 1.0) < 1e-6);

    std::cout << "CFR engine basic test passed!" << std::endl;
    return 0;
}
