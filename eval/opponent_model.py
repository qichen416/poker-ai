from typing import Dict, Optional
from poker_core import GameState, Stage, ActionType

class OpponentModel:
    def __init__(self):
        self.stats = {
            'hands_seen': 0,
            'vpip_actions': [],
            'pfr_actions': [],
            'aggression': {'raise': 0, 'call': 0, 'check': 0},
        }

    def update(self, action: ActionType, amount: int, stage: Stage):
        if stage == Stage.PREFLOP:
            self.stats['hands_seen'] += 1
            if action in [ActionType.CALL, ActionType.RAISE, ActionType.BET, ActionType.ALLIN]:
                self.stats['vpip_actions'].append(1)
            else:
                self.stats['vpip_actions'].append(0)
            if action == ActionType.RAISE:
                self.stats['pfr_actions'].append(1)
            else:
                self.stats['pfr_actions'].append(0)
        if action == ActionType.RAISE:
            self.stats['aggression']['raise'] += 1
        elif action == ActionType.CALL:
            self.stats['aggression']['call'] += 1
        elif action == ActionType.CHECK:
            self.stats['aggression']['check'] += 1

    def get_profile(self) -> Dict[str, Optional[float]]:
        n = max(self.stats['hands_seen'], 1)
        vpip = sum(self.stats['vpip_actions']) / n if self.stats['vpip_actions'] else None
        pfr = sum(self.stats['pfr_actions']) / n if self.stats['pfr_actions'] else None
        agg = self.stats['aggression']
        af = agg['raise'] / max(agg['call'], 1) if agg['call'] > 0 else None
        return {'vpip': vpip, 'pfr': pfr, 'af': af}

    def get_exploit_offset(self, state: GameState) -> Dict[str, float]:
        profile = self.get_profile()
        vpip = profile.get('vpip')
        af = profile.get('af')
        offset = {'fold': 0.0, 'check': 0.0, 'call': 0.0, 'raise': 0.0, 'bet': 0.0, 'allin': 0.0}
        if vpip is None:
            return offset
        if vpip < 0.20:
            offset['raise'] += 0.10
            offset['fold'] -= 0.05
        elif vpip > 0.40:
            offset['raise'] -= 0.05
            offset['call'] += 0.10
        if af and af > 3.0:
            offset['call'] += 0.10
            offset['fold'] -= 0.10
        for k in offset:
            offset[k] = max(-0.15, min(0.15, offset[k]))
        return offset
