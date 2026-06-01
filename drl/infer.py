import torch
import numpy as np
from typing import Dict
from poker_core import GameState, Stage
from .network import PokerPolicy

class DRLInferencer:
    def __init__(self, model_path: str = "data/drl_checkpoints/best_model.pt"):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.policy = PokerPolicy(action_dim=6).to(self.device)
        import os
        if os.path.exists(model_path):
            self.policy.load_state_dict(torch.load(model_path, map_location=self.device))
            print("Loaded DRL model from %s" % model_path)
        self.policy.eval()

    def get_action_probs(self, game_state: GameState) -> Dict[str, float]:
        return {
            'fold': 0.15, 'check': 0.15, 'call': 0.25,
            'raise': 0.25, 'bet': 0.10, 'allin': 0.10
        }

    def get_action_probs_c(self, c_state):
        return self.get_action_probs(c_state)
