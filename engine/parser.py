import re
from poker_core import GameState, Card, CardUtils, Stage, Position, ActionType

class ProtocolParser:
    def parse(self, message: str, current_state: GameState = None) -> GameState:
        state = current_state or GameState()
        if message.startswith('preflop'):
            self._parse_preflop(message, state)
        elif message.startswith('flop'):
            self._parse_flop(message, state)
        elif message.startswith('turn'):
            self._parse_turn(message, state)
        elif message.startswith('river'):
            self._parse_river(message, state)
        elif any(message.startswith(a.value) for a in ActionType):
            self._parse_action(message, state, player='opponent')
        return state

    def _parse_preflop(self, msg: str, state: GameState):
        parts = msg.split('|')
        if len(parts) >= 2:
            pos_str = parts[1].strip()
            if pos_str in [p.value for p in Position]:
                state.my_position = Position(pos_str)
        cards = CardUtils.parse_protocol_cards(msg)
        if len(cards) >= 2:
            state.my_cards = [cards[0], cards[1]]
        state.stage = Stage.PREFLOP

    def _parse_flop(self, msg: str, state: GameState):
        cards = CardUtils.parse_protocol_cards(msg)
        if len(cards) >= 3:
            state.community_cards = [cards[0], cards[1], cards[2]]
            state.num_community = 3
        state.stage = Stage.FLOP

    def _parse_turn(self, msg: str, state: GameState):
        cards = CardUtils.parse_protocol_cards(msg)
        if cards:
            state.community_cards[state.num_community] = cards[0]
            state.num_community += 1
        state.stage = Stage.TURN

    def _parse_river(self, msg: str, state: GameState):
        cards = CardUtils.parse_protocol_cards(msg)
        if cards:
            state.community_cards[state.num_community] = cards[0]
            state.num_community += 1
        state.stage = Stage.RIVER

    def _parse_action(self, msg: str, state: GameState, player: str):
        parts = msg.split()
        action_str = parts[0].lower()
        amount = int(parts[1]) if len(parts) > 1 else 0
        action_map = {
            'fold': ActionType.FOLD, 'check': ActionType.CHECK,
            'call': ActionType.CALL, 'bet': ActionType.BET,
            'raise': ActionType.RAISE, 'allin': ActionType.ALLIN
        }
        action = action_map.get(action_str, ActionType.FOLD)
        state.history.append(ActionRecord(state.stage, action, amount, player == 'opponent'))
