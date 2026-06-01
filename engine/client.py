import socket
from typing import Optional
from poker_core import GameState
from .parser import ProtocolParser

class PokerClient:
    def __init__(self, host: str, port: int, timeout: float = 10.0):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.parser = ProtocolParser()
        self.timeout = timeout

    def connect(self) -> bool:
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.host, self.port))
            print("Connected to %s:%d" % (self.host, self.port))
            return True
        except Exception as e:
            print("Connection failed: %s" % e)
            return False

    def receive(self) -> Optional[str]:
        try:
            data = self.socket.recv(4096).decode('utf-8').strip()
            if not data:
                return None
            return data
        except socket.timeout:
            return None
        except Exception as e:
            print("Receive error: %s" % e)
            return None

    def send(self, message: str) -> bool:
        try:
            self.socket.sendall(("%s\n" % message).encode('utf-8'))
            return True
        except Exception as e:
            print("Send error: %s" % e)
            return False

    def close(self):
        if self.socket:
            self.socket.close()
            self.socket = None
