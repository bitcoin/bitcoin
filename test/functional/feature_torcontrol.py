#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""
from contextlib import contextmanager
import hashlib
import hmac
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


# Constants from torcontrol.cpp
TOR_SAFE_SERVERKEY = b"Tor safe cookie authentication server-to-controller hash"
TOR_SAFE_CLIENTKEY = b"Tor safe cookie authentication controller-to-server hash"


class SafeCookieServer(MockTorControlServer):
    def __init__(self, port, cookie_data, cookie_fpath, fail_authchallenge=False, server_nonce=b'\x44' * 32, corrupt_server_hash=False):
        super().__init__(port, manual_mode=True)

        self.cookie = cookie_data
        self.cookie_path = cookie_fpath
        self.client_nonce = None
        self.server_nonce = server_nonce
        self.fail_authchallenge = fail_authchallenge
        self.corrupt_server_hash = corrupt_server_hash

    def _compute_hmac(self, key, client_nonce):
        """Compute HMAC-SHA256 for SAFECOOKIE authentication"""
        return hmac.new(key, self.cookie + client_nonce + self.server_nonce, hashlib.sha256).digest()

    def _handle_authchallenge(self, command):
        """Parse AUTHCHALLENGE command and extract client nonce"""
        if self.fail_authchallenge:
            return "515 Authentication failed\r\n"

        # Format: AUTHCHALLENGE SAFECOOKIE <client_nonce_hex>
        parts = command.split()
        if len(parts) != 3:
            return "513 Syntax error in AUTHCHALLENGE command\r\n"

        self.client_nonce = bytes.fromhex(parts[2])
        server_hash = self._compute_hmac(TOR_SAFE_SERVERKEY, self.client_nonce)
        if self.corrupt_server_hash:
            # a real HMAC-SHA256 hash will never be 32 zero bytes
            server_hash = b'\x00' * 32
        return (f"250-AUTHCHALLENGE SERVERHASH={server_hash.hex()} "
                f"SERVERNONCE={self.server_nonce.hex()}\r\n"
                f"250 OK\r\n")

    def _handle_authenticate(self, command):
        """Verify AUTHENTICATE command has correct client hash"""

        # Format: AUTHENTICATE <client_hash_hex>
        parts = command.split()
        if len(parts) == 2 and self.client_nonce is not None:
            received_hash = bytes.fromhex(parts[1])
            # Compute expected client hash
            expected_hash = self._compute_hmac(TOR_SAFE_CLIENTKEY, self.client_nonce)
            if received_hash == expected_hash:
                return "250 OK\r\n"
        return "515 Bad authentication\r\n"

    def _get_response(self, command):
        if command == "PROTOCOLINFO 1":
            # COOKIEFILE is a QuotedString, so backslashes (Windows paths) must be escaped
            cookie_path = str(self.cookie_path).replace("\\", "\\\\")
            return ("250-PROTOCOLINFO 1\r\n"
                    f'250-AUTH METHODS=SAFECOOKIE COOKIEFILE="{cookie_path}"\r\n'
                    '250-VERSION Tor="0.1.2.3"\r\n'
                    "250 OK\r\n")
        if command.startswith("AUTHCHALLENGE"):
            return self._handle_authchallenge(command)
        if command.startswith("AUTHENTICATE"):
            return self._handle_authenticate(command)
        if command.startswith("GETINFO"):
            return '250-net/listeners/socks="127.0.0.1:9050"\r\n250 OK\r\n'
        if command.startswith("ADD_ONION"):
            return ("250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\r\n"
                    "250 OK\r\n")
        return super()._get_response(command)

    def get_response(self, command):
        return self._get_response(command)


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

        # Wait for connection and PROTOCOLINFO command
        mock_tor.conn_ready.wait(timeout=10)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 1, timeout=10)
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")

    @contextmanager
    def expect_disconnect(self, expect, mock_tor):
        initial_len = len(mock_tor.received_commands)
        yield

        if expect:
            # No reconnect before the initial reconnect timeout of 1s has passed
            ensure_for(duration=0.5, f=lambda: len(mock_tor.received_commands) == initial_len)
            # Expect to receive a PROTOCOLINFO 1 on reconnect, bumping the received
            # commands length.
            self.wait_until(lambda: len(mock_tor.received_commands) == initial_len + 1)
            assert_equal(mock_tor.received_commands[initial_len], "PROTOCOLINFO 1")
        else:
            # No disconnect, so no reconnect message
            ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == initial_len)

    def expect_command(self, mock_tor, index, prefix):
        """Wait until the mock received command number `index`, check its prefix and return it."""
        self.wait_until(lambda: len(mock_tor.received_commands) > index, timeout=10)
        command = mock_tor.received_commands[index]
        assert command.startswith(prefix), f"Expected command {index} to start with {prefix!r}, got {command!r}."
        return command

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

    def test_reconnect_backoff(self):
        self.log.info("Test that a connection closed by Tor is re-established with backoff")

        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        with self.expect_disconnect(True, mock_tor):
            # Reply before closing, like Tor does after a failed AUTHENTICATE
            mock_tor.send_raw("515 Authentication failed\r\n")
            mock_tor.conn.shutdown(socket.SHUT_WR)

        mock_tor.stop()

    def test_safecookie_auth_success(self):
        self.log.info("Test that SAFECOOKIE authentication succeeds")

        # Store cookie file in node's datadir
        cookie = b'\x12' * 32
        cookie_path = self.nodes[0].datadir_path / "tor_cookie"
        cookie_path.write_bytes(cookie)

        mock_tor = SafeCookieServer(self.next_port(), cookie, str(cookie_path))
        self.restart_with_mock(mock_tor)
        mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        command = self.expect_command(mock_tor, 1, "AUTHCHALLENGE SAFECOOKIE ")
        mock_tor.send_raw(mock_tor.get_response(command))

        command = self.expect_command(mock_tor, 2, "AUTHENTICATE ")
        auth_response = mock_tor.get_response(command)
        mock_tor.send_raw(auth_response)
        # Verify successful authentication
        assert_equal(auth_response, "250 OK\r\n")

        # After successful auth, we should proceed to GETINFO
        command = self.expect_command(mock_tor, 3, "GETINFO net/listeners/socks")
        mock_tor.send_raw(mock_tor.get_response(command))
        self.expect_command(mock_tor, 4, "ADD_ONION ")

        mock_tor.stop()

    def test_safecookie_authchallenge_error(self):
        self.log.info("Test that AUTHCHALLENGE returns an error code")

        cookie = b'\x12' * 32
        cookie_path = self.nodes[0].datadir_path / "tor_cookie"
        cookie_path.write_bytes(cookie)

        mock_tor = SafeCookieServer(self.next_port(), cookie, str(cookie_path), fail_authchallenge=True)
        self.restart_with_mock(mock_tor)
        mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        command = self.expect_command(mock_tor, 1, "AUTHCHALLENGE SAFECOOKIE ")
        with self.nodes[0].assert_debug_log(["SAFECOOKIE authentication challenge failed"], timeout=10):
            mock_tor.send_raw(mock_tor.get_response(command))

        mock_tor.stop()

    def test_safecookie_short_cookie(self):
        self.log.info("Test that SAFECOOKIE authentication fails when the cookie file is shorter than 32 bytes")

        # Store short cookie file in node's datadir
        short_cookie = b'\x12' * 31
        short_cookie_path = self.nodes[0].datadir_path / "tor_cookie_short"
        short_cookie_path.write_bytes(short_cookie)

        mock_tor = SafeCookieServer(self.next_port(), short_cookie, str(short_cookie_path))
        self.restart_with_mock(mock_tor)
        with self.nodes[0].assert_debug_log([f"Authentication cookie {short_cookie_path} is not exactly 32 bytes"], timeout=10):
            mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        mock_tor.stop()

    def test_safecookie_unreadable_cookie(self):
        self.log.info("Test that SAFECOOKIE authentication is available but the cookie file cannot be read")

        # Point COOKIEFILE at a path that doesn't exist, so the node can't read it
        missing_cookie_path = self.nodes[0].datadir_path / "tor_cookie_missing"

        cookie = b'\x12' * 32
        mock_tor = SafeCookieServer(self.next_port(), cookie, str(missing_cookie_path))
        self.restart_with_mock(mock_tor)
        with self.nodes[0].assert_debug_log([f"Authentication cookie {missing_cookie_path} could not be opened"], timeout=10):
            mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        mock_tor.stop()

    def test_safecookie_invalid_server_nonce(self):
        self.log.info("Test that SAFECOOKIE authentication fails when the server nonce is not 32 bytes")

        cookie = b'\x12' * 32
        cookie_path = self.nodes[0].datadir_path / "tor_cookie"
        cookie_path.write_bytes(cookie)

        # The server hash for the 31-byte nonce is valid, but the nonce length is wrong
        mock_tor = SafeCookieServer(self.next_port(), cookie, str(cookie_path), server_nonce=b'\x44' * 31)
        self.restart_with_mock(mock_tor)
        mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        command = self.expect_command(mock_tor, 1, "AUTHCHALLENGE SAFECOOKIE ")
        with self.nodes[0].assert_debug_log(["ServerNonce is not 32 bytes"], timeout=10):
            mock_tor.send_raw(mock_tor.get_response(command))

        mock_tor.stop()

    def test_safecookie_server_hash_mismatch(self):
        self.log.info("Test that SAFECOOKIE authentication fails when the server hash does not match the computed one")

        cookie = b'\x12' * 32
        cookie_path = self.nodes[0].datadir_path / "tor_cookie"
        cookie_path.write_bytes(cookie)

        mock_tor = SafeCookieServer(self.next_port(), cookie, str(cookie_path), corrupt_server_hash=True)
        self.restart_with_mock(mock_tor)
        mock_tor.send_raw(mock_tor.get_response("PROTOCOLINFO 1"))

        command = self.expect_command(mock_tor, 1, "AUTHCHALLENGE SAFECOOKIE ")
        with self.nodes[0].assert_debug_log(["does not match expected ServerHash"], timeout=10):
            mock_tor.send_raw(mock_tor.get_response(command))

        # Node must not send AUTHENTICATE to a server that failed to prove it knows the cookie
        ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == 2)
        mock_tor.stop()

    def run_test(self):
        self.test_basic()
        self.test_partial_data()
        self.test_pow_fallback()
        self.test_oversized_line()
        self.test_overmany_lines()
        self.test_reconnect_backoff()

        # Reset the port counter, otherwise 'next_port()' can run
        # past 'MAX_NODES'
        self._port_counter = 0

        self.test_safecookie_auth_success()
        self.test_safecookie_authchallenge_error()
        self.test_safecookie_short_cookie()
        self.test_safecookie_unreadable_cookie()
        self.test_safecookie_invalid_server_nonce()
        self.test_safecookie_server_hash_mismatch()


if __name__ == '__main__':
    TorControlTest(__file__).main()
