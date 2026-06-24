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
    assert state.pot == 150
    assert state.to_call == 50
    assert env.is_action_legal(ActionType.CALL)
    assert not env.is_action_legal(ActionType.CHECK)

def test_environment_can_finish_a_hand():
    env = SelfPlayEnv()
    env.set_opponent_policy(lambda state: ActionType.CHECK)
    env.reset(seed=42)
    env.step(ActionType.CALL, 0)
    env.step(ActionType.CHECK, 0)
    env.step(ActionType.CHECK, 0)
    result = env.step(ActionType.CHECK, 0)
    assert result.done
    assert result.state.stage == Stage.SHOWDOWN
    assert result.state.my_chips + result.state.opponent_chips == 40000

def test_forced_board_tie_splits_equity():
    calc = WinRateCalculator()
    hole = [Card(1, 0), Card(2, 1)]
    board = [
        Card(0, 12), Card(0, 11), Card(0, 10), Card(0, 9), Card(0, 8)
    ]
    assert calc.calculate(hole, board, 1, 20) == pytest.approx(0.5)
