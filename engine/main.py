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
    # 初始化一个空游戏状态（阶段通常为 PREFLOP，无手牌和公共牌）
    state = GameState()

    # 尝试连接服务器，如果失败则打印错误并退出
    if not client.connect():
        print("Failed to connect, exiting")
        sys.exit(1)

    # 连接成功，打印启动信息
    print("=== Poker AI started ===")

    try:
        # 进入主循环，持续接收并处理服务器消息
        while True:
            # 阻塞接收一条消息（服务器一行协议文本）
            msg = client.receive()
            # 如果收到空消息（连接断开），跳出循环
            if msg is None:
                break
            # 如果消息为 'gameover'，表示游戏结束，跳出循环
            if msg == 'gameover':
                print("Game over")
                break
            # 使用协议解析器将消息应用到当前状态，更新手牌、公共牌、阶段等信息
            state = parser.parse(msg, state)
            # 打印收到的原始消息
            print("Received: %s" % msg)
            # 打印更新后的游戏状态
            print("State: %s" % state)
            # 决策引擎根据当前状态生成动作（如 fold, call, raise 等）
            action = engine.make_decision(state)
            # 打印 AI 决定采取的动作
            print("Action: %s" % action)
            # 将动作发送给服务器，如果发送失败则跳出循环
            if not client.send(action):
                break
    except KeyboardInterrupt:
        # 用户按下 Ctrl+C，优雅中断
        print("Interrupted by user")
    finally:
        # 无论如何，确保关闭网络连接
        client.close()

# 程序入口
if __name__ == '__main__':
    main()