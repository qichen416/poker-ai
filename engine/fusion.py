import time
import random
from typing import Dict
from poker_core import GameState, CFREngine, WinRateCalculator, ActionType, Stage, Position
from shared.constants import DECISION_TIMEOUT
from drl import DRLInferencer
from eval.opponent_model import OpponentModel

class DecisionEngine:
    def __init__(self):
        self.cfr_engine = CFREngine()
        self.win_calc = WinRateCalculator()
        self.drl = DRLInferencer()
        self.opponent = OpponentModel()
        self.alpha = 0.5

    def make_decision(self, game_state: GameState) -> str:
        start_time = time.time()
        if game_state.stage == Stage.PREFLOP:
            action = self._preflop_decision(game_state)
        else:
            action = self._postflop_decision(game_state)
        elapsed = time.time() - start_time
        if elapsed > DECISION_TIMEOUT * 0.9:
            print("WARNING: timeout (%.2fs), fallback to call" % elapsed)
            return "call"
        return action

    def _preflop_decision(self, state: GameState) -> str:
        key = self.cfr_engine.make_info_set_key(state)
        probs = self.cfr_engine.get_average_strategy(key)
        return self._sample_action(probs, state)

    def _postflop_decision(self, state: GameState) -> str:
        key = self.cfr_engine.make_info_set_key(state)
        cfr_probs = self.cfr_engine.get_average_strategy(key)
        drl_probs = self.drl.get_action_probs_c(state)
        merged = self._merge_probs(cfr_probs, drl_probs, self.alpha)
        return self._sample_action(merged, state)

    def _merge_probs(self, cfr, drl, alpha):
        actions = ["fold", "check", "call", "bet", "raise", "allin"]
        merged = {}
        for i, a in enumerate(actions):
            c = cfr[i] if isinstance(cfr, (list, tuple)) else cfr.get(a, 0.0)
            d = drl.get(a, 0.0) if isinstance(drl, dict) else drl[i]
            merged[a] = (1 - alpha) * c + alpha * d
        total = sum(merged.values())
        return {k: v/total for k, v in merged.items()} if total > 0 else merged

    def _sample_action(self, probs, state: GameState) -> str:
        actions = list(probs.keys())
        weights = [probs[a] for a in actions]
        action = random.choices(actions, weights=weights, k=1)[0]
        if action in ['raise', 'bet']:
            amount = max(state.pot, 100)
            return "%s %d" % (action, amount)
        return action
