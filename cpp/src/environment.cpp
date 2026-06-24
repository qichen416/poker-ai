#include "poker/environment.h"
#include "poker/hand_evaluator.h"
#include <algorithm>
#include <stdexcept>

namespace poker {

namespace {
// 当前竞赛规则采用固定盲注和每局固定初始筹码。
constexpr int SMALL_BLIND = 50;
constexpr int BIG_BLIND = 100;
constexpr int STARTING_CHIPS = 20000;
}

SelfPlayEnv::SelfPlayEnv() : rng_(std::random_device{}()) {}

GameState SelfPlayEnv::reset(uint64_t seed) {
    if (seed != 0) {
        // mt19937 接收 32 位种子；拆分 uint64_t 可保留调用方提供的全部种子信息。
        std::seed_seq sequence{
            static_cast<uint32_t>(seed),
            static_cast<uint32_t>(seed >> 32)
        };
        rng_.seed(sequence);
    }

    state_ = GameState();
    initial_hero_chips_ = STARTING_CHIPS;
    state_.my_position = Position::SB;
    state_.my_chips = STARTING_CHIPS - SMALL_BLIND;
    state_.opponent_chips = STARTING_CHIPS - BIG_BLIND;
    state_.pot = SMALL_BLIND + BIG_BLIND;
    state_.stage = Stage::PREFLOP;
    state_.num_community = 0;

    // 单挑翻前小盲先行动，因此英雄初始面对 50 的跟注差额。
    hero_street_commit_ = SMALL_BLIND;
    opponent_street_commit_ = BIG_BLIND;
    current_bet_ = BIG_BLIND;
    minimum_raise_ = BIG_BLIND;
    hero_acted_ = false;
    opponent_acted_ = false;
    hero_turn_ = true;
    done_ = false;

    deal_cards();
    update_public_state();
    return state_;
}

void SelfPlayEnv::deal_cards() {
    auto deck = create_deck();
    std::shuffle(deck.begin(), deck.end(), rng_);
    state_.my_cards = {deck[0], deck[1]};
    opponent_cards_ = {deck[2], deck[3]};
    // 预先生成完整牌面，但只通过 num_community 分街公开。
    for (size_t i = 0; i < board_.size(); ++i) {
        board_[i] = deck[4 + i];
    }
}

std::vector<ActionType> SelfPlayEnv::legal_actions() const {
    std::vector<ActionType> actions;
    for (ActionType action : {
            ActionType::FOLD, ActionType::CHECK, ActionType::CALL,
            ActionType::BET, ActionType::RAISE, ActionType::ALLIN}) {
        int representative_amount = 0;
        // BET/RAISE 必须带金额；这里用最小合法金额探测该动作类型是否可用。
        if (action == ActionType::BET) {
            representative_amount = BIG_BLIND;
        } else if (action == ActionType::RAISE) {
            representative_amount = current_bet_ + minimum_raise_;
        }
        if (is_action_legal(action, representative_amount)) {
            actions.push_back(action);
        }
    }
    return actions;
}

bool SelfPlayEnv::is_action_legal(ActionType action, int amount) const {
    if (done_ || !hero_turn_) return false;

    const int to_call = std::max(0, current_bet_ - hero_street_commit_);
    // maximum_target 是英雄本街最多能“加到”的金额，即当前投入加剩余筹码。
    const int maximum_target = hero_street_commit_ + state_.my_chips;

    switch (action) {
    case ActionType::FOLD:
        return to_call > 0;
    case ActionType::CHECK:
        return to_call == 0;
    case ActionType::CALL:
        return to_call > 0 && state_.my_chips > 0;
    case ActionType::BET:
        return current_bet_ == 0 && amount >= BIG_BLIND && amount <= maximum_target;
    case ActionType::RAISE: {
        if (current_bet_ == 0 || amount <= current_bet_ || amount > maximum_target) return false;
        const bool is_all_in = amount == maximum_target;
        // 短码全押可以不足一个完整最小加注，但不能超过自己的全部筹码。
        return is_all_in || amount - current_bet_ >= minimum_raise_;
    }
    case ActionType::ALLIN:
        return state_.my_chips > 0;
    }
    return false;
}

bool SelfPlayEnv::apply_action(bool opponent, ActionType action, int amount) {
    // 用引用把英雄和对手复用同一套下注状态机，opponent 同时写入动作来源。
    int& chips = opponent ? state_.opponent_chips : state_.my_chips;
    int& committed = opponent ? opponent_street_commit_ : hero_street_commit_;
    bool& acted = opponent ? opponent_acted_ : hero_acted_;
    bool& other_acted = opponent ? hero_acted_ : opponent_acted_;
    const int to_call = std::max(0, current_bet_ - committed);
    const int maximum_target = committed + chips;
    int contribution = 0;

    if (action == ActionType::FOLD) {
        if (to_call == 0) return false;
        state_.history.push_back({state_.stage, action, 0, opponent});
        finish_by_fold(opponent);
        return true;
    }

    if (action == ActionType::CHECK) {
        if (to_call != 0) return false;
    } else if (action == ActionType::CALL) {
        if (to_call == 0 || chips == 0) return false;
        contribution = std::min(to_call, chips);
    } else if (action == ActionType::BET) {
        if (current_bet_ != 0 || amount < BIG_BLIND || amount > maximum_target) return false;
        contribution = amount - committed;
        // 首次下注的大小同时成为后续加注必须达到的最小增量。
        minimum_raise_ = amount;
        current_bet_ = amount;
        other_acted = false;
    } else if (action == ActionType::RAISE) {
        if (current_bet_ == 0 || amount <= current_bet_ || amount > maximum_target) return false;
        const int raise_size = amount - current_bet_;
        const bool is_all_in = amount == maximum_target;
        if (!is_all_in && raise_size < minimum_raise_) return false;
        contribution = amount - committed;
        // amount 是本街目标投入；真正从筹码扣除的是目标值与已有投入的差。
        minimum_raise_ = std::max(minimum_raise_, raise_size);
        current_bet_ = amount;
        other_acted = false;
    } else if (action == ActionType::ALLIN) {
        if (chips == 0) return false;
        const int target = maximum_target;
        contribution = chips;
        if (target > current_bet_) {
            const int raise_size = target - current_bet_;
            if (raise_size >= minimum_raise_) {
                minimum_raise_ = raise_size;
            }
            current_bet_ = target;
            other_acted = false;
        }
    }

    chips -= contribution;
    committed += contribution;
    state_.pot += contribution;
    acted = true;
    state_.history.push_back({state_.stage, action, contribution, opponent});

    if (betting_round_complete()) {
        advance_stage();
    } else {
        hero_turn_ = opponent;
    }
    update_public_state();
    return true;
}

bool SelfPlayEnv::betting_round_complete() const {
    // 一方全押后不再等待其后续动作，直接发完公共牌进入摊牌。
    if (!hero_acted_ || !opponent_acted_) return false;
    return hero_street_commit_ == opponent_street_commit_
        || state_.my_chips == 0
        || state_.opponent_chips == 0;
}

void SelfPlayEnv::advance_stage() {
    if (state_.my_chips == 0 || state_.opponent_chips == 0) {
        state_.stage = Stage::SHOWDOWN;
        state_.num_community = 5;
        finish_showdown();
        return;
    }

    hero_street_commit_ = 0;
    opponent_street_commit_ = 0;
    current_bet_ = 0;
    minimum_raise_ = BIG_BLIND;
    hero_acted_ = false;
    opponent_acted_ = false;
    hero_turn_ = false; // Heads-up: the big blind acts first after the flop.

    switch (state_.stage) {
    case Stage::PREFLOP:
        state_.stage = Stage::FLOP;
        state_.num_community = 3;
        break;
    case Stage::FLOP:
        state_.stage = Stage::TURN;
        state_.num_community = 4;
        break;
    case Stage::TURN:
        state_.stage = Stage::RIVER;
        state_.num_community = 5;
        break;
    case Stage::RIVER:
        state_.stage = Stage::SHOWDOWN;
        finish_showdown();
        break;
    case Stage::SHOWDOWN:
        break;
    }
    update_public_state();
}

SelfPlayEnv::StepResult SelfPlayEnv::step(ActionType action, int amount) {
    if (done_) return make_result();
    if (!hero_turn_) {
        throw std::logic_error("step() called while waiting for the opponent");
    }
    if (!apply_action(false, action, amount)) {
        throw std::invalid_argument("illegal poker action");
    }

    while (!done_ && !hero_turn_) {
        // 对手策略在环境内部自动执行，调用方每次 step 只负责英雄的一次决策。
        run_opponent_turn();
    }
    update_public_state();
    return make_result();
}

std::vector<SelfPlayEnv::StepResult> SelfPlayEnv::step_batch(
    const std::vector<ActionType>& actions,
    const std::vector<int>& amounts
) {
    if (actions.size() != amounts.size()) {
        throw std::invalid_argument("actions and amounts must have the same size");
    }
    std::vector<StepResult> results;
    results.reserve(actions.size());
    for (size_t i = 0; i < actions.size(); ++i) {
        results.push_back(step(actions[i], amounts[i]));
        if (done_) break;
    }
    return results;
}

void SelfPlayEnv::set_opponent_policy(OpponentPolicy policy) {
    opponent_policy_ = std::move(policy);
}

void SelfPlayEnv::run_opponent_turn() {
    if (done_ || hero_turn_) return;

    const int to_call = std::max(0, current_bet_ - opponent_street_commit_);
    ActionType action = opponent_policy_
        ? opponent_policy_(state_)
        : (to_call > 0 ? ActionType::CALL : ActionType::CHECK);
    int amount = 0;

    if (action == ActionType::BET) {
        // 对手策略只返回动作类型时，环境用至少一个大盲、至多全部筹码补出金额。
        amount = std::min(
            opponent_street_commit_ + state_.opponent_chips,
            std::max(BIG_BLIND, state_.pot)
        );
    } else if (action == ActionType::RAISE) {
        amount = std::min(
            opponent_street_commit_ + state_.opponent_chips,
            current_bet_ + std::max(minimum_raise_, state_.pot)
        );
    }

    if (!apply_action(true, action, amount)) {
        // A policy may suggest an action that is illegal in the current state.
        // Use the safest legal fallback so training cannot deadlock.
        action = to_call > 0 ? ActionType::CALL : ActionType::CHECK;
        if (!apply_action(true, action, 0)) {
            throw std::logic_error("no legal fallback action for opponent");
        }
    }
}

void SelfPlayEnv::finish_by_fold(bool opponent_folded) {
    if (opponent_folded) {
        state_.my_chips += state_.pot;
    } else {
        state_.opponent_chips += state_.pot;
    }
    state_.pot = 0;
    done_ = true;
    hero_turn_ = false;
}

void SelfPlayEnv::finish_showdown() {
    // 双方使用各自两张底牌和相同五张公共牌组成七张牌进行比较。
    std::array<Card, 7> hero_hand{
        state_.my_cards[0], state_.my_cards[1],
        board_[0], board_[1], board_[2], board_[3], board_[4]
    };
    std::array<Card, 7> opponent_hand{
        opponent_cards_[0], opponent_cards_[1],
        board_[0], board_[1], board_[2], board_[3], board_[4]
    };

    HandEvaluator evaluator;
    const HandResult hero_result = evaluator.evaluate_7cards(hero_hand);
    const HandResult opponent_result = evaluator.evaluate_7cards(opponent_hand);

    if (hero_result > opponent_result) {
        state_.my_chips += state_.pot;
    } else if (opponent_result > hero_result) {
        state_.opponent_chips += state_.pot;
    } else {
        // 奇数底池无法完全均分时，把多出的 1 筹码分给英雄，保证筹码总量守恒。
        const int hero_share = state_.pot / 2 + state_.pot % 2;
        state_.my_chips += hero_share;
        state_.opponent_chips += state_.pot - hero_share;
    }
    state_.pot = 0;
    done_ = true;
    hero_turn_ = false;
    update_public_state();
}

void SelfPlayEnv::update_public_state() {
    // GameState 只公开已经到达的街道，未发出的 board_ 内容保持隐藏。
    for (uint8_t i = 0; i < state_.num_community; ++i) {
        state_.community_cards[i] = board_[i];
    }
    state_.to_call = done_ || !hero_turn_
        ? 0
        : std::max(0, current_bet_ - hero_street_commit_);
}

SelfPlayEnv::StepResult SelfPlayEnv::make_result() const {
    StepResult result;
    result.state = state_;
    result.reward = done_
        // 非终局奖励保持 0，终局才返回相对开局筹码的净收益。
        ? static_cast<double>(state_.my_chips - initial_hero_chips_)
        : 0.0;
    result.done = done_;
    return result;
}

} // namespace poker
