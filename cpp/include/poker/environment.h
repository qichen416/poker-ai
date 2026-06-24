#pragma once
#include "poker/game_state.h"
#include "poker/card.h"
#include <vector>
#include <array>
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
    // 返回当前英雄玩家可选的动作类型，供训练器或决策层生成动作掩码。
    std::vector<ActionType> legal_actions() const;
    // amount 对 BET/RAISE 表示“加到的本街总投入”，其他动作可保持为 0。
    bool is_action_legal(ActionType action, int amount = 0) const;
    std::vector<StepResult> step_batch(
        const std::vector<ActionType>& actions,
        const std::vector<int>& amounts
    );
    void set_opponent_policy(OpponentPolicy policy);
private:
    GameState state_;
    std::mt19937 rng_;
    OpponentPolicy opponent_policy_;
    // 对手底牌和未来公共牌只保存在环境内部，避免训练策略提前看到隐藏信息。
    std::array<Card, 2> opponent_cards_{};
    std::array<Card, 5> board_{};
    // 本街投入单独跟踪，用于计算跟注差额、最小加注和下注轮结束条件。
    int hero_street_commit_ = 0;
    int opponent_street_commit_ = 0;
    int current_bet_ = 0;
    int minimum_raise_ = 100;
    // 两人都行动且投入相等时，本下注轮才可以结束。
    bool hero_acted_ = false;
    bool opponent_acted_ = false;
    bool hero_turn_ = true;
    bool done_ = false;
    // 奖励使用英雄相对开局筹码的净变化，适合直接作为自博弈终局奖励。
    int initial_hero_chips_ = 20000;

    void deal_cards();
    void run_opponent_turn();
    bool apply_action(bool opponent, ActionType action, int amount);
    bool betting_round_complete() const;
    void advance_stage();
    StepResult make_result() const;
    void finish_by_fold(bool opponent_folded);
    void finish_showdown();
    void update_public_state();
};

} // namespace poker
