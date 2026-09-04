#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""
from contextlib import contextmanager
import socket
import threading
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    p2p_port,
)

SERVICE_ID = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd"


class MockTorControlServer:
    def __init__(self, port, manual_mode=False, private_key=None, service_id=SERVICE_ID):
        self.port = port
        self.sock = None
        self.conn = None
        self.running = False
        self.thread = None
        self.received_commands = []
        self.manual_mode = manual_mode
        self.private_key = private_key  # returned by ADD_ONION, as Tor does for a generated key
        self.service_id = service_id
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
            reply = f"250-ServiceID={self.service_id}\r\n"
            if self.private_key:
                reply += f"250-PrivateKey={self.private_key}\r\n"
            return reply + "250 OK\r\n"
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

    def restart_with_mock(self, mock_tor, cached_private_key=None):
        self.stop_node(0)
        key_path = self.nodes[0].chain_path / "onion_v3_private_key"
        if cached_private_key is None:
            key_path.unlink(missing_ok=True)
        else:
            key_path.write_bytes(cached_private_key.encode())
        mock_tor.start()
        self.start_node(0, extra_args=[
            f"-torcontrol=127.0.0.1:{mock_tor.port}",
            "-listenonion=1",
            "-debug=tor",
        ])

        # Wait for connection and PROTOCOLINFO command
        mock_tor.conn_ready.wait(timeout=10)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 1, timeout=10)
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")

    @contextmanager
    def expect_disconnect(self, expect, mock_tor):
        initial_len = len(mock_tor.received_commands)
        yield

        if expect:
            # Expect to receive a PROTOCOLINFO 1 on reconnect, bumping the received
            # commands length.
            self.wait_until(lambda: len(mock_tor.received_commands) == initial_len + 1)
            assert_equal(mock_tor.received_commands[initial_len], "PROTOCOLINFO 1")
        else:
            # No disconnect, so no reconnect message
            ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == initial_len)

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
                if command.startswith("ADD_ONION") and "PoWDefensesEnabled=1" in command:
                    return "512 Unrecognized option\r\n"
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
        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        MAX_LINE_LENGTH = 100000

        self.log.info("Test that Tor control does not disconnect with a MAX_LINE_LENGTH line.")
        with self.expect_disconnect(False, mock_tor):
            msg = "250-" + ("A" * (MAX_LINE_LENGTH - 5)) + "\r"
            assert_equal(len(msg), MAX_LINE_LENGTH)
            # The \n is not counted in line length.
            mock_tor.send_raw(msg + "\n")

        self.log.info("Test that Tor control disconnects with a MAX_LINE_LENGTH + 1 line")
        with self.expect_disconnect(True, mock_tor):
            msg = "250-" + ("A" * (MAX_LINE_LENGTH - 4)) + "\r"
            assert_equal(len(msg), MAX_LINE_LENGTH + 1)
            mock_tor.send_raw(msg + "\n")

        mock_tor.stop()

    def test_overmany_lines(self):
        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        MAX_LINE_COUNT = 1000

        self.log.info("Test that Tor control does not disconnect on receiving MAX_LINE_COUNT lines.")
        with self.expect_disconnect(False, mock_tor):
            for _ in range(MAX_LINE_COUNT - 1):
                mock_tor.send_raw("250-Continuing\r\n")
            mock_tor.send_raw("250 OK\r\n")

        self.log.info("Test that Tor control disconnects on receiving MAX_LINE_COUNT + 1 lines.")
        with self.expect_disconnect(True, mock_tor):
            for _ in range(MAX_LINE_COUNT + 1):
                mock_tor.send_raw("250-Continuing\r\n")

        mock_tor.stop()

    def test_malformed_service_id(self):
        invalid_service_id = "invalid"
        key_path = self.nodes[0].chain_path / "onion_v3_private_key"
        mock_tor = MockTorControlServer(self.next_port(), service_id=invalid_service_id)

        self.log.info("Test a malformed service ID returned by ADD_ONION")
        with self.nodes[0].assert_debug_log([f"Got tor service ID {invalid_service_id}"], timeout=10):  # TODO: Reject the malformed service ID
            self.restart_with_mock(mock_tor)
        assert key_path.exists()  # TODO: Do not cache a key for a malformed service ID
        mock_tor.stop()

    def test_private_key_tor_command_injection(self):
        valid_private_key = "ED25519-V3:wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w=="  # 64 arbitrary bytes encoded as Base64
        key_path = self.nodes[0].chain_path / "onion_v3_private_key"

        self.log.info("Test that a valid returned private key is cached and reused")
        tor_control_port = self.next_port()
        mock_tor = MockTorControlServer(tor_control_port, private_key=valid_private_key)
        with self.nodes[0].assert_debug_log(["Cached service private key"], timeout=10):
            self.restart_with_mock(mock_tor)
        assert_equal(key_path.read_bytes(), valid_private_key.encode())
        mock_tor.stop()

        mock_tor = MockTorControlServer(tor_control_port)
        self.restart_with_mock(mock_tor, cached_private_key=valid_private_key)
        self.wait_until(lambda: any(command.startswith(f"ADD_ONION {valid_private_key} ") for command in mock_tor.received_commands), timeout=10)
        mock_tor.stop()

        for private_key, injected, cached_log in [
            # A line break ends the ADD_ONION command, so Tor runs the rest of the key as a second command
            (
                f"{valid_private_key}\r\nSIGNAL SHUTDOWN\r\n",
                "SIGNAL SHUTDOWN",
                "Received unexpected sync reply 510",  # TODO: Refuse to use the cached key
            ),
            # A space ends the key argument, so Tor parses the rest of the key as further ADD_ONION arguments
            (
                f"{valid_private_key} Flags=Detach",
                "Flags=Detach",
                "Cached service private key",  # TODO: Refuse to use the cached key
            ),
        ]:
            self.log.info(f"Test {injected!r} injected through a returned private key")
            # A reply line ends at CRLF and an unquoted value at a space, so only a quoted value can carry either
            escaped_private_key = private_key.replace("\r", "\\r").replace("\n", "\\n")
            quoted_private_key = f'"{escaped_private_key}"'
            mock_tor = MockTorControlServer(tor_control_port, private_key=quoted_private_key)
            with self.nodes[0].assert_debug_log(["Cached service private key"], timeout=10):  # TODO: Reject the returned key
                self.restart_with_mock(mock_tor)
            assert_equal(key_path.read_bytes(), private_key.encode())  # TODO: Reject the returned key
            mock_tor.stop()

            self.log.info(f"Test {injected!r} injected through a cached private key")
            mock_tor = MockTorControlServer(tor_control_port)
            with self.nodes[0].assert_debug_log([cached_log], timeout=10):
                self.restart_with_mock(mock_tor, cached_private_key=private_key)
            self.wait_until(lambda: any(injected in command for command in mock_tor.received_commands), timeout=10)  # TODO: Refuse to send the cached key
            mock_tor.stop()

    def run_test(self):
        self.test_basic()
        self.test_partial_data()
        self.test_pow_fallback()
        self.test_oversized_line()
        self.test_overmany_lines()
        self.test_malformed_service_id()
        self.test_private_key_tor_command_injection()


if __name__ == '__main__':
    TorControlTest(__file__).main()
