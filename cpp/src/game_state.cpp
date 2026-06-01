#include "poker/game_state.h"

namespace poker {

std::vector<ActionRecord> GameState::get_stage_history() const {
    std::vector<ActionRecord> res;
    for (const auto& a : history) {
        if (a.stage == stage) res.push_back(a);
    }
    return res;
}

bool GameState::is_first_action() const {
    return get_stage_history().empty();
}

} // namespace poker
