import socket
import time
from typing import Optional


class PokerClient:
    """使用换行分帧的可靠 TCP 客户端。

    TCP 是字节流，一次 recv 可能只含半条消息，也可能粘连多条消息，因此
    `_receive_buffer` 会保留尚未消费的字节，receive() 每次只返回一帧。
    """

    def __init__(
        self,
        host: str,
        port: int,
        timeout: float = 10.0,
        reconnect_attempts: int = 3,
        reconnect_delay: float = 0.2,
        auto_reconnect: bool = True,
    ):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.reconnect_attempts = max(1, reconnect_attempts)
        self.reconnect_delay = max(0.0, reconnect_delay)
        self.auto_reconnect = auto_reconnect
        self.socket: Optional[socket.socket] = None
        # 跨 recv 调用保存半包，同时保留粘包中的后续完整帧。
        self._receive_buffer = bytearray()
        # 用户主动 close 后禁止自动重连，避免程序退出时重新建立连接。
        self._closing = False

    @property
    def connected(self) -> bool:
        return self.socket is not None

    def connect(self) -> bool:
        self._closing = False
        for attempt in range(self.reconnect_attempts):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(self.timeout)
                sock.connect((self.host, self.port))
                self.socket = sock
                # 新连接不能继承旧连接尚未完成的半条协议消息。
                self._receive_buffer.clear()
                return True
            except OSError:
                try:
                    sock.close()
                except UnboundLocalError:
                    pass
                self.socket = None
                if attempt + 1 < self.reconnect_attempts:
                    time.sleep(self.reconnect_delay)
        return False

    def reconnect(self) -> bool:
        """关闭失效 socket，并按 connect() 的重试策略建立新连接。"""
        self._close_socket()
        if self._closing:
            return False
        return self.connect()

    def receive(self) -> Optional[str]:
        """Return exactly one UTF-8 line, regardless of TCP packet boundaries."""
        while True:
            # 优先消费缓冲区；处理粘包时无需再次访问网络。
            newline = self._receive_buffer.find(b"\n")
            if newline >= 0:
                frame = bytes(self._receive_buffer[:newline])
                del self._receive_buffer[: newline + 1]
                return frame.rstrip(b"\r").decode("utf-8")

            if self.socket is None:
                if not self.auto_reconnect or not self.reconnect():
                    return None

            try:
                chunk = self.socket.recv(4096)
                if not chunk:
                    # recv 返回空字节表示对端已正常关闭连接。
                    if not self.auto_reconnect or not self.reconnect():
                        return None
                    continue
                self._receive_buffer.extend(chunk)
                # 比赛协议消息应很短；限制帧长可防止异常服务端无限占用内存。
                if len(self._receive_buffer) > 65536:
                    raise ValueError("protocol frame exceeds 64 KiB")
            except socket.timeout:
                return None
            except (OSError, UnicodeError):
                if not self.auto_reconnect or not self.reconnect():
                    return None

    def send(self, message: str) -> bool:
        """Send one complete protocol frame.

        发送失败后不会自动重放动作：服务端可能已经收到第一次下注，自动重发
        会造成重复行动。上层必须根据平台的重连/局面恢复协议决定如何继续。
        """
        if self.socket is None:
            return False
        try:
            self.socket.sendall(("%s\n" % message).encode("utf-8"))
            return True
        except OSError:
            self._close_socket()
            return False

    def close(self) -> None:
        """用户主动关闭连接并清除所有未完成帧。"""
        self._closing = True
        self._close_socket()
        self._receive_buffer.clear()

    def _close_socket(self) -> None:
        if self.socket is not None:
            try:
                self.socket.close()
            finally:
                self.socket = None
