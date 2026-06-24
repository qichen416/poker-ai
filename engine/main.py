# 导入命令行参数解析库
import argparse
# 导入系统相关功能（用于退出程序）
import sys
# 导入自定义的网络客户端模块，负责与服务器通信
from engine.client import PokerClient
# 导入协议解析器，将服务器文本消息转换为游戏状态
from engine.parser import ProtocolParser
# 导入融合决策引擎，基于游戏状态生成AI动作
from engine.fusion import DecisionEngine
# 导入游戏状态数据结构
from poker_core import GameState


def run_game(client, protocol_parser, decide):
    """运行一段平台协议会话，并返回最后同步成功的状态。

    决策动作必须先成功发送，再写入本地 GameState；否则网络发送失败会让
    本地筹码领先于服务端，后续所有 to_call 都会建立在错误状态上。
    """
    state = GameState()
    while True:
        message = client.receive()
        if message is None:
            # 超时但 socket 仍存在时继续等待；真正断开且重连失败时才结束会话。
            if client.connected:
                continue
            return state
        if message == "gameover":
            return state

        state = protocol_parser.parse(message, state)
        action = decide(state)
        if not client.send(action):
            return state
        # sendall 成功后才能确认本次英雄动作已被本地状态接受。
        protocol_parser.record_action(action, state, is_opponent=False)

def main():
    # 创建命令行参数解析器
    parser = argparse.ArgumentParser()
    # 添加 --host 参数，指定服务器IP，默认 localhost
    parser.add_argument('--host', default='localhost', help='Platform IP')
    # 添加 --port 参数，指定服务器端口，类型为整数，默认 10002
    parser.add_argument('--port', type=int, default=10002, help='Platform port')
    # 解析命令行参数
    args = parser.parse_args()

    # 根据命令行参数创建 PokerClient 实例
    client = PokerClient(args.host, args.port)
    # 创建协议解析器
    parser = ProtocolParser()
    # 创建决策引擎
    engine = DecisionEngine()
    # 尝试连接服务器，如果失败则打印错误并退出
    if not client.connect():
        print("Failed to connect, exiting")
        sys.exit(1)

    # 连接成功，打印启动信息
    print("=== Poker AI started ===")

    try:
        run_game(client, parser, engine.make_decision)
        print("Game over")
    except KeyboardInterrupt:
        # 用户按下 Ctrl+C，优雅中断
        print("Interrupted by user")
    finally:
        # 无论如何，确保关闭网络连接
        client.close()

# 程序入口
if __name__ == '__main__':
    main()
