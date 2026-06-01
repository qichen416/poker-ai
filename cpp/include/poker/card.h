#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace poker {

struct Card {
    uint8_t suit;
    uint8_t rank;

    Card() = default;
    Card(uint8_t s, uint8_t r) : suit(s), rank(r) {}

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
