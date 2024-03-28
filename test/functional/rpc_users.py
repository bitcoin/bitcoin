#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiple RPC users."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    str_to_b64str,
)

import http.client
import urllib.parse
import subprocess
from random import SystemRandom
import string
import configparser
import sys


def call_with_auth(node, user, password):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(user, password))}

    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class HTTPBasicsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def conf_setup(self):
        #Append rpcauth to bitcoin.conf before initialization
        self.rtpassword = "cA773lm788buwYe4g4WT+05pKyNruVKjQ25x3n0DQcM="
        rpcauth = "rpcauth=rt:93648e835a54c573682c2eb19f882535$7681e9c5b74bdd85e78166031d2058e1069b3ed7ed967c93fc63abba06f31144"

        self.rpcuser = "rpcuser💻"
        self.rpcpassword = "rpcpassword🔑"

        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        gen_rpcauth = config['environment']['RPCAUTH']

        # Generate RPCAUTH with specified password
        self.rt2password = "8/F3uMDw4KSEbw96U3CA1C4X05dkHDN2BPFjTgZW4KI="
        p = subprocess.Popen([sys.executable, gen_rpcauth, 'rt2', self.rt2password], stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpcauth2 = lines[1]

        # Generate RPCAUTH without specifying password
        self.user = ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, self.user], stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpcauth3 = lines[1]
        self.password = lines[3]

        with open(self.nodes[0].datadir_path / "bitcoin.conf", "a", encoding="utf8") as f:
            f.write(rpcauth + "\n")
            f.write(rpcauth2 + "\n")
            f.write(rpcauth3 + "\n")
        with open(self.nodes[1].datadir_path / "bitcoin.conf", "a", encoding="utf8") as f:
            f.write("rpcuser={}\n".format(self.rpcuser))
            f.write("rpcpassword={}\n".format(self.rpcpassword))
        self.restart_node(0)
        self.restart_node(1)

    def test_auth(self, node, user, password):
        self.log.info('Correct...')
        assert_equal(200, call_with_auth(node, user, password).status)

        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user, password + 'wrong').status)

        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user + 'wrong', password).status)

        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user + 'wrong', password + 'wrong').status)

    def run_test(self):
        self.conf_setup()
        self.log.info('Check correctness of the rpcauth config option')
        url = urllib.parse.urlparse(self.nodes[0].url)

        self.test_auth(self.nodes[0], url.username, url.password)
        self.test_auth(self.nodes[0], 'rt', self.rtpassword)
        self.test_auth(self.nodes[0], 'rt2', self.rt2password)
        self.test_auth(self.nodes[0], self.user, self.password)

        self.log.info('Check correctness of the rpcuser/rpcpassword config options')
        url = urllib.parse.urlparse(self.nodes[1].url)

        self.test_auth(self.nodes[1], self.rpcuser, self.rpcpassword)

        init_error = 'Error: Unable to start HTTP server. See debug log for details.'

        self.log.info('Check blank -rpcauth is ignored')
        rpcauth_abc = '-rpcauth=abc:$2e32c2f20c67e29c328dd64a4214180f18da9e667d67c458070fd856f1e9e5e7'
        rpcauth_def = '-rpcauth=def:$fd7adb152c05ef80dccf50a1fa4c05d5a3ec6da95575fc312ae7c5d091836351'
        self.restart_node(0, extra_args=['-rpcauth'])
        self.restart_node(0, extra_args=['-rpcauth=', rpcauth_abc])
        self.restart_node(0, extra_args=[rpcauth_def, '-rpcauth='])
        # ...without disrupting usage of other -rpcauth tokens
        assert_equal(200, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)

        self.log.info('Check -norpcauth disables all previous -rpcauth params')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth'])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(401, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)

        self.log.info('Check -norpcauth can be reversed with -rpcauth')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', '-rpcauth'])
        # FIXME: assert_equal(200, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)

        self.log.info('Check -norpcauth followed by a specific -rpcauth=* restores config file -rpcauth=* values too')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', rpcauth_abc])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', '-rpcauth='])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)

        self.log.info('Check -rpcauth are validated')
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo:bar'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo:bar:baz'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo$bar:baz'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo$bar$baz'])

        self.log.info('Check that failure to write cookie file will abort the node gracefully')
        (self.nodes[0].chain_path / ".cookie.tmp").mkdir()
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error)


if __name__ == '__main__':
    HTTPBasicsTest().main()
