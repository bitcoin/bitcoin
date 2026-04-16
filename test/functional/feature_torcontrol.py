#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""
import socket
import threading
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    p2p_port,
)


class MockTorControlServer:
    def __init__(self, port, manual_mode=False):
        self.port = port
        self.sock = None
        self.conn = None
        self.running = False
        self.thread = None
        self.received_commands = []
        self.manual_mode = manual_mode
        self.conn_ready = threading.Event()

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
        if self.conn:
            self.conn.close()
        if self.sock:
            self.sock.close()
        if self.thread:
            self.thread.join(timeout=5)

    def _serve(self):
        while self.running:
            try:
                self.conn, _ = self.sock.accept()
                self.conn.settimeout(1.0)
                self.conn_ready.set()
                self._handle_connection(self.conn)
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
                            if not self.manual_mode:
                                response = self._get_response(command)
                                conn.sendall(response.encode('utf-8'))
                except socket.timeout:
                    continue
        finally:
            conn.close()

    def send_raw(self, data):
        if self.conn:
            self.conn.sendall(data.encode('utf-8'))

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

    def next_port(self):
        self._port_counter = getattr(self, '_port_counter', 0) + 1
        return p2p_port(self.num_nodes + self._port_counter)

    def restart_with_mock(self, mock_tor):
        mock_tor.start()
        self.restart_node(0, extra_args=[
            f"-torcontrol=127.0.0.1:{mock_tor.port}",
            "-listenonion=1",
            "-debug=tor",
        ])

    def test_basic(self):
        self.log.info("Test Tor control basic functionality")

        mock_tor = MockTorControlServer(self.next_port())
        self.restart_with_mock(mock_tor)

        # Waiting for Tor control commands
        self.wait_until(lambda: len(mock_tor.received_commands) >= 4, timeout=10)

        # Verify expected protocol sequence
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")
        assert_equal(mock_tor.received_commands[1], "AUTHENTICATE")
        assert_equal(mock_tor.received_commands[2], "GETINFO net/listeners/socks")
        assert mock_tor.received_commands[3].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" in mock_tor.received_commands[3]

        # Clean up
        mock_tor.stop()

    def test_partial_data(self):
        self.log.info("Test that partial Tor control responses are buffered until complete")

        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        # Wait for connection and PROTOCOLINFO command
        mock_tor.conn_ready.wait(timeout=10)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 1, timeout=10)
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")

        # Send partial response (no \r\n on last line)
        mock_tor.send_raw(
            "250-PROTOCOLINFO 1\r\n"
            "250-AUTH METHODS=NULL\r\n"
            "250 OK"
        )

        # Verify AUTHENTICATE is not sent
        ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == 1)

        # Complete the response
        mock_tor.send_raw("\r\n")

        # Should now process the complete response and send AUTHENTICATE
        self.wait_until(lambda: len(mock_tor.received_commands) >= 2, timeout=5)
        assert_equal(mock_tor.received_commands[1], "AUTHENTICATE")

        # Clean up
        mock_tor.stop()

    def test_pow_fallback(self):
        self.log.info("Test that ADD_ONION retries without PoW on 512 error")

        class NoPowServer(MockTorControlServer):
            def _get_response(self, command):
                if command.startswith("ADD_ONION"):
                    if "PoWDefensesEnabled=1" in command:
                        return "512 Unrecognized option\r\n"
                    else:
                        return (
                            "250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\r\n"
                            "250 OK\r\n"
                        )
                return super()._get_response(command)

        mock_tor = NoPowServer(self.next_port())
        self.restart_with_mock(mock_tor)

        # Expect: PROTOCOLINFO, AUTHENTICATE, GETINFO, ADD_ONION (with PoW), ADD_ONION (without PoW)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 5, timeout=10)

        # First ADD_ONION should have PoW enabled
        assert mock_tor.received_commands[3].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" in mock_tor.received_commands[3]

        # Retry should be ADD_ONION without PoW
        assert mock_tor.received_commands[4].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" not in mock_tor.received_commands[4]

        # Clean up
        mock_tor.stop()

    def test_oversized_line(self):
        self.log.info("Test that Tor control disconnects on oversized response lines")

        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        # Wait for connection and PROTOCOLINFO command.
        mock_tor.conn_ready.wait(timeout=10)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 1, timeout=10)
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")

        # Send a single line longer than MAX_LINE_LENGTH. The node should disconnect.
        MAX_LINE_LENGTH = 100000
        mock_tor.send_raw("250-" + ("A" * (MAX_LINE_LENGTH + 1)) + "\r\n")
        ensure_for(duration=2, f=lambda: self.nodes[0].process.poll() is None)

        # Connection should be dropped and retried, causing another PROTOCOLINFO.
        self.wait_until(lambda: len(mock_tor.received_commands) >= 2, timeout=10)
        assert_equal(mock_tor.received_commands[1], "PROTOCOLINFO 1")

        mock_tor.stop()

    def run_test(self):
        self.test_basic()
        self.test_partial_data()
        self.test_pow_fallback()
        self.test_oversized_line()

if __name__ == '__main__':
    TorControlTest(__file__).main()
