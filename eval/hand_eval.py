from poker_core import HandEvaluator as CppHandEvaluator

class HandEvaluator:
    def __init__(self):
        self._eval = CppHandEvaluator()
    def evaluate_7cards(self, cards):
        return self._eval.evaluate_7cards(cards)
