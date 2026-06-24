import socket
import threading

from engine.client import PokerClient


class ChunkSocket:
    """用预切分字节块模拟 TCP 半包与粘包。"""
    def __init__(self, chunks):
        self.chunks = list(chunks)

    def recv(self, _size):
        return self.chunks.pop(0)

    def close(self):
        pass


def test_receive_reassembles_partial_frame_and_preserves_sticky_frame():
    # 第一条消息跨两个 recv，第二条消息与第一条尾部粘在同一个 recv 中。
    client = PokerClient("localhost", 10002)
    client.socket = ChunkSocket(
        [
            b"preflop|SMALL",
            b"BLIND|<0,12><1,11>\nflop|<2,3><3,4><1,5>\n",
        ]
    )

    assert client.receive() == "preflop|SMALLBLIND|<0,12><1,11>"
    assert client.receive() == "flop|<2,3><3,4><1,5>"


def test_receive_reconnects_after_server_disconnect():
    # 首次连接立即断开，第二次连接发送有效帧，验证真实 socket 重连路径。
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen(2)
    host, port = listener.getsockname()

    def serve_two_connections():
        first, _ = listener.accept()
        first.close()
        second, _ = listener.accept()
        second.sendall(b"gameover\n")
        second.close()
        listener.close()

    thread = threading.Thread(target=serve_two_connections, daemon=True)
    thread.start()
    client = PokerClient(
        host,
        port,
        timeout=2,
        reconnect_attempts=5,
        reconnect_delay=0.01,
    )

    try:
        assert client.connect()
        assert client.receive() == "gameover"
    finally:
        client.close()
        thread.join(timeout=5)
