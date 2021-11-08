#!/usr/bin/env python3
"""
Basic example of monitoring for reception of a new block.
"""

import time

from bitcoincore import p2p

# Uncomment to display all P2P messages received.
# import logging
# logging.basicConfig(level=logging.DEBUG)


class BlockLogger(p2p.P2PInterface):

    def on_block(self, msg):
        block = msg.block
        block.rehash()
        print(f"--> found block!\n    {block.sha256} {block.hash}")


def main():
    netthread = p2p.NetworkThread()
    netthread.start()
    peer = None

    try:
        peer = BlockLogger()
        peer.peer_connect('127.0.0.1', 8333, net='mainnet', timeout_factor=10)()
        peer.wait_for_connect()
        peer.wait_for_verack()
        ack = peer.last_message['version']
        print(f"Saw verack from local peer: {ack}")
        while True:
            time.sleep(1)
    finally:
        if peer and peer.is_connected:
            print("Disconnecting peer")
            peer.peer_disconnect()
            peer.wait_for_disconnect()
        netthread.close()


if __name__ == '__main__':
    main()
