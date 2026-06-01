import torch
import torch.nn as nn

class PokerPolicy(nn.Module):
    def __init__(self, action_dim: int = 6):
        super().__init__()
        self.action_dim = action_dim
        self.card_cnn = nn.Sequential(
            nn.Conv2d(4, 32, kernel_size=(3, 2), padding=(1, 0)),
            nn.ReLU(),
            nn.Conv2d(32, 64, kernel_size=(3, 2), padding=(1, 0)),
            nn.ReLU(),
            nn.Flatten(),
        )
        self.action_gru = nn.GRU(input_size=10, hidden_size=64, num_layers=1, batch_first=True)
        card_feat_dim = 64 * 13 * 2
        action_feat_dim = 64
        scalar_dim = 4
        fused_dim = card_feat_dim + action_feat_dim + scalar_dim
        self.fc = nn.Sequential(
            nn.Linear(fused_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
        )
        self.policy_head = nn.Linear(128, action_dim)
        self.value_head = nn.Linear(128, 1)

    def forward(self, card_tensor, action_history, scalar_features):
        card_feat = self.card_cnn(card_tensor)
        _, hidden = self.action_gru(action_history)
        action_feat = hidden.squeeze(0)
        fused = torch.cat([card_feat, action_feat, scalar_features], dim=-1)
        x = self.fc(fused)
        policy_logits = self.policy_head(x)
        value = self.value_head(x)
        return policy_logits, value

    def count_parameters(self):
        return sum(p.numel() for p in self.parameters() if p.requires_grad)
