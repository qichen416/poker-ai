#include "poker/card.h"
#include "poker/hand_evaluator.h"
#include <iostream>
#include <cassert>

using namespace poker;

int main() {
    HandEvaluator eval;

    std::array<Card, 7> hand1 = {
        Card(0, 12), Card(1, 11),
        Card(2, 5), Card(3, 3), Card(0, 1), Card(1, 0), Card(2, 9)
    };
    auto res1 = eval.evaluate_7cards(hand1);
    assert(res1.rank == 1);
    std::cout << "Test 1 passed: High card" << std::endl;

    std::array<Card, 7> hand2 = {
        Card(0, 12), Card(1, 12),
        Card(2, 5), Card(3, 3), Card(0, 1), Card(1, 0), Card(2, 9)
    };
    auto res2 = eval.evaluate_7cards(hand2);
    assert(res2.rank == 2);
    std::cout << "Test 2 passed: One pair" << std::endl;

    std::cout << "All C++ hand eval tests passed!" << std::endl;
    return 0;
}
