from engine.client import PokerClient
from engine.main import run_game
from engine.parser import ProtocolParser
from poker_core import ActionType, Stage
from tests.mock_server import MockServer


def test_mock_server_runs_a_complete_stateful_protocol_game():
    # 使用真实本地 socket 跑完 raise/bet/call/check 序列，验证整条通信链路。
    server = MockServer(port=0)
    thread = server.serve_in_thread(max_games=1)
    client = PokerClient(
        *server.address,
        timeout=2,
        reconnect_attempts=1,
        auto_reconnect=False,
    )
    assert client.connect()

    def decide(state):
        return "call" if state.to_call > 0 else "check"

    try:
        state = run_game(client, ProtocolParser(), decide)
    finally:
        client.close()
        thread.join(timeout=5)
        server.stop()

    assert server.error is None
    assert server.actions == ["call", "call", "check", "call", "check", "check"]
    assert state.stage == Stage.RIVER
    assert state.num_community == 5
    assert state.my_chips == 19600
    assert state.opponent_chips == 19600
    assert state.pot == 800
    assert state.to_call == 0
    assert len(state.history) == 8
    assert [record.action for record in state.history].count(ActionType.CALL) == 3
