#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
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

    def setup_chain(self):
        super().setup_chain()
        #Append rpcauth to bitcoin.conf before initialization
        rpcauth = "rpcauth=rt:93648e835a54c573682c2eb19f882535$7681e9c5b74bdd85e78166031d2058e1069b3ed7ed967c93fc63abba06f31144"
        rpcauth2 = "rpcauth=rt2:f8607b1a88861fac29dfccf9b52ff9f$ff36a0c23c8c62b4846112e50fa888416e94c17bfd4c42f88fd8f55ec6a3137e"
        rpcuser = "rpcuser=rpcuser💻"
        rpcpassword = "rpcpassword=rpcpassword🔑"

        self.user = ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        gen_rpcauth = config['environment']['RPCAUTH']
        p = subprocess.Popen([sys.executable, gen_rpcauth, self.user], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        rpcauth3 = lines[1]
        self.password = lines[3]

        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write(rpcauth+"\n")
            f.write(rpcauth2+"\n")
            f.write(rpcauth3+"\n")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 1), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write(rpcuser+"\n")
            f.write(rpcpassword+"\n")

    def run_test(self):

        ##################################################
        # Check correctness of the rpcauth config option #
        ##################################################
        url = urllib.parse.urlparse(self.nodes[0].url)

        password = "cA773lm788buwYe4g4WT+05pKyNruVKjQ25x3n0DQcM="
        password2 = "8/F3uMDw4KSEbw96U3CA1C4X05dkHDN2BPFjTgZW4KI="

        self.log.info('Correct...')
        assert_equal(200, call_with_auth(self.nodes[0], url.username, url.password).status)

        #Use new authpair to confirm both work
        self.log.info('Correct...')
        assert_equal(200, call_with_auth(self.nodes[0], 'rt', password).status)

        #Wrong login name with rt's password
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[0], 'rtwrong', password).status)

        #Wrong password for rt
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[0], 'rt', password+'wrong').status)

        #Correct for rt2
        self.log.info('Correct...')
        assert_equal(200, call_with_auth(self.nodes[0], 'rt2', password2).status)

        #Wrong password for rt2
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[0], 'rt2', password2+'wrong').status)

        #Correct for randomly generated user
        self.log.info('Correct...')
        assert_equal(200, call_with_auth(self.nodes[0], self.user, self.password).status)

        #Wrong password for randomly generated user
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[0], self.user, self.password+'Wrong').status)

        ###############################################################
        # Check correctness of the rpcuser/rpcpassword config options #
        ###############################################################
        url = urllib.parse.urlparse(self.nodes[1].url)

        # rpcuser and rpcpassword authpair
        self.log.info('Correct...')
        assert_equal(200, call_with_auth(self.nodes[1], "rpcuser💻", "rpcpassword🔑").status)

        #Wrong login name with rpcuser's password
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[1], 'rpcuserwrong', 'rpcpassword').status)

        #Wrong password for rpcuser
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(self.nodes[1], 'rpcuser', 'rpcpasswordwrong').status)

if __name__ == '__main__':
    HTTPBasicsTest ().main ()
