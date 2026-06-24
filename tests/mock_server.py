import random
import socket
import threading
from typing import List, Optional, Tuple

from poker_core import CardUtils


class MockServer:
    """可重复、可停止的比赛协议测试服务器。

    固定随机种子保证发牌可复现；记录客户端动作并严格校验顺序，使集成测试
    同时覆盖 TCP 分帧、协议解析、状态更新和主循环发送行为。
    """

    def __init__(self, host: str = "127.0.0.1", port: int = 10002, seed: int = 7):
        self.host = host
        self.port = port
        self.seed = seed
        self.actions: List[str] = []
        self.error: Optional[BaseException] = None
        self._server_socket: Optional[socket.socket] = None
        self._ready = threading.Event()
        self._stop = threading.Event()

    @property
    def address(self) -> Tuple[str, int]:
        return self.host, self.port

    def serve_in_thread(self, max_games: int = 1) -> threading.Thread:
        """后台启动服务器，并等待 bind/listen 完成后再把端口交给客户端。"""
        thread = threading.Thread(
            target=self.start,
            kwargs={"max_games": max_games},
            daemon=True,
        )
        thread.start()
        if not self._ready.wait(timeout=5):
            raise RuntimeError("mock server did not start")
        return thread

    def start(self, max_games: Optional[int] = None) -> None:
        games = 0
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_socket = sock
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self.host, self.port))
        # port=0 时由操作系统选择空闲端口，可避免并行测试端口冲突。
        self.port = sock.getsockname()[1]
        sock.listen(5)
        sock.settimeout(0.2)
        self._ready.set()

        try:
            while not self._stop.is_set() and (
                max_games is None or games < max_games
            ):
                try:
                    client, _address = sock.accept()
                except socket.timeout:
                    continue
                with client:
                    client.settimeout(5)
                    self._run_game(client, games)
                games += 1
        except BaseException as error:
            self.error = error
        finally:
            sock.close()
            self._server_socket = None

    def stop(self) -> None:
        self._stop.set()
        if self._server_socket is not None:
            try:
                self._server_socket.close()
            except OSError:
                pass

    def _run_game(self, client: socket.socket, game_number: int) -> None:
        deck = list(CardUtils.create_deck())
        random.Random(self.seed + game_number).shuffle(deck)
        hole = deck[:2]
        board = deck[4:9]
        receive_buffer = bytearray()

        # 每个元组是“服务端消息 → 期望客户端动作”，包含跨街下注和跟注。
        script = [
            (
                "preflop|SMALLBLIND|%s" % CardUtils.cards_to_protocol(hole),
                "call",
            ),
            ("raise 200", "call"),
            (
                "flop|%s" % CardUtils.cards_to_protocol(board[:3]),
                "check",
            ),
            ("bet 100", "call"),
            ("turn|%s" % board[3].to_protocol(), "check"),
            ("river|%s" % board[4].to_protocol(), "check"),
        ]

        for message, expected_action in script:
            self._send_line(client, message)
            action = self._receive_line(client, receive_buffer)
            self.actions.append(action)
            if action.split(maxsplit=1)[0] != expected_action:
                raise AssertionError(
                    "expected %s, received %s" % (expected_action, action)
                )
        self._send_line(client, "gameover")

    @staticmethod
    def _send_line(client: socket.socket, message: str) -> None:
        client.sendall(("%s\n" % message).encode("utf-8"))

    @staticmethod
    def _receive_line(client: socket.socket, buffer: bytearray) -> str:
        # Mock 端也按字节流处理，不能假设一次 recv 恰好得到一条动作。
        while b"\n" not in buffer:
            chunk = client.recv(1024)
            if not chunk:
                raise ConnectionError("client disconnected before sending action")
            buffer.extend(chunk)
        newline = buffer.index(b"\n")
        frame = bytes(buffer[:newline])
        del buffer[: newline + 1]
        return frame.rstrip(b"\r").decode("utf-8")


if __name__ == "__main__":
    MockServer().start()
