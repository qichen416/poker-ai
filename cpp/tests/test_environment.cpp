#include "poker/environment.h"
#include <algorithm>
#include <iostream>

using namespace poker;

static int failures = 0;

static void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        ++failures;
    }
}

int main() {
    {
        // reset 必须建立合法翻前状态，并给决策层提供正确动作集合。
        SelfPlayEnv env;
        const GameState state = env.reset(42);

        check(state.stage == Stage::PREFLOP, "reset starts preflop");
        check(state.my_position == Position::SB, "hero starts as small blind");
        check(state.my_chips == 19950, "small blind is posted");
        check(state.opponent_chips == 19900, "big blind is posted");
        check(state.pot == 150, "pot contains both blinds");
        check(state.to_call == 50, "small blind faces 50 to call");
        check(state.my_cards[0].index() != state.my_cards[1].index(), "hole cards are unique");
        check(env.is_action_legal(ActionType::CALL), "call is legal facing the big blind");
        check(env.is_action_legal(ActionType::FOLD), "fold is legal facing a bet");
        check(env.is_action_legal(ActionType::RAISE, 200), "raise-to 200 is legal");
        check(!env.is_action_legal(ActionType::CHECK), "check is illegal facing a bet");
        check(!env.is_action_legal(ActionType::BET, 100), "bet is illegal when a bet exists");
        const auto actions = env.legal_actions();
        check(std::find(actions.begin(), actions.end(), ActionType::RAISE) != actions.end(),
              "legal action list includes raise when a minimum raise is affordable");
    }

    {
        // 跟注盲注后由大盲自动过牌，环境应进入翻牌圈并公开三张牌。
        SelfPlayEnv env;
        env.set_opponent_policy([](const GameState&) { return ActionType::CHECK; });
        env.reset(42);
        const auto result = env.step(ActionType::CALL, 0);

        check(!result.done, "calling the blind does not end the hand");
        check(result.state.stage == Stage::FLOP, "call then big-blind check advances to flop");
        check(result.state.num_community == 3, "flop exposes three community cards");
        check(result.state.to_call == 0, "opponent check leaves nothing to call");
        check(result.state.history.size() == 3, "call and two automatic checks are recorded");
        check(env.is_action_legal(ActionType::CHECK), "hero can check after opponent checks");
        check(env.is_action_legal(ActionType::BET, 100), "hero can bet after opponent checks");
        const auto actions = env.legal_actions();
        check(std::find(actions.begin(), actions.end(), ActionType::BET) != actions.end(),
              "legal action list includes bet on an unopened street");
    }

    {
        // 弃牌立即终局，只损失已经投入的小盲，且底池转给对手。
        SelfPlayEnv env;
        const GameState start = env.reset(7);
        const auto result = env.step(ActionType::FOLD, 0);

        check(result.done, "fold ends the hand");
        check(result.reward == -50.0, "fold reward equals the lost small blind");
        check(result.state.opponent_chips == 20050, "opponent receives the pot after fold");
        check(result.state.pot == 0, "pot is cleared after settlement");
        check(start.my_cards[0].index() != start.my_cards[1].index(), "seeded reset remains valid");
    }

    {
        // 双方一路过牌必须稳定走到摊牌，并保持总筹码守恒。
        SelfPlayEnv env;
        env.set_opponent_policy([](const GameState&) { return ActionType::CHECK; });
        env.reset(99);
        env.step(ActionType::CALL, 0);
        env.step(ActionType::CHECK, 0);
        env.step(ActionType::CHECK, 0);
        const auto result = env.step(ActionType::CHECK, 0);

        check(result.done, "a checked-down hand reaches showdown");
        check(result.state.stage == Stage::SHOWDOWN, "checked-down hand ends at showdown");
        check(result.state.num_community == 5, "showdown exposes all five board cards");
        check(result.state.pot == 0, "showdown distributes the pot");
        check(result.state.my_chips + result.state.opponent_chips == 40000,
              "showdown conserves all chips");
        check(result.state.history.size() == 8, "complete check-down history is recorded");
    }

    {
        // 翻牌下注被跟注后应正确累积底池并进入转牌圈。
        SelfPlayEnv env;
        env.set_opponent_policy([](const GameState&) { return ActionType::CALL; });
        env.reset(123);
        env.step(ActionType::CALL, 0);
        const auto result = env.step(ActionType::BET, 200);

        check(!result.done, "a called flop bet continues the hand");
        check(result.state.stage == Stage::TURN, "called flop bet advances to turn");
        check(result.state.pot == 600, "pot includes blinds, preflop call, bet, and call");
        check(result.state.to_call == 0, "automatic opponent check leaves hero free to act");
    }

    {
        // 翻前全押被默认对手跟注后应自动发完牌面并结算。
        SelfPlayEnv env;
        env.reset(321);
        const auto result = env.step(ActionType::ALLIN, 0);

        check(result.done, "called preflop all-in ends at showdown");
        check(result.state.stage == Stage::SHOWDOWN, "all-in runs out the board");
        check(result.state.num_community == 5, "all-in reveals all community cards");
        check(result.state.my_chips + result.state.opponent_chips == 40000,
              "all-in settlement conserves all chips");
    }

    {
        // 面对大盲时 check 非法，环境必须显式报错而不是静默接受。
        SelfPlayEnv env;
        env.reset(1);
        bool threw = false;
        try {
            env.step(ActionType::CHECK, 0);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "illegal actions raise an explicit error");
    }

    if (failures == 0) {
        std::cout << "Environment reset test passed!" << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
