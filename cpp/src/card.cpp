#include "poker/card.h"
#include <sstream>
#include <regex>

namespace poker {

std::string Card::to_string() const {
    const char* suits = "hsdc";
    const char* ranks = "23456789TJQKA";
    char buf[3] = {ranks[rank], suits[suit], '\0'};
    return std::string(buf);
}

std::string Card::to_protocol() const {
    return "<" + std::to_string(suit) + "," + std::to_string(rank) + ">";
}

std::array<Card, 52> create_deck() {
    std::array<Card, 52> deck;
    size_t idx = 0;
    for (int s = 0; s < 4; ++s) {
        for (int r = 0; r < 13; ++r) {
            deck[idx++] = Card(s, r);
        }
    }
    return deck;
}

std::vector<Card> parse_protocol_cards(const std::string& s) {
    std::vector<Card> cards;
    std::regex re(R"(<(\d+),(\d+)>)");
    auto begin = std::sregex_iterator(s.begin(), s.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        int suit = std::stoi((*it)[1].str());
        int rank = std::stoi((*it)[2].str());
        cards.emplace_back(suit, rank);
    }
    return cards;
}

} // namespace poker
