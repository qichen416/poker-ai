from engine.parser import ProtocolParser
from poker_core import ActionType, GameState, Position, Stage


def test_preflop_and_opponent_raise_update_complete_state():
    # 覆盖盲注初始化、英雄跟注、对手加注及两条动作历史的连续同步。
    parser = ProtocolParser()
    state = parser.parse(
        "preflop|SMALLBLIND|<0,12><1,11>",
        GameState(),
    )

    assert state.my_position == Position.SB
    assert state.stage == Stage.PREFLOP
    assert state.my_chips == 19950
    assert state.opponent_chips == 19900
    assert state.pot == 150
    assert state.to_call == 50

    parser.record_action("call", state, is_opponent=False)
    parser.parse("raise 200", state)

    assert state.my_chips == 19900
    assert state.opponent_chips == 19700
    assert state.pot == 400
    assert state.to_call == 200
    assert len(state.history) == 2
    assert state.history[0].action == ActionType.CALL
    assert not state.history[0].is_opponent
    assert state.history[1].action == ActionType.RAISE
    assert state.history[1].amount == 200
    assert state.history[1].is_opponent


def test_street_transition_resets_commitments_and_tracks_bet_call():
    # 翻牌后本街投入必须归零，新的 bet/call 只影响翻牌圈差额。
    parser = ProtocolParser()
    state = parser.parse(
        "preflop|SMALLBLIND|<0,12><1,11>",
        GameState(),
    )
    parser.record_action("call", state, is_opponent=False)
    parser.parse("check", state)

    parser.parse("flop|<2,3><3,4><1,5>", state)
    assert state.stage == Stage.FLOP
    assert state.num_community == 3
    assert state.to_call == 0

    parser.parse("check", state)
    parser.record_action("bet 300", state, is_opponent=False)
    parser.parse("call", state)

    assert state.my_chips == 19600
    assert state.opponent_chips == 19600
    assert state.pot == 800
    assert state.to_call == 0
    assert [record.amount for record in state.history] == [50, 0, 0, 300, 300]
