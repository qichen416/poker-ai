from typing import Optional

from poker_core import (
    ActionRecord,
    ActionType,
    CardUtils,
    GameState,
    Position,
    Stage,
)
from shared.constants import BIG_BLIND, INITIAL_CHIPS, SMALL_BLIND
from shared.utils import parse_action_string


_ACTION_TYPES = {
    "fold": ActionType.FOLD,
    "check": ActionType.CHECK,
    "call": ActionType.CALL,
    "bet": ActionType.BET,
    "raise": ActionType.RAISE,
    "allin": ActionType.ALLIN,
}


class ProtocolParser:
    """将平台协议消息持续合并到同一个 GameState。

    解析器额外保存双方“本街累计投入”。GameState 只有 to_call，无法仅凭它
    正确还原连续 raise/call，因此这两个内部字段是筹码同步的必要上下文。
    """

    def __init__(self):
        # 进入新街道时清零；翻前则初始化为大小盲。
        self._hero_street_commit = 0
        self._opponent_street_commit = 0

    def parse(
        self,
        message: str,
        current_state: Optional[GameState] = None,
    ) -> GameState:
        state = current_state if current_state is not None else GameState()
        message = message.strip()

        # 街道消息负责牌面和阶段，动作消息默认来自对手。
        if message.startswith("preflop"):
            self._parse_preflop(message, state)
        elif message.startswith("flop"):
            self._parse_flop(message, state)
        elif message.startswith("turn"):
            self._parse_turn(message, state)
        elif message.startswith("river"):
            self._parse_river(message, state)
        elif message.split(maxsplit=1)[0].lower() in _ACTION_TYPES:
            self.record_action(message, state, is_opponent=True)
        else:
            raise ValueError("unknown protocol message: %s" % message)
        return state

    def record_action(
        self,
        message: str,
        state: GameState,
        is_opponent: bool,
    ) -> None:
        """把一次已确认发生的动作写入筹码、底池、历史和 to_call。

        协议中的 ``bet N`` / ``raise N`` 按项目约定表示“本次额外投入 N”，
        与 C++ SelfPlayEnv 内部使用的“本街加到 N”语义不同，调用时不要混用。
        """
        action_name, declared_amount = parse_action_string(message)
        if action_name not in _ACTION_TYPES:
            raise ValueError("unknown poker action: %s" % action_name)
        if declared_amount < 0:
            raise ValueError("action amount cannot be negative")

        action = _ACTION_TYPES[action_name]
        chips = state.opponent_chips if is_opponent else state.my_chips
        own_commit = (
            self._opponent_street_commit
            if is_opponent
            else self._hero_street_commit
        )
        other_commit = (
            self._hero_street_commit
            if is_opponent
            else self._opponent_street_commit
        )

        if action in (ActionType.FOLD, ActionType.CHECK):
            contribution = 0
        elif action == ActionType.CALL:
            # 跟注只补齐双方本街投入差，短码时最多投入剩余全部筹码。
            contribution = min(max(0, other_commit - own_commit), chips)
        elif action in (ActionType.BET, ActionType.RAISE):
            if declared_amount <= 0:
                raise ValueError("%s requires a positive amount" % action_name)
            contribution = min(declared_amount, chips)
        else:  # ALLIN
            contribution = chips

        if is_opponent:
            state.opponent_chips -= contribution
            self._opponent_street_commit += contribution
        else:
            state.my_chips -= contribution
            self._hero_street_commit += contribution

        state.pot += contribution
        # pybind11 的 vector 属性读取后是 Python 副本，需要整体写回才能更新 C++。
        history = list(state.history)
        history.append(
            ActionRecord(state.stage, action, contribution, is_opponent)
        )
        state.history = history
        state.to_call = max(
            # to_call 始终站在英雄视角：对手本街投入减英雄本街投入。
            0, self._opponent_street_commit - self._hero_street_commit
        )

    def _parse_preflop(self, message: str, state: GameState) -> None:
        parts = message.split("|")
        if len(parts) < 3:
            raise ValueError("invalid preflop message: %s" % message)

        positions = {
            "BIGBLIND": Position.BB,
            "SMALLBLIND": Position.SB,
        }
        position_name = parts[1].strip().upper()
        if position_name not in positions:
            raise ValueError("unknown position: %s" % position_name)

        cards = CardUtils.parse_protocol_cards(message)
        if len(cards) != 2:
            raise ValueError("preflop message must contain exactly two cards")

        state.history = []
        state.num_community = 0
        state.my_cards = cards
        state.my_position = positions[position_name]
        state.stage = Stage.PREFLOP
        state.my_chips = INITIAL_CHIPS
        state.opponent_chips = INITIAL_CHIPS
        state.pot = SMALL_BLIND + BIG_BLIND

        # 平台会告知英雄座位；据此扣除双方盲注并建立翻前投入基线。
        if state.my_position == Position.SB:
            state.my_chips -= SMALL_BLIND
            state.opponent_chips -= BIG_BLIND
            self._hero_street_commit = SMALL_BLIND
            self._opponent_street_commit = BIG_BLIND
        else:
            state.my_chips -= BIG_BLIND
            state.opponent_chips -= SMALL_BLIND
            self._hero_street_commit = BIG_BLIND
            self._opponent_street_commit = SMALL_BLIND
        state.to_call = max(
            0, self._opponent_street_commit - self._hero_street_commit
        )

    def _start_street(self, state: GameState, stage: Stage) -> None:
        # 每条街都是独立下注轮，之前街道的投入已经包含在 pot 中。
        state.stage = stage
        state.to_call = 0
        self._hero_street_commit = 0
        self._opponent_street_commit = 0

    def _parse_flop(self, message: str, state: GameState) -> None:
        cards = CardUtils.parse_protocol_cards(message)
        if len(cards) != 3:
            raise ValueError("flop message must contain exactly three cards")
        board = list(state.community_cards)
        # community_cards 在 C++ 中是固定长度 array，必须保留 5 个槽位整体写回。
        board[:3] = cards
        state.community_cards = board
        state.num_community = 3
        self._start_street(state, Stage.FLOP)

    def _parse_turn(self, message: str, state: GameState) -> None:
        self._append_board_card(message, state, expected_count=3)
        self._start_street(state, Stage.TURN)

    def _parse_river(self, message: str, state: GameState) -> None:
        self._append_board_card(message, state, expected_count=4)
        self._start_street(state, Stage.RIVER)

    @staticmethod
    def _append_board_card(
        message: str,
        state: GameState,
        expected_count: int,
    ) -> None:
        cards = CardUtils.parse_protocol_cards(message)
        # 强制按 flop→turn→river 顺序到达，避免重复消息悄悄覆盖牌面。
        if len(cards) != 1 or state.num_community != expected_count:
            raise ValueError("community cards arrived out of order")
        board = list(state.community_cards)
        board[expected_count] = cards[0]
        state.community_cards = board
        state.num_community = expected_count + 1
