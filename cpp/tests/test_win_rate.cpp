#include "poker/win_rate.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace poker;

static int failures = 0;

static void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        ++failures;
    }
}

int main() {
    WinRateCalculator calculator;
    const std::array<Card, 2> hole{Card(1, 0), Card(2, 1)};
    const std::vector<Card> royal_board{
        Card(0, 12), Card(0, 11), Card(0, 10), Card(0, 9), Card(0, 8)
    };

    // 公共牌本身构成皇家同花顺时，所有玩家必然平局，应按人数平分权益。
    const double heads_up = calculator.calculate(hole, royal_board, 1, 20);
    check(std::abs(heads_up - 0.5) < 1e-9,
          "a forced heads-up board tie has 0.5 equity");

    const double three_way = calculator.calculate(hole, royal_board, 2, 20);
    check(std::abs(three_way - (1.0 / 3.0)) < 1e-9,
          "a forced three-way board tie has one-third equity");

    bool duplicate_threw = false;
    try {
        calculator.calculate(
            {Card(0, 12), Card(0, 12)}, {}, 1, 10
        );
    } catch (const std::invalid_argument&) {
        duplicate_threw = true;
    }
    check(duplicate_threw, "duplicate known cards are rejected");

    bool invalid_simulations_threw = false;
    try {
        calculator.calculate(hole, {}, 1, 0);
    } catch (const std::invalid_argument&) {
        invalid_simulations_threw = true;
    }
    check(invalid_simulations_threw, "non-positive simulation counts are rejected");

    return failures == 0 ? 0 : 1;
}
