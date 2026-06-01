import socket
import threading
import random
from poker_core import Card, CardUtils

class MockServer:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.deck = CardUtils.create_deck()

    def _deal_cards(self):
        import random
        random.shuffle(self.deck)
        return self.deck[0:2], self.deck[2:4], self.deck[4:9]

    def _run_game(self, client_socket):
        try:
            hole1, hole2, community = self._deal_cards()
            msg = "preflop|BIGBLIND|%s" % CardUtils.cards_to_protocol(hole1)
            client_socket.sendall(("%s\n" % msg).encode())
            action1 = client_socket.recv(1024).decode().strip()
            print("[Mock] Preflop action: %s" % action1)

            msg = "flop|%s" % CardUtils.cards_to_protocol(community[:3])
            client_socket.sendall(("%s\n" % msg).encode())
            action2 = client_socket.recv(1024).decode().strip()
            print("[Mock] Flop action: %s" % action2)

            msg = "turn|%s" % community[3].to_protocol()
            client_socket.sendall(("%s\n" % msg).encode())
            action3 = client_socket.recv(1024).decode().strip()
            print("[Mock] Turn action: %s" % action3)

            msg = "river|%s" % community[4].to_protocol()
            client_socket.sendall(("%s\n" % msg).encode())
            action4 = client_socket.recv(1024).decode().strip()
            print("[Mock] River action: %s" % action4)

            client_socket.sendall(b"gameover\n")
            print("[Mock] Game over")
        except Exception as e:
            print("[Mock] Error: %s" % e)
        finally:
            client_socket.close()

    def start(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self.host, self.port))
        sock.listen(5)
        print("[Mock] Server listening on %s:%d" % (self.host, self.port))
        try:
            while True:
                client, addr = sock.accept()
                print("[Mock] Client connected from %s" % str(addr))
                thread = threading.Thread(target=self._run_game, args=(client,))
                thread.daemon = True
                thread.start()
        except KeyboardInterrupt:
            print("[Mock] Server shutting down")
        finally:
            sock.close()

if __name__ == '__main__':
    MockServer().start()
