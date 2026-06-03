import socket
from typing import Optional
from poker_core import GameState        # 游戏状态核心类（当前未直接使用，可能由调用方使用）
from .parser import ProtocolParser      # 自定义协议解析器

class PokerClient:
    """
    扑克游戏 TCP 客户端。

    负责与扑克游戏服务器建立连接、发送指令、接收响应，
    并利用 ProtocolParser 解析服务器返回的协议消息。
    """

    def __init__(self, host: str, port: int, timeout: float = 10.0):
        """
        初始化客户端实例。

        :param host: 服务器主机名或 IP 地址
        :param port: 服务器端口号
        :param timeout: 套接字操作超时时间（秒），默认 10.0 秒
        """
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None   # TCP 套接字，连接后赋值
        self.parser = ProtocolParser()                # 协议解析器，用于处理接收到的数据
        self.timeout = timeout

    def connect(self) -> bool:
        """
        建立与服务器的 TCP 连接。

        :return: 连接成功返回 True，否则返回 False
        """
        try:
            # 创建 TCP 套接字
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # 设置超时，避免 recv 等操作无限阻塞
            self.socket.settimeout(self.timeout)
            # 连接服务器
            self.socket.connect((self.host, self.port))
            print("Connected to %s:%d" % (self.host, self.port))
            return True
        except Exception as e:
            print("Connection failed: %s" % e)
            return False

    def receive(self) -> Optional[str]:
        """
        从服务器接收一行数据（以换行符分隔的消息）。

        该方法会阻塞直到接收到数据、超时或发生错误。
        注意：返回的字符串已去除首尾空白（包括换行符）。

        :return: 接收到的消息字符串；超时或连接关闭时返回 None；出错时也返回 None
        """
        try:
            # 接收最多 4096 字节，解码为 UTF-8 字符串并去除首尾空白（包括换行）
            data = self.socket.recv(4096).decode('utf-8').strip()
            if not data:
                # 收到空数据表示连接被对端关闭
                return None
            return data
        except socket.timeout:
            # 超时没有数据可读，返回 None（调用方可据此判断是否需要重试）
            return None
        except Exception as e:
            print("Receive error: %s" % e)
            return None

    def send(self, message: str) -> bool:
        """
        向服务器发送一条消息，自动附加换行符作为消息结束标志。

        :param message: 待发送的消息内容（不含换行符）
        :return: 发送成功返回 True，失败返回 False
        """
        try:
            # 确保消息以换行符结尾，然后编码为 UTF-8 并发送
            self.socket.sendall(("%s\n" % message).encode('utf-8'))
            return True
        except Exception as e:
            print("Send error: %s" % e)
            return False

    def close(self):
        """
        关闭与服务器的连接，释放套接字资源。

        如果已经处于关闭状态，则不做任何操作。
        """
        if self.socket:
            self.socket.close()
            self.socket = None