import pytest
from poker_core import (
    Card, create_deck, parse_protocol_cards,
    HandEvaluator, GameState, CFREngine,
    WinRateCalculator, SelfPlayEnv,
    Stage, Position, ActionType
)

def test_card_basic():
    c = Card(0, 12)
    assert c.suit == 0
    assert c.rank == 12
    assert c.to_string() == "Ah"
    assert c.to_protocol() == "<0,12>"

def test_parse_protocol():
    cards = parse_protocol_cards("<2,3><3,4>")
    assert len(cards) == 2
    assert cards[0] == Card(2, 3)
    assert cards[1] == Card(3, 4)

def test_deck():
    deck = create_deck()
    assert len(deck) == 52

def test_hand_evaluator():
    eval = HandEvaluator()
    hand = [
        Card(0, 12), Card(1, 11),
        Card(2, 5), Card(3, 3), Card(0, 1), Card(1, 0), Card(2, 9)
    ]
    res = eval.evaluate_7cards(hand)
    assert res.rank >= 1

def test_cfr_engine():
    engine = CFREngine()
    state = GameState()
    key = engine.make_info_set_key(state)
    strat = engine.get_strategy(key)
    assert len(strat) == 6
    assert abs(sum(strat) - 1.0) < 1e-6

def test_win_rate():
    calc = WinRateCalculator()
    my = [Card(0, 12), Card(1, 12)]
    comm = []
    rate = calc.calculate(my, comm, num_opponents=1, n_simulations=100)
    assert 0.0 <= rate <= 1.0

def test_environment():
    env = SelfPlayEnv()
    state = env.reset(seed=42)
    assert state.my_cards[0].index() != state.my_cards[1].index()
