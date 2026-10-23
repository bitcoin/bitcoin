#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""
from contextlib import contextmanager
import os
import socket
import threading
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    p2p_port,
)


class MockTorControlServer:
    def __init__(self, port, manual_mode=False, host='127.0.0.1'):
        self.port = port
        self.host = host
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
        self.sock.bind((self.host, self.port))
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

    def restart_with_mock(self, mock_tor, extra_args=None):
        mock_tor.start()
        self.restart_node(0, extra_args=[
            f"-torcontrol={mock_tor.host}:{mock_tor.port}",
            "-listenonion=1",
            "-debug=tor",
        ] + (extra_args or []))

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

    def test_cookiefile_not_regular(self):
        self.log.info("Test that a COOKIEFILE that is not a regular file is not read")

        if not hasattr(os, "mkfifo"):
            self.log.info("Skipping, no FIFOs on this platform")
            return

        # Opening a FIFO for reading blocks until something opens it for writing.
        fifo = os.path.join(self.options.tmpdir, "cookie.fifo")
        os.mkfifo(fifo)
        escaped_fifo = fifo.replace("\\", "\\\\")

        class FifoCookieServer(MockTorControlServer):
            def _get_response(self, command):
                if command == "PROTOCOLINFO 1":
                    return (
                        "250-PROTOCOLINFO 1\r\n"
                        f"250-AUTH METHODS=COOKIE,SAFECOOKIE COOKIEFILE=\"{escaped_fifo}\"\r\n"
                        "250-VERSION Tor=\"0.1.2.3\"\r\n"
                        "250 OK\r\n"
                    )
                return super()._get_response(command)

        mock_tor = FifoCookieServer(self.next_port())
        with self.nodes[0].assert_debug_log(["is not a regular file"], timeout=10):
            self.restart_with_mock(mock_tor)
        # Authentication is not attempted.
        ensure_for(duration=1, f=lambda: len(mock_tor.received_commands) == 1)

        # The torcontrol thread must not be stuck, or shutdown would not finish.
        self.stop_node(0)
        mock_tor.stop()

    def test_cookiefile_missing(self):
        self.log.info("Test that a COOKIEFILE that cannot be examined is not read")

        missing = os.path.join(self.options.tmpdir, "no", "such", "cookie")
        escaped_missing = missing.replace("\\", "\\\\")

        class MissingCookieServer(MockTorControlServer):
            def _get_response(self, command):
                if command == "PROTOCOLINFO 1":
                    return (
                        "250-PROTOCOLINFO 1\r\n"
                        f"250-AUTH METHODS=COOKIE,SAFECOOKIE COOKIEFILE=\"{escaped_missing}\"\r\n"
                        "250-VERSION Tor=\"0.1.2.3\"\r\n"
                        "250 OK\r\n"
                    )
                return super()._get_response(command)

        mock_tor = MissingCookieServer(self.next_port())
        with self.nodes[0].assert_debug_log(["is not a regular file"], timeout=10):
            self.restart_with_mock(mock_tor)
        ensure_for(duration=1, f=lambda: len(mock_tor.received_commands) == 1)

        mock_tor.stop()

    def test_cookiefile_regular(self):
        self.log.info("Test that a COOKIEFILE that is a regular file is read")

        cookie = os.path.join(self.options.tmpdir, "cookie")
        with open(cookie, "wb") as f:
            f.write(bytes(32))

        # The backslashes of a Windows path are unescaped by the node.
        escaped_cookie = cookie.replace("\\", "\\\\")

        class CookieServer(MockTorControlServer):
            def _get_response(self, command):
                if command == "PROTOCOLINFO 1":
                    return (
                        "250-PROTOCOLINFO 1\r\n"
                        f"250-AUTH METHODS=COOKIE,SAFECOOKIE COOKIEFILE=\"{escaped_cookie}\"\r\n"
                        "250-VERSION Tor=\"0.1.2.3\"\r\n"
                        "250 OK\r\n"
                    )
                return super()._get_response(command)

        mock_tor = CookieServer(self.next_port())
        self.restart_with_mock(mock_tor)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 2, timeout=10)
        assert mock_tor.received_commands[1].startswith("AUTHCHALLENGE SAFECOOKIE ")

        mock_tor.stop()

    def check_socks_listeners(self, listeners, expected_proxy, expected_log, host="127.0.0.1", extra_args=None):
        class SocksServer(MockTorControlServer):
            def _get_response(self, command):
                if command.startswith("GETINFO"):
                    return f"250-net/listeners/socks={listeners}\r\n250 OK\r\n"
                return super()._get_response(command)

        mock_tor = SocksServer(self.next_port(), host=host)
        with self.nodes[0].assert_debug_log([expected_log], timeout=10):
            self.restart_with_mock(mock_tor, extra_args)
            self.wait_until(lambda: len(mock_tor.received_commands) >= 4, timeout=10)

        onion_proxy = next(n["proxy"] for n in self.nodes[0].getnetworkinfo()["networks"] if n["name"] == "onion")
        assert_equal(onion_proxy, expected_proxy)

        mock_tor.stop()
        # The port is free again, and p2p_port() allows only MAX_NODES ports.
        self._port_counter -= 1

    def nonloopback_address(self):
        """An address of this host that is not loopback and that accepts connections, or None."""
        # Connecting a UDP socket sends nothing, it only selects the local address.
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            try:
                s.connect(("192.0.2.1", 9))
                address = s.getsockname()[0]
            except OSError:
                return None
        if address.startswith("127.") or address == "0.0.0.0":
            return None
        # Some environments do not deliver connections to such an address.
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
                server.bind((address, 0))
                server.listen(1)
                server.settimeout(2)
                with socket.create_connection(server.getsockname(), timeout=2):
                    server.accept()[0].close()
        except OSError:
            return None
        return address

    def test_socks_listeners(self):
        self.log.info("Test which SOCKS listeners from Tor are used as onion proxy")
        ignored = "Ignoring SOCKS listener"

        self.log.debug("Control port on loopback: a loopback listener is used, wherever it is in the list")
        self.check_socks_listeners('"[::1]:9150" "203.0.113.7:9050"', "[::1]:9150", "Configuring onion proxy for [::1]:9150")
        self.check_socks_listeners('"203.0.113.7:9050" "[::1]:9150"', "[::1]:9150", "Configuring onion proxy for [::1]:9150")

        self.log.debug("Control port on loopback: another host is ignored, the default proxy is used")
        self.check_socks_listeners('"203.0.113.7:9050"', "127.0.0.1:9050", ignored)

        self.log.debug("Control port on loopback: a listener that is not a host address is ignored and logged")
        self.check_socks_listeners('"0.0.0.0:9050"', "127.0.0.1:9050", f"{ignored} 0.0.0.0:9050")
        self.check_socks_listeners('"unix:/run/tor/socks"', "127.0.0.1:9050", f"{ignored} unix:/run/tor/socks")

        self.log.debug("Control port on loopback: another host is ignored, the proxy from -proxy is kept")
        self.check_socks_listeners('"203.0.113.7:9050"', "203.0.113.9:9050", "Keeping onion proxy", extra_args=["-proxy=203.0.113.9:9050"])

        address = self.nonloopback_address()
        if address is None:
            self.log.info("Skipping the tests with a control port that is not on loopback, no such address")
            return

        self.log.debug("Control port not on loopback: a listener on the same host is used")
        self.check_socks_listeners(f'"{address}:9150"', f"{address}:9150", f"Configuring onion proxy for {address}:9150", host=address)

        self.log.debug("Control port not on loopback: another host is ignored, the proxy from -proxy is kept")
        self.check_socks_listeners('"203.0.113.7:9050"', f"{address}:9050", ignored, host=address, extra_args=[f"-proxy={address}:9050"])

        self.log.debug("Control port not on loopback: a loopback listener is ignored too")
        self.check_socks_listeners('"127.0.0.1:9050"', "203.0.113.9:9050", ignored, host=address, extra_args=["-proxy=203.0.113.9:9050"])

    def run_test(self):
        self.test_basic()
        self.test_partial_data()
        self.test_pow_fallback()
        self.test_oversized_line()
        self.test_overmany_lines()
        self.test_reconnect_backoff()
        self.test_cookiefile_not_regular()
        self.test_cookiefile_missing()
        self.test_cookiefile_regular()
        self.test_socks_listeners()


if __name__ == '__main__':
    TorControlTest(__file__).main()
