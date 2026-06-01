import math
from typing import Dict

class EloRating:
    def __init__(self, k: int = 32, initial: int = 1500):
        self.k = k
        self.ratings: Dict[str, int] = {}
        self.initial = initial

    def get_rating(self, player: str) -> int:
        return self.ratings.get(player, self.initial)

    def update(self, winner: str, loser: str, draw: bool = False):
        r1 = self.get_rating(winner)
        r2 = self.get_rating(loser)
        e1 = 1 / (1 + math.pow(10, (r2 - r1) / 400))
        e2 = 1 / (1 + math.pow(10, (r1 - r2) / 400))
        if draw:
            s1, s2 = 0.5, 0.5
        else:
            s1, s2 = 1.0, 0.0
        self.ratings[winner] = int(r1 + self.k * (s1 - e1))
        self.ratings[loser] = int(r2 + self.k * (s2 - e2))

    def get_leaderboard(self) -> list:
        return sorted(self.ratings.items(), key=lambda x: x[1], reverse=True)
