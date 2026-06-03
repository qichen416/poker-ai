import re
from poker_core import GameState, Card, CardUtils, Stage, Position, ActionType

class ProtocolParser:
    """
    协议解析器
    负责将服务器发送的文本协议消息解析为结构化的游戏状态对象。
    支持对 preflop / flop / turn / river 阶段消息以及对手动作消息的解析。
    """

    def parse(self, message: str, current_state: GameState = None) -> GameState:
        """
        解析一条消息并更新（或创建）游戏状态。

        :param message: 服务器发来的单条协议字符串
        :param current_state: 当前游戏状态，若为 None 则创建新状态
        :return: 更新后的 GameState 对象
        """
        # 如果没有传入当前状态，则创建一个新的空状态
        state = current_state or GameState()

        # 根据消息前缀判断阶段或动作类型
        if message.startswith('preflop'):
            self._parse_preflop(message, state)
        elif message.startswith('flop'):
            self._parse_flop(message, state)
        elif message.startswith('turn'):
            self._parse_turn(message, state)
        elif message.startswith('river'):
            self._parse_river(message, state)
        elif any(message.startswith(a.value) for a in ActionType):
            # 如果消息以某个动作类型（如 fold, call, raise 等）开头
            # 则视为对手动作，调用动作解析函数
            self._parse_action(message, state, player='opponent')
        # 其他未知消息不修改状态，直接返回
        return state

    def _parse_preflop(self, msg: str, state: GameState):
        """
        解析 preflop 阶段消息。
        消息格式示例: "preflop|BIGBLIND|AsKh" 或 "preflop|SMALLBLIND|2c3c"

        :param msg: 原始消息字符串
        :param state: 待更新的游戏状态
        """
        # 用竖线分割消息段
        parts = msg.split('|')
        # 如果有位置信息（第二段）
        if len(parts) >= 2:
            pos_str = parts[1].strip()
            # 如果该字符串是有效的 Position 枚举值，则设置我方位置
            if pos_str in [p.value for p in Position]:
                state.my_position = Position(pos_str)

        # 使用卡牌工具类解析整条消息中的卡牌
        cards = CardUtils.parse_protocol_cards(msg)
        # 若解析出至少2张牌，设为我的手牌
        if len(cards) >= 2:
            state.my_cards = [cards[0], cards[1]]

        # 设置当前游戏阶段为 PREFLOP
        state.stage = Stage.PREFLOP

    def _parse_flop(self, msg: str, state: GameState):
        """
        解析 flop 阶段消息。
        消息格式示例: "flop|AcKdQh"

        :param msg: 原始消息字符串
        :param state: 待更新的游戏状态
        """
        cards = CardUtils.parse_protocol_cards(msg)
        # 至少需要3张牌才构成有效翻牌
        if len(cards) >= 3:
            # 将前3张作为公共牌
            state.community_cards = [cards[0], cards[1], cards[2]]
            state.num_community = 3

        # 更新阶段为 FLOP
        state.stage = Stage.FLOP

    def _parse_turn(self, msg: str, state: GameState):
        """
        解析 turn 阶段消息。
        消息格式示例: "turn|2h"

        :param msg: 原始消息字符串
        :param state: 待更新的游戏状态
        """
        cards = CardUtils.parse_protocol_cards(msg)
        if cards:
            # 公共牌数组已经预留了5个位置，按 num_community 索引填入新牌
            state.community_cards[state.num_community] = cards[0]
            state.num_community += 1

        # 更新阶段为 TURN
        state.stage = Stage.TURN

    def _parse_river(self, msg: str, state: GameState):
        """
        解析 river 阶段消息。
        消息格式示例: "river|8s"

        :param msg: 原始消息字符串
        :param state: 待更新的游戏状态
        """
        cards = CardUtils.parse_protocol_cards(msg)
        if cards:
            # 类似 Turn 的处理，将最后一张公共牌填入
            state.community_cards[state.num_community] = cards[0]
            state.num_community += 1

        # 更新阶段为 RIVER
        state.stage = Stage.RIVER

    def _parse_action(self, msg: str, state: GameState, player: str):
        """
        解析对手动作消息。
        消息格式示例: "raise 200" 或 "fold"

        :param msg: 原始动作字符串
        :param state: 当前游戏状态，用于记录历史
        :param player: 动作来源（如 'opponent' 表示对手）
        """
        # 按空格分割，第一段是动作类型，第二段（可选）是下注/加注额度
        parts = msg.split()
        action_str = parts[0].lower()
        amount = int(parts[1]) if len(parts) > 1 else 0

        # 动作字符串到 ActionType 枚举的映射
        action_map = {
            'fold': ActionType.FOLD,
            'check': ActionType.CHECK,
            'call': ActionType.CALL,
            'bet': ActionType.BET,
            'raise': ActionType.RAISE,
            'allin': ActionType.ALLIN
        }
        # 获取动作枚举，未知动作默认为 FOLD
        action = action_map.get(action_str, ActionType.FOLD)

        # 将动作记录追加到历史记录中
        # 注意：这里使用了 ActionRecord 类，但代码中尚未定义该类的实现。
        # state.history.append(ActionRecord(state.stage, action, amount, player == 'opponent'))