'''
C++核心库的Python包装 (Python 3.10 + CUDA 12.6)。
'''
import sys

if sys.version_info < (3, 10) or sys.version_info >= (3, 11):
    raise ImportError(
        "This package requires Python 3.10.x exactly. "
        "Current version: %s" % sys.version
    )

try:
    from poker_core._core import (
        Card, create_deck, parse_protocol_cards,
        HandEvaluator, HandResult,
        GameState, ActionRecord, Stage, Position, ActionType,
        CFREngine,
        WinRateCalculator,
        SelfPlayEnv, StepResult,
    )
except ImportError as e:
    raise ImportError(
        "C++ core library not compiled for Python 3.10.\n"
        "Windows MSVC: cd cpp && mkdir build && cd build && "
        "cmake .. -G \"Visual Studio 17 2022\" -A x64 "
        "-DPYBIND11_PYTHON_VERSION=3.10 && "
        "cmake --build . --config Release\n"
        "Then: cd ../.. && pip install -e ."
    ) from e


class CardUtils:
    """扑克牌工具类，调用底层 C++ 实现的静态方法进行牌面解析和转换"""

    @staticmethod
    def create_deck():
        """创建一副完整的 52 张扑克牌"""
        return create_deck()

    @staticmethod
    def parse_protocol_cards(msg: str):
        """
        从协议字符串中解析出所有卡牌。

        支持格式示例::

            "preflop|BIGBLIND|<0,12><1,11>"
            "flop|<0,12><1,11><2,10>"

        :param msg: 包含 ``<suit,rank>`` 片段的任意字符串
        :return: 按出现顺序解析出的 Card 列表
        """
        return parse_protocol_cards(msg)

    @staticmethod
    def cards_to_protocol(cards):
        """将 Card 列表拼接为协议字符串，如 '<0,12><1,11>'"""
        return "".join(card.to_protocol() for card in cards)


# 给 pybind11 枚举补上 __iter__，使其支持 for...in 遍历
# 三个枚举共享同一个 metaclass，所以用一个通用函数
_meta = type(ActionType)
_ENUM_MEMBERS = {
    'ActionType': lambda cls: [cls.FOLD, cls.CHECK, cls.CALL, cls.BET, cls.RAISE, cls.ALLIN],
    'Position':   lambda cls: [cls.SB, cls.BB, cls.BUTTON],
    'Stage':      lambda cls: [cls.PREFLOP, cls.FLOP, cls.TURN, cls.RIVER, cls.SHOWDOWN],
}
def _enum_iter(cls):
    members = _ENUM_MEMBERS.get(cls.__name__, lambda c: [])(cls)
    return iter(members)
_meta.__iter__ = _enum_iter


__all__ = [
    'Card', 'CardUtils', 'create_deck', 'parse_protocol_cards',
    'HandEvaluator', 'HandResult',
    'GameState', 'ActionRecord', 'Stage', 'Position', 'ActionType',
    'CFREngine',
    'WinRateCalculator',
    'SelfPlayEnv', 'StepResult',
]
