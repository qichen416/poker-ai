#include "poker/card.h"
#include <iostream>
#include <stdexcept>

using namespace poker;

int main() {
    // 构造函数和协议解析器都必须在非法牌进入计算核心前拒绝它。
    bool constructor_threw = false;
    try {
        Card invalid(4, 0);
    } catch (const std::out_of_range&) {
        constructor_threw = true;
    }

    bool parser_threw = false;
    try {
        parse_protocol_cards("<0,12><3,13>");
    } catch (const std::out_of_range&) {
        parser_threw = true;
    }

    if (!constructor_threw || !parser_threw) {
        std::cerr << "Invalid cards must be rejected" << std::endl;
        return 1;
    }
    return 0;
}
