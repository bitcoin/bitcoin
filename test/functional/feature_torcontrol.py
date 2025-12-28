#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""

import socket
import threading
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, p2p_port


class MockTorControlServer:
    def __init__(self, port):
        self.port = port
        self.sock = None
        self.running = False
        self.thread = None
        self.received_commands = []

    def start(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.settimeout(1.0)
        self.sock.bind(('127.0.0.1', self.port))
        self.sock.listen(1)
        self.running = True
        self.thread = threading.Thread(target=self._serve)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.running = False
        if self.sock:
            self.sock.close()
        if self.thread:
            self.thread.join(timeout=5)

    def _serve(self):
        while self.running:
            try:
                conn, _ = self.sock.accept()
                conn.settimeout(1.0)
                self._handle_connection(conn)
            except socket.timeout:
                continue
            except OSError:
                break

    def _handle_connection(self, conn):
        try:
            buf = b""
            while self.running:
                try:
                    data = conn.recv(1024)
                    if not data:
                        break
                    buf += data
                    while b"\r\n" in buf:
                        line, buf = buf.split(b"\r\n", 1)
                        command = line.decode('utf-8').strip()
                        if command:
                            self.received_commands.append(command)
                            response = self._get_response(command)
                            conn.sendall(response.encode('utf-8'))
                except socket.timeout:
                    continue
        finally:
            conn.close()

    def _get_response(self, command):
        if command == "PROTOCOLINFO 1":
            return (
                "250-PROTOCOLINFO 1\r\n"
                "250-AUTH METHODS=NULL\r\n"
                "250-VERSION Tor=\"0.1.2.3\"\r\n"
                "250 OK\r\n"
            )
        elif command == "AUTHENTICATE":
            return "250 OK\r\n"
        elif command.startswith("ADD_ONION"):
            return (
                "250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\r\n"
                "250 OK\r\n"
            )
        elif command.startswith("GETINFO"):
            return "250-net/listeners/socks=\"127.0.0.1:9050\"\r\n250 OK\r\n"
        else:
            return "510 Unrecognized command\r\n"


class TorControlTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info("Test Tor control basic functionality")
        tor_port = p2p_port(self.num_nodes + 1)
        mock_tor = MockTorControlServer(tor_port)
        mock_tor.start()

        self.restart_node(0, extra_args=[
            f"-torcontrol=127.0.0.1:{tor_port}",
            "-listenonion=1",
            "-debug=tor"
        ])

        # Waiting for Tor control commands
        self.wait_until(lambda: len(mock_tor.received_commands) >= 4, timeout=10)

        # Verify expected protocol sequence
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")
        assert_equal(mock_tor.received_commands[1], "AUTHENTICATE")
        assert_equal(mock_tor.received_commands[2], "GETINFO net/listeners/socks")
        assert mock_tor.received_commands[3].startswith("ADD_ONION ")

        # Clean up
        mock_tor.stop()


if __name__ == '__main__':
    TorControlTest(__file__).main()
