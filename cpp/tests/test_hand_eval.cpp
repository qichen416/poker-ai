#include "poker/card.h"
#include "poker/hand_evaluator.h"
#include <iostream>
#include <cassert>

using namespace poker;

static int passed = 0, failed = 0;

static void check_rank(const char* name, const Card* cards_7, uint8_t expected) {
    HandEvaluator ev;
    std::array<Card, 7> hand;
    for (int i = 0; i < 7; ++i) hand[i] = cards_7[i];
    HandResult res = ev.evaluate_7cards(hand);
    if (res.rank == expected) {
        std::cout << "  PASS: " << name << " (rank=" << int(res.rank) << ")" << std::endl;
        passed++;
    } else {
        std::cout << "  FAIL: " << name
                  << " expected=" << int(expected)
                  << " got=" << int(res.rank) << std::endl;
        failed++;
    }
}

int main() {
    std::cout << "=== Poker Hand Evaluator Tests ===" << std::endl;

    // 1. High Card
    {
        Card c[7] = {
            Card(0,12), Card(1,10), Card(2,7), Card(3,4), Card(0,2), Card(1,0), Card(2,8)};
        check_rank("High Card", c, 1);
    }

    // 2. One Pair
    {
        Card c[7] = {
            Card(0,12), Card(1,12), Card(2,8), Card(3,5), Card(0,3), Card(1,1), Card(2,10)};
        check_rank("One Pair", c, 2);
    }

    // 3. Two Pair
    {
        Card c[7] = {
            Card(0,12), Card(1,12), Card(2,8), Card(3,8), Card(0,5), Card(1,3), Card(2,1)};
        check_rank("Two Pair", c, 3);
    }

    // 4. Three of a Kind
    {
        Card c[7] = {
            Card(0,12), Card(1,12), Card(2,12), Card(3,8), Card(0,5), Card(1,3), Card(2,1)};
        check_rank("Three of a Kind", c, 4);
    }

    // 5. Straight
    {
        Card c[7] = {
            Card(0,11), Card(1,10), Card(2,9), Card(3,8), Card(0,7), Card(1,2), Card(2,0)};
        check_rank("Straight", c, 5);
    }

    // 5b. Wheel Straight (A-2-3-4-5)
    {
        Card c[7] = {
            Card(0,12), Card(1,0), Card(2,1), Card(3,2), Card(0,3), Card(1,7), Card(2,9)};
        check_rank("Wheel Straight (A-5)", c, 5);
    }

    // 6. Flush
    {
        Card c[7] = {
            Card(0,12), Card(0,10), Card(0,7), Card(0,5), Card(0,2), Card(1,8), Card(2,3)};
        check_rank("Flush", c, 6);
    }

    // 7. Full House
    {
        Card c[7] = {
            Card(0,12), Card(1,12), Card(2,12), Card(3,8), Card(0,8), Card(1,3), Card(2,1)};
        check_rank("Full House", c, 7);
    }

    // 8. Four of a Kind
    {
        Card c[7] = {
            Card(0,12), Card(1,12), Card(2,12), Card(3,12), Card(0,8), Card(1,3), Card(2,1)};
        check_rank("Four of a Kind", c, 8);
    }

    // 9. Straight Flush
    {
        Card c[7] = {
            Card(0,11), Card(0,10), Card(0,9), Card(0,8), Card(0,7), Card(1,2), Card(2,4)};
        check_rank("Straight Flush", c, 9);
    }

    // 9b. Royal Flush
    {
        Card c[7] = {
            Card(0,12), Card(0,11), Card(0,10), Card(0,9), Card(0,8), Card(1,2), Card(2,4)};
        check_rank("Royal Flush", c, 9);
    }

    // -- Comparison tests --
    HandEvaluator ev;

    // Straight flush > Four of a kind
    {
        std::array<Card, 7> sf = {
            Card(0,5),Card(0,4),Card(0,3),Card(0,2),Card(0,1),Card(1,8),Card(2,9)};
        std::array<Card, 7> fk = {
            Card(0,12),Card(1,12),Card(2,12),Card(3,12),Card(0,8),Card(1,3),Card(2,1)};
        auto r1 = ev.evaluate_7cards(sf);
        auto r2 = ev.evaluate_7cards(fk);
        if (r1 > r2) {
            std::cout << "  PASS: Straight Flush > Four of a Kind" << std::endl;
            passed++;
        } else {
            std::cout << "  FAIL: Straight Flush > Four of a Kind" << std::endl;
            failed++;
        }
    }

    // AAKKQ > KKQQA (two pair comparison by high pair)
    {
        std::array<Card, 7> h1 = {
            Card(0,12),Card(1,12),Card(2,11),Card(3,11),Card(0,10),Card(1,2),Card(2,3)};
        std::array<Card, 7> h2 = {
            Card(0,11),Card(1,11),Card(2,10),Card(3,10),Card(0,12),Card(1,2),Card(2,3)};
        auto r1 = ev.evaluate_7cards(h1);
        auto r2 = ev.evaluate_7cards(h2);
        if (r1 > r2 && !(r2 > r1)) {
            std::cout << "  PASS: AAKKQ > KKQQA (two pair)" << std::endl;
            passed++;
        } else {
            std::cout << "  FAIL: AAKKQ > KKQQA (two pair)" << std::endl;
            failed++;
        }
    }

    // Identical hands should tie
    {
        std::array<Card, 7> h1 = {
            Card(0,12),Card(1,12),Card(2,8),Card(3,5),Card(0,3),Card(1,1),Card(2,10)};
        std::array<Card, 7> h2 = {
            Card(2,12),Card(3,12),Card(2,8),Card(3,5),Card(0,3),Card(1,1),Card(2,10)};
        auto r1 = ev.evaluate_7cards(h1);
        auto r2 = ev.evaluate_7cards(h2);
        bool fail1 = r1 > r2;
        bool fail2 = r2 > r1;
        if (!fail1 && !fail2) {
            std::cout << "  PASS: Identical hands tie" << std::endl;
            passed++;
        } else {
            std::cout << "  FAIL: Identical hands tie" << std::endl;
            failed++;
        }
    }

    std::cout << "\n========== Results: " << passed << " passed, "
              << failed << " failed ==========" << std::endl;
    return failed == 0 ? 0 : 1;
}
