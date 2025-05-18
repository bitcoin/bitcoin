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
import os
from pathlib import Path
import platform
import urllib.parse
import subprocess
from random import SystemRandom
import string
import configparser
import sys
from typing import Optional


def call_with_auth(node, user, password, method="getbestblockhash"):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(user, password))}

    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', f'{{"method": "{method}"}}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class HTTPBasicsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def conf_setup(self):
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
        p = subprocess.Popen([sys.executable, gen_rpcauth, 'rt2', self.rt2password], stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpcauth2 = lines[1]

        # Generate RPCAUTH without specifying password
        self.user = ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, self.user], stdout=subprocess.PIPE, text=True)
        lines = p.stdout.read().splitlines()
        rpcauth3 = lines[1]
        self.password = lines[3]

        # Generate rpcauthfile with one entry
        username = 'rpcauth_single_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", Path(self.options.tmpdir) / 'rpcauth_single', username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )

        # Generate rpcauthfile with two entries
        username = 'rpcauth_multi1_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", Path(self.options.tmpdir) / 'rpcauth_multi', username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )
        # Blank lines in between should get ignored
        with open(Path(self.options.tmpdir) / 'rpcauth_multi', "a", encoding='utf8') as f:
            f.write("\n\n")
        username = 'rpcauth_multi2_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, "--output", Path(self.options.tmpdir) / 'rpcauth_multi', username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        self.authinfo.append( (username, lines[1]) )

        # Hand-generated rpcauthfile with one entry and no newline
        username = 'rpcauth_nonewline_' + ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        p = subprocess.Popen([sys.executable, gen_rpcauth, username], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        assert "\n" not in lines[1]
        assert lines[1][:8] == 'rpcauth='
        with open(Path(self.options.tmpdir) / 'rpcauth_nonewline', "a", encoding='utf8') as f:
            f.write(lines[1][8:])
        self.authinfo.append( (username, lines[3]) )

        with open(self.nodes[0].datadir_path / "bitcoin.conf", "a", encoding="utf8") as f:
            f.write(rpcauth + "\n")
            f.write(rpcauth2 + "\n")
            f.write(rpcauth3 + "\n")
            f.write("rpcauthfile=rpcauth_single\n")
            f.write("rpcauthfile=rpcauth_multi\n")
            f.write("rpcauthfile=rpcauth_nonewline\n")
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

    def test_rpccookieperms(self):
        p = {"owner": 0o600, "group": 0o640, "all": 0o644}

        if platform.system() == 'Windows':
            self.log.info(f"Skip cookie file permissions checks as OS detected as: {platform.system()=}")
            return

        self.log.info('Check cookie file permissions can be set using -rpccookieperms')

        cookie_file_path = self.nodes[1].chain_path / '.cookie'
        PERM_BITS_UMASK = 0o777

        def test_perm(perm: Optional[str]):
            if not perm:
                perm = 'owner'
                self.restart_node(1)
            else:
                self.restart_node(1, extra_args=[f"-rpccookieperms={perm}"])

            file_stat = os.stat(cookie_file_path)
            actual_perms = file_stat.st_mode & PERM_BITS_UMASK
            expected_perms = p[perm]
            assert_equal(expected_perms, actual_perms)

        # Remove any leftover rpc{user|password} config options from previous tests
        self.nodes[1].replace_in_config([("rpcuser", "#rpcuser"), ("rpcpassword", "#rpcpassword")])

        self.log.info('Check default cookie permission')
        test_perm(None)

        self.log.info('Check custom cookie permissions')
        for perm in ["owner", "group", "all"]:
            test_perm(perm)

    def test_norpccookiefile(self, node0_cookie_path):
        assert self.nodes[0].is_node_stopped(), "We expect previous test to stopped the node"
        assert not node0_cookie_path.exists()

        self.log.info('Starting with -norpccookiefile')
        # Start, but don't wait for RPC connection as TestNode.wait_for_rpc_connection() requires the cookie.
        with self.nodes[0].busy_wait_for_debug_log([b'init message: Done loading']):
            self.nodes[0].start(extra_args=["-norpccookiefile"])
        assert not node0_cookie_path.exists()

        self.log.info('Testing user/password authentication still works without cookie file')
        assert_equal(200, call_with_auth(self.nodes[0], "rt", self.rtpassword).status)
        # After confirming that we could log in, check that cookie file does not exist.
        assert not node0_cookie_path.exists()

        # Need to shut down in slightly unorthodox way since cookie auth can't be used
        assert_equal(200, call_with_auth(self.nodes[0], "rt", self.rtpassword, method="stop").status)
        self.nodes[0].wait_until_stopped()

    def run_test(self):
        self.conf_setup()
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

        self.log.info('Check blank -rpcauth is ignored')
        rpcauth_abc = '-rpcauth=abc:$2e32c2f20c67e29c328dd64a4214180f18da9e667d67c458070fd856f1e9e5e7'
        rpcauth_def = '-rpcauth=def:$fd7adb152c05ef80dccf50a1fa4c05d5a3ec6da95575fc312ae7c5d091836351'
        self.restart_node(0, extra_args=['-rpcauth'])
        self.restart_node(0, extra_args=['-rpcauth=', rpcauth_abc])
        self.restart_node(0, extra_args=[rpcauth_def, '-rpcauth='])
        # ...without disrupting usage of other -rpcauth tokens
        assert_equal(200, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        for info in self.authinfo:
            assert_equal(200, call_with_auth(self.nodes[0], *info).status)

        self.log.info('Check -norpcauth disables all previous -rpcauth* params')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth'])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(401, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        for info in self.authinfo:
            assert_equal(401, call_with_auth(self.nodes[0], *info).status)

        self.log.info('Check -norpcauth can be reversed with -rpcauth')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', '-rpcauth'])
        # FIXME: assert_equal(200, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        for info in self.authinfo:
            assert_equal(200, call_with_auth(self.nodes[0], *info).status)

        self.log.info('Check -norpcauth followed by a specific -rpcauth=* restores config file -rpcauth=* values too')
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', rpcauth_abc])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        for info in self.authinfo:
            assert_equal(200, call_with_auth(self.nodes[0], *info).status)
        self.restart_node(0, extra_args=[rpcauth_def, '-norpcauth', '-rpcauth='])
        assert_equal(401, call_with_auth(self.nodes[0], 'def', 'abc').status)
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        for info in self.authinfo:
            assert_equal(200, call_with_auth(self.nodes[0], *info).status)

        self.log.info('Check -rpcauth are validated')
        self.stop_node(0)
        self.log.info('Check malformed -rpcauth')
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo:bar'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo:bar:baz'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo$bar:baz'])
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error, extra_args=['-rpcauth=foo$bar$baz'])

        # pw = bitcoin
        rpcauth_user1 = '-rpcauth=user1:6dd184e5e69271fdd69103464630014f$eb3d7ce67c4d1ff3564270519b03b636c0291012692a5fa3dd1d2075daedd07b'
        rpcauth_user2 = '-rpcauth=user2:57b2f77c919eece63cfa46c2f06e46ae$266b63902f99f97eeaab882d4a87f8667ab84435c3799f2ce042ef5a994d620b'

        self.log.info('Check -norpcauth disables previous -rpcauth params')
        self.restart_node(0, extra_args=[rpcauth_user1, rpcauth_user2, '-norpcauth'])
        assert_equal(401, call_with_auth(self.nodes[0], 'user1', 'bitcoin').status)
        assert_equal(401, call_with_auth(self.nodes[0], 'rt', self.rtpassword).status)
        self.stop_node(0)

        self.log.info('Check that failure to write cookie file will abort the node gracefully')
        cookie_path =     self.nodes[0].chain_path / ".cookie"
        cookie_path_tmp = self.nodes[0].chain_path / ".cookie.tmp"
        cookie_path_tmp.mkdir()
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error)
        cookie_path_tmp.rmdir()
        assert not cookie_path.exists()
        self.restart_node(0)
        assert cookie_path.exists()
        self.stop_node(0)

        self.test_rpccookieperms()

        self.test_norpccookiefile(cookie_path)

if __name__ == '__main__':
    HTTPBasicsTest(__file__).main()
