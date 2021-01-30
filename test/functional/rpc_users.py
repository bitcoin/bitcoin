#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiple RPC users."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    str_to_b64str,
)

import os
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

    def setup_chain(self):
        super().setup_chain()
        self.authinfo = []

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
        p = subprocess.Popen([sys.executable, gen_rpcauth, 'rt2', self.rt2password], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        rpcauth2 = lines[1]

        # Generate RPCAUTH without specifying password
        self.user = ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, self.user], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        rpcauth3 = lines[1]
        self.password = lines[3]

        # Generate rpcauthfile with one entry
        username = 'rpcauth_single_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", os.path.join(self.options.tmpdir, "rpcauth_single"), username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )

        # Generate rpcauthfile with two entries
        username = 'rpcauth_multi1_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", os.path.join(self.options.tmpdir, "rpcauth_multi"), username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )
        # Blank lines in between should get ignored
        with open(os.path.join(self.options.tmpdir, "rpcauth_multi"), "a", encoding='utf8') as f:
            f.write("\n\n")
        username = 'rpcauth_multi2_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", os.path.join(self.options.tmpdir, "rpcauth_multi"), username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )

        # Hand-generated rpcauthfile with one entry and no newline
        username = 'rpcauth_nonewline_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        assert "\n" not in lines[1]
        assert lines[1][:8] == 'rpcauth='
        with open(os.path.join(self.options.tmpdir, "rpcauth_nonewline"), "a", encoding='utf8') as f:
            f.write(lines[1][8:])
        self.authinfo.append( (username, lines[3]) )

        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write(rpcauth + "\n")
            f.write(rpcauth2 + "\n")
            f.write(rpcauth3 + "\n")
            f.write("rpcauthfile=rpcauth_single\n")
            f.write("rpcauthfile=rpcauth_multi\n")
            f.write("rpcauthfile=rpcauth_nonewline\n")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 1), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcuser={}\n".format(self.rpcuser))
            f.write("rpcpassword={}\n".format(self.rpcpassword))

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
        self.log.info('Check correctness of the rpcauth config option')
        url = urllib.parse.urlparse(self.nodes[0].url)

        self.test_auth(self.nodes[0], url.username, url.password)
        self.test_auth(self.nodes[0], 'rt', self.rtpassword)
        self.test_auth(self.nodes[0], 'rt2', self.rt2password)
        self.test_auth(self.nodes[0], self.user, self.password)
        for info in self.authinfo:
            self.test_auth(self.nodes[0], *info)

        self.log.info('Check correctness of the rpcuser/rpcpassword config options')
        url = urllib.parse.urlparse(self.nodes[1].url)

        self.test_auth(self.nodes[1], self.rpcuser, self.rpcpassword)

        init_error = 'Error: Unable to start HTTP server. See debug log for details.'

        self.log.info('Check -rpcauth are validated')
        # Empty -rpcauth= are ignored
        self.restart_node(0, extra_args=['-rpcauth='])
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo:bar'])

        self.log.info('Check that failure to write cookie file will abort the node gracefully')
        cookie_file = os.path.join(get_datadir_path(self.options.tmpdir, 0), self.chain, '.cookie.tmp')
        os.mkdir(cookie_file)
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error)


if __name__ == '__main__':
    HTTPBasicsTest().main()
