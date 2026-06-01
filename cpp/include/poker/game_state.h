#pragma once
#include "poker/card.h"
#include <vector>
#include <string>
#include <cstdint>

namespace poker {

enum class Stage : uint8_t { PREFLOP, FLOP, TURN, RIVER, SHOWDOWN };
enum class Position : uint8_t { SB, BB, BUTTON };
enum class ActionType : uint8_t { FOLD, CHECK, CALL, BET, RAISE, ALLIN };

struct ActionRecord {
    Stage stage;
    ActionType action;
    int amount;
    bool is_opponent;
};

struct GameState {
    std::array<Card, 2> my_cards;
    std::array<Card, 5> community_cards;
    uint8_t num_community = 0;
    Position my_position = Position::BB;
    Stage stage = Stage::PREFLOP;
    int my_chips = 20000;
    int opponent_chips = 20000;
    int pot = 0;
    int to_call = 0;
    std::vector<ActionRecord> history;
    float opponent_vpip = -1.0f;
    float opponent_pfr = -1.0f;
    float opponent_af = -1.0f;

    std::vector<ActionRecord> get_stage_history() const;
    bool is_first_action() const;
};

} // namespace poker
