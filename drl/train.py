import argparse
import yaml
import torch
from .network import PokerPolicy
from poker_core import SelfPlayEnv

def train(config: dict):
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print("Training on %s" % device)
    policy = PokerPolicy(action_dim=6).to(device)
    print("Model parameters: %d" % policy.count_parameters())
    env = SelfPlayEnv()
    print("Training stub - implement PPO loop")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', default='configs/drl.yaml')
    args = parser.parse_args()
    with open(args.config, 'r') as f:
        config = yaml.safe_load(f)
    train(config)
