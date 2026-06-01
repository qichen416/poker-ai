import argparse
import sys
from engine.client import PokerClient
from engine.parser import ProtocolParser
from engine.fusion import DecisionEngine
from poker_core import GameState

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='localhost', help='Platform IP')
    parser.add_argument('--port', type=int, default=8888, help='Platform port')
    args = parser.parse_args()

    client = PokerClient(args.host, args.port)
    parser = ProtocolParser()
    engine = DecisionEngine()
    state = GameState()

    if not client.connect():
        print("Failed to connect, exiting")
        sys.exit(1)

    print("=== Poker AI started ===")

    try:
        while True:
            msg = client.receive()
            if msg is None:
                break
            if msg == 'gameover':
                print("Game over")
                break
            state = parser.parse(msg, state)
            print("Received: %s" % msg)
            print("State: %s" % state)
            action = engine.make_decision(state)
            print("Action: %s" % action)
            if not client.send(action):
                break
    except KeyboardInterrupt:
        print("Interrupted by user")
    finally:
        client.close()

if __name__ == '__main__':
    main()
