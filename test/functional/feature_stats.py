#!/usr/bin/env python3
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test stats reporting"""

import queue
import socket
import time
import threading

from test_framework.netutil import test_ipv6_local
from test_framework.test_framework import BitcoinTestFramework
from queue import Queue

ONION_ADDR = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"

class StatsServer:
    def __init__(self, host: str, port: int):
        self.running = False
        self.thread = None
        self.queue: Queue[str] = Queue()

        addr_info = socket.getaddrinfo(host, port, socket.AF_UNSPEC, socket.SOCK_DGRAM)
        self.af = addr_info[0][0]
        self.addr = (host, port)

        self.s = socket.socket(self.af, socket.SOCK_DGRAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(self.addr)
        self.s.settimeout(0.1)

    def run(self):
        while self.running:
            try:
                data, _ = self.s.recvfrom(4096)
                messages = data.decode('utf-8').strip().split('\n')
                for msg in messages:
                    if msg:
                        self.queue.put(msg)
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    raise AssertionError("Unexpected exception raised: " + type(e).__name__)

    def start(self):
        assert not self.running
        self.running = True
        self.thread = threading.Thread(target=self.run)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=2)
        self.s.close()

    def assert_msg_received(self, expected_msg: str, timeout: int = 30):
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                msg = self.queue.get(timeout=5)
                if expected_msg in msg:
                    return
            except queue.Empty:
                continue
        raise AssertionError(f"Did not receive message containing '{expected_msg}' within {timeout} seconds")


class StatsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info("Test invalid command line options")
        self.test_invalid_command_line_options()

        self.log.info("Test command line behavior")
        self.test_command_behavior()

        self.log.info("Check that server can receive stats client messages")
        self.have_ipv6 = test_ipv6_local()
        self.test_conn('127.0.0.1')
        if self.have_ipv6:
            self.test_conn('::1')
        else:
            self.log.warning("Testing without local IPv6 support")

    def test_invalid_command_line_options(self):
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Cannot init Statsd client (Port must be between 1 and 65535, supplied 65536)',
            extra_args=['-statshost=127.0.0.1', '-statsport=65536'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Cannot init Statsd client (No text before the scheme delimiter, malformed URL)',
            extra_args=['-statshost=://127.0.0.1'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Cannot init Statsd client (Unsupported URL scheme, must begin with udp://)',
            extra_args=['-statshost=http://127.0.0.1'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Cannot init Statsd client (No host specified, malformed URL)',
            extra_args=['-statshost=udp://'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=f'Error: Cannot init Statsd client (Host {ONION_ADDR} on unsupported network)',
            extra_args=[f'-statshost=udp://{ONION_ADDR}'],
        )

    def test_command_behavior(self):
        with self.nodes[0].assert_debug_log(expected_msgs=['Transmitting stats are disabled, will not init Statsd client']):
            self.restart_node(0, extra_args=[])
        # The port specified in the URL supercedes -statsport
        with self.nodes[0].assert_debug_log(expected_msgs=[
            'Supplied URL with port, ignoring -statsport',
            'StatsdClient initialized to transmit stats to 127.0.0.1:8126',
            'Started threaded RawSender sending messages to 127.0.0.1:8126'
        ]):
            self.restart_node(0, extra_args=['-debug=net', '-statshost=udp://127.0.0.1:8126', '-statsport=8125'])
        # Not specifying the port in the URL or -statsport will select the default port. Also, validate -statsduration behavior.
        with self.nodes[0].assert_debug_log(expected_msgs=[
            'Send interval is zero, not starting RawSender queueing thread',
            'StatsdClient initialized to transmit stats to 127.0.0.1:8125',
            'Started RawSender sending messages to 127.0.0.1:8125'
        ]):
            self.restart_node(0, extra_args=['-debug=net', '-statshost=udp://127.0.0.1', '-statsduration=0'])

    def test_conn(self, host: str):
        server = StatsServer(host, 8125)
        server.start()
        self.restart_node(0, extra_args=[f'-statshost=udp://{host}', '-statsbatchsize=0', '-statsduration=0'])
        server.assert_msg_received("CheckBlock_us")
        server.stop()

if __name__ == '__main__':
    StatsTest().main()
