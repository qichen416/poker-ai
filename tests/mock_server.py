import socket
import threading
import random
from poker_core import Card, CardUtils  # 假设的自定义模块，提供扑克牌和工具函数

class MockServer:
    """
    模拟德州扑克游戏服务器
    用于测试扑克 AI 客户端，按阶段发送手牌和公共牌，
    接收客户端的动作响应，但不进行筹码结算或胜负判定。
    """
    def __init__(self, host='localhost', port=10002):
        """
        初始化模拟服务器
        :param host: 监听的 IP 地址，默认 localhost
        :param port: 监听的端口号，默认 10002
        """
        self.host = host
        self.port = port
        # 创建一副完整的扑克牌（通常为52张）
        self.deck = CardUtils.create_deck()

    def _deal_cards(self):
        """
        洗牌并分配手牌和公共牌
        :return: (玩家A手牌列表, 玩家B手牌列表, 公共牌列表)
                 玩家A手牌发给客户端，玩家B手牌不使用，公共牌分阶段发送
        """
        random.shuffle(self.deck)          # 随机打乱牌堆
        # 前2张给玩家A，接下来2张给玩家B，再5张为公共牌
        return self.deck[0:2], self.deck[2:4], self.deck[4:9]

    def _run_game(self, client_socket):
        """
        为单个客户端连接运行一局完整的游戏
        :param client_socket: 已建立的客户端套接字
        """
        try:
            # ---------- 发牌阶段 ----------
            hole1, hole2, community = self._deal_cards()  # 获取分配的牌
            # 构造 preflop 消息：阶段|位置|手牌
            # 标记客户端为 BIGBLIND（大盲位），实际这里固定写死
            msg = "preflop|BIGBLIND|%s" % CardUtils.cards_to_protocol(hole1)
            # 发送消息，末尾加换行作为协议分隔符
            client_socket.sendall(("%s\n" % msg).encode())
            # 等待客户端返回动作（如 fold/call/raise）
            action1 = client_socket.recv(1024).decode().strip()
            print("[Mock] Preflop action: %s" % action1)

            # ---------- Flop 阶段 ----------
            # 发送前3张公共牌
            msg = "flop|%s" % CardUtils.cards_to_protocol(community[:3])
            client_socket.sendall(("%s\n" % msg).encode())
            action2 = client_socket.recv(1024).decode().strip()
            print("[Mock] Flop action: %s" % action2)

            # ---------- Turn 阶段 ----------
            # 发送第4张公共牌（单张）
            msg = "turn|%s" % community[3].to_protocol()
            client_socket.sendall(("%s\n" % msg).encode())
            action3 = client_socket.recv(1024).decode().strip()
            print("[Mock] Turn action: %s" % action3)

            # ---------- River 阶段 ----------
            # 发送第5张公共牌（单张）
            msg = "river|%s" % community[4].to_protocol()
            client_socket.sendall(("%s\n" % msg).encode())
            action4 = client_socket.recv(1024).decode().strip()
            print("[Mock] River action: %s" % action4)

            # ---------- 游戏结束 ----------
            client_socket.sendall(b"gameover\n")
            print("[Mock] Game over")

        except Exception as e:
            print("[Mock] Error: %s" % e)
        finally:
            # 确保连接关闭
            client_socket.close()

    def start(self):
        """
        启动模拟服务器，监听端口并为每个客户端创建线程处理游戏
        """
        # 创建 TCP 套接字
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # 允许端口重用，避免重启时地址被占用
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self.host, self.port))
        sock.listen(5)  # 最大等待连接队列长度
        print("[Mock] Server listening on %s:%d" % (self.host, self.port))

        try:
            while True:
                # 等待客户端连接
                client, addr = sock.accept()
                print("[Mock] Client connected from %s" % str(addr))
                # 为每个客户端启动一个守护线程，主线程退出时自动结束
                thread = threading.Thread(target=self._run_game, args=(client,))
                thread.daemon = True
                thread.start()
        except KeyboardInterrupt:
            # Ctrl+C 时优雅关闭
            print("[Mock] Server shutting down")
        finally:
            sock.close()

if __name__ == '__main__':
    # 程序入口：启动模拟服务器
    MockServer().start()