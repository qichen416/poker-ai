#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>

namespace poker {

struct Card {
    uint8_t suit;
    uint8_t rank;

    Card() = default;
    Card(uint8_t s, uint8_t r) : suit(s), rank(r) {
        // 尽早拒绝非法牌，避免越界值进入 index()、牌型评估和蒙特卡洛牌堆。
        if (s >= 4 || r >= 13) {
            throw std::out_of_range("card suit or rank is out of range");
        }
    }

    bool operator==(const Card& other) const {
        return suit == other.suit && rank == other.rank;
    }

    uint8_t index() const { return suit * 13 + rank; }

    std::string to_string() const;
    std::string to_protocol() const;
};

std::array<Card, 52> create_deck();
std::vector<Card> parse_protocol_cards(const std::string& s);

} // namespace poker
