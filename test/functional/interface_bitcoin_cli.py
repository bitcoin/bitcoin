#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-cli"""

from decimal import Decimal
import re

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_process_error,
    assert_raises_rpc_error,
    get_auth_cookie,
)
import time

# The block reward of coinbaseoutput.nValue (50) BTC/block matures after
# COINBASE_MATURITY (100) blocks. Therefore, after mining 101 blocks we expect
# node 0 to have a balance of (BLOCKS - COINBASE_MATURITY) * 50 BTC/block.
BLOCKS = COINBASE_MATURITY + 1
BALANCE = (BLOCKS - 100) * 50

JSON_PARSING_ERROR = 'error: Error parsing JSON: foo'
BLOCKS_VALUE_OF_ZERO = 'error: the first argument (number of blocks to generate, default: 1) must be an integer value greater than zero'
TOO_MANY_ARGS = 'error: too many arguments (maximum 2 for nblocks and maxtries)'
WALLET_NOT_LOADED = 'Requested wallet does not exist or is not loaded'
WALLET_NOT_SPECIFIED = 'Wallet file not specified'


def cli_get_info_string_to_dict(cli_get_info_string):
    """Helper method to convert human-readable -getinfo into a dictionary"""
    cli_get_info = {}
    lines = cli_get_info_string.splitlines()
    line_idx = 0
    ansi_escape = re.compile(r'(\x9B|\x1B\[)[0-?]*[ -\/]*[@-~]')
    while line_idx < len(lines):
        # Remove ansi colour code
        line = ansi_escape.sub('', lines[line_idx])
        if "Balances" in line:
            # When "Balances" appears in a line, all of the following lines contain "balance: wallet" until an empty line
            cli_get_info["Balances"] = {}
            while line_idx < len(lines) and not (lines[line_idx + 1] == ''):
                line_idx += 1
                balance, wallet = lines[line_idx].strip().split(" ")
                # Remove right justification padding
                wallet = wallet.strip()
                if wallet == '""':
                    # Set default wallet("") to empty string
                    wallet = ''
                cli_get_info["Balances"][wallet] = balance.strip()
        elif ": " in line:
            key, value = line.split(": ")
            if key == 'Wallet' and value == '""':
                # Set default wallet("") to empty string
                value = ''
            if key == "Proxy" and value == "N/A":
                # Set N/A to empty string to represent no proxy
                value = ''
            cli_get_info[key.strip()] = value.strip()
        line_idx += 1
    return cli_get_info


class TestBitcoinCli(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        if self.is_wallet_compiled():
            self.requires_wallet = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_cli()

    def run_test(self):
        """Main test logic"""
        self.nodes[0].generate(BLOCKS)

        self.log.info("Compare responses from getblockchaininfo RPC and `bitcoin-cli getblockchaininfo`")
        cli_response = self.nodes[0].cli.getblockchaininfo()
        rpc_response = self.nodes[0].getblockchaininfo()
        assert_equal(cli_response, rpc_response)

        user, password = get_auth_cookie(self.nodes[0].datadir, self.chain)

        self.log.info("Test -stdinrpcpass option")
        assert_equal(BLOCKS, self.nodes[0].cli('-rpcuser={}'.format(user), '-stdinrpcpass', input=password).getblockcount())
        assert_raises_process_error(1, 'Incorrect rpcuser or rpcpassword', self.nodes[0].cli('-rpcuser={}'.format(user), '-stdinrpcpass', input='foo').echo)

        self.log.info("Test -stdin and -stdinrpcpass")
        assert_equal(['foo', 'bar'], self.nodes[0].cli('-rpcuser={}'.format(user), '-stdin', '-stdinrpcpass', input=password + '\nfoo\nbar').echo())
        assert_raises_process_error(1, 'Incorrect rpcuser or rpcpassword', self.nodes[0].cli('-rpcuser={}'.format(user), '-stdin', '-stdinrpcpass', input='foo').echo)

        self.log.info("Test connecting to a non-existing server")
        assert_raises_process_error(1, "Could not connect to the server", self.nodes[0].cli('-rpcport=1').echo)

        self.log.info("Test connecting with non-existing RPC cookie file")
        assert_raises_process_error(1, "Could not locate RPC credentials", self.nodes[0].cli('-rpccookiefile=does-not-exist', '-rpcpassword=').echo)

        self.log.info("Test -getinfo with arguments fails")
        assert_raises_process_error(1, "-getinfo takes no arguments", self.nodes[0].cli('-getinfo').help)

        self.log.info("Test -getinfo with -color=never does not return ANSI escape codes")
        assert "\u001b[0m" not in self.nodes[0].cli('-getinfo', '-color=never').send_cli()

        self.log.info("Test -getinfo with -color=always returns ANSI escape codes")
        assert "\u001b[0m" in self.nodes[0].cli('-getinfo', '-color=always').send_cli()

        self.log.info("Test -getinfo with invalid value for -color option")
        assert_raises_process_error(1, "Invalid value for -color option. Valid values: always, auto, never.", self.nodes[0].cli('-getinfo', '-color=foo').send_cli)

        self.log.info("Test -getinfo returns expected network and blockchain info")
        if self.is_wallet_compiled():
            self.nodes[0].encryptwallet(password)
        cli_get_info_string = self.nodes[0].cli('-getinfo').send_cli()
        cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)

        network_info = self.nodes[0].getnetworkinfo()
        blockchain_info = self.nodes[0].getblockchaininfo()
        assert_equal(int(cli_get_info['Version']), network_info['version'])
        assert_equal(cli_get_info['Verification progress'], "%.4f%%" % (blockchain_info['verificationprogress'] * 100))
        assert_equal(int(cli_get_info['Blocks']), blockchain_info['blocks'])
        assert_equal(int(cli_get_info['Headers']), blockchain_info['headers'])
        assert_equal(int(cli_get_info['Time offset (s)']), network_info['timeoffset'])
        expected_network_info = f"in {network_info['connections_in']}, out {network_info['connections_out']}, total {network_info['connections']}"
        assert_equal(cli_get_info["Network"], expected_network_info)
        assert_equal(cli_get_info['Proxy'], network_info['networks'][0]['proxy'])
        assert_equal(Decimal(cli_get_info['Difficulty']), blockchain_info['difficulty'])
        assert_equal(cli_get_info['Chain'], blockchain_info['chain'])

        if self.is_wallet_compiled():
            self.log.info("Test -getinfo and bitcoin-cli getwalletinfo return expected wallet info")
            assert_equal(Decimal(cli_get_info['Balance']), BALANCE)
            assert 'Balances' not in cli_get_info_string
            wallet_info = self.nodes[0].getwalletinfo()
            assert_equal(int(cli_get_info['Keypool size']), wallet_info['keypoolsize'])
            assert_equal(int(cli_get_info['Unlocked until']), wallet_info['unlocked_until'])
            assert_equal(Decimal(cli_get_info['Transaction fee rate (-paytxfee) (BTC/kvB)']), wallet_info['paytxfee'])
            assert_equal(Decimal(cli_get_info['Min tx relay fee rate (BTC/kvB)']), network_info['relayfee'])
            assert_equal(self.nodes[0].cli.getwalletinfo(), wallet_info)

            # Setup to test -getinfo, -generate, and -rpcwallet= with multiple wallets.
            wallets = [self.default_wallet_name, 'Encrypted', 'secret']
            amounts = [BALANCE + Decimal('9.999928'), Decimal(9), Decimal(31)]
            self.nodes[0].createwallet(wallet_name=wallets[1])
            self.nodes[0].createwallet(wallet_name=wallets[2])
            w1 = self.nodes[0].get_wallet_rpc(wallets[0])
            w2 = self.nodes[0].get_wallet_rpc(wallets[1])
            w3 = self.nodes[0].get_wallet_rpc(wallets[2])
            rpcwallet2 = '-rpcwallet={}'.format(wallets[1])
            rpcwallet3 = '-rpcwallet={}'.format(wallets[2])
            w1.walletpassphrase(password, self.rpc_timeout)
            w2.encryptwallet(password)
            w1.sendtoaddress(w2.getnewaddress(), amounts[1])
            w1.sendtoaddress(w3.getnewaddress(), amounts[2])

            # Mine a block to confirm; adds a block reward (50 BTC) to the default wallet.
            self.nodes[0].generate(1)

            self.log.info("Test -getinfo with multiple wallets and -rpcwallet returns specified wallet balance")
            for i in range(len(wallets)):
                cli_get_info_string = self.nodes[0].cli('-getinfo', '-rpcwallet={}'.format(wallets[i])).send_cli()
                cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)
                assert 'Balances' not in cli_get_info_string
                assert_equal(cli_get_info["Wallet"], wallets[i])
                assert_equal(Decimal(cli_get_info['Balance']), amounts[i])

            self.log.info("Test -getinfo with multiple wallets and -rpcwallet=non-existing-wallet returns no balances")
            cli_get_info_string = self.nodes[0].cli('-getinfo', '-rpcwallet=does-not-exist').send_cli()
            assert 'Balance' not in cli_get_info_string
            assert 'Balances' not in cli_get_info_string

            self.log.info("Test -getinfo with multiple wallets returns all loaded wallet names and balances")
            assert_equal(set(self.nodes[0].listwallets()), set(wallets))
            cli_get_info_string = self.nodes[0].cli('-getinfo').send_cli()
            cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)
            assert 'Balance' not in cli_get_info
            for k, v in zip(wallets, amounts):
                assert_equal(Decimal(cli_get_info['Balances'][k]), v)

            # Unload the default wallet and re-verify.
            self.nodes[0].unloadwallet(wallets[0])
            assert wallets[0] not in self.nodes[0].listwallets()
            cli_get_info_string = self.nodes[0].cli('-getinfo').send_cli()
            cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)
            assert 'Balance' not in cli_get_info
            assert 'Balances' in cli_get_info_string
            for k, v in zip(wallets[1:], amounts[1:]):
                assert_equal(Decimal(cli_get_info['Balances'][k]), v)
            assert wallets[0] not in cli_get_info

            self.log.info("Test -getinfo after unloading all wallets except a non-default one returns its balance")
            self.nodes[0].unloadwallet(wallets[2])
            assert_equal(self.nodes[0].listwallets(), [wallets[1]])
            cli_get_info_string = self.nodes[0].cli('-getinfo').send_cli()
            cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)
            assert 'Balances' not in cli_get_info_string
            assert_equal(cli_get_info['Wallet'], wallets[1])
            assert_equal(Decimal(cli_get_info['Balance']), amounts[1])

            self.log.info("Test -getinfo with -rpcwallet=remaining-non-default-wallet returns only its balance")
            cli_get_info_string = self.nodes[0].cli('-getinfo', rpcwallet2).send_cli()
            cli_get_info = cli_get_info_string_to_dict(cli_get_info_string)
            assert 'Balances' not in cli_get_info_string
            assert_equal(cli_get_info['Wallet'], wallets[1])
            assert_equal(Decimal(cli_get_info['Balance']), amounts[1])

            self.log.info("Test -getinfo with -rpcwallet=unloaded wallet returns no balances")
            cli_get_info_string = self.nodes[0].cli('-getinfo', rpcwallet3).send_cli()
            cli_get_info_keys = cli_get_info_string_to_dict(cli_get_info_string)
            assert 'Balance' not in cli_get_info_keys
            assert 'Balances' not in cli_get_info_string

            # Test bitcoin-cli -generate.
            n1 = 3
            n2 = 4
            w2.walletpassphrase(password, self.rpc_timeout)
            blocks = self.nodes[0].getblockcount()

            self.log.info('Test -generate with no args')
            generate = self.nodes[0].cli('-generate').send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), 1)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1)

            self.log.info('Test -generate with bad args')
            assert_raises_process_error(1, JSON_PARSING_ERROR, self.nodes[0].cli('-generate', 'foo').echo)
            assert_raises_process_error(1, BLOCKS_VALUE_OF_ZERO, self.nodes[0].cli('-generate', 0).echo)
            assert_raises_process_error(1, TOO_MANY_ARGS, self.nodes[0].cli('-generate', 1, 2, 3).echo)

            self.log.info('Test -generate with nblocks')
            generate = self.nodes[0].cli('-generate', n1).send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), n1)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1 + n1)

            self.log.info('Test -generate with nblocks and maxtries')
            generate = self.nodes[0].cli('-generate', n2, 1000000).send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), n2)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1 + n1 + n2)

            self.log.info('Test -generate -rpcwallet in single-wallet mode')
            generate = self.nodes[0].cli(rpcwallet2, '-generate').send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), 1)
            assert_equal(self.nodes[0].getblockcount(), blocks + 2 + n1 + n2)

            self.log.info('Test -generate -rpcwallet=unloaded wallet raises RPC error')
            assert_raises_rpc_error(-18, WALLET_NOT_LOADED, self.nodes[0].cli(rpcwallet3, '-generate').echo)
            assert_raises_rpc_error(-18, WALLET_NOT_LOADED, self.nodes[0].cli(rpcwallet3, '-generate', 'foo').echo)
            assert_raises_rpc_error(-18, WALLET_NOT_LOADED, self.nodes[0].cli(rpcwallet3, '-generate', 0).echo)
            assert_raises_rpc_error(-18, WALLET_NOT_LOADED, self.nodes[0].cli(rpcwallet3, '-generate', 1, 2, 3).echo)

            # Test bitcoin-cli -generate with -rpcwallet in multiwallet mode.
            self.nodes[0].loadwallet(wallets[2])
            n3 = 4
            n4 = 10
            blocks = self.nodes[0].getblockcount()

            self.log.info('Test -generate -rpcwallet with no args')
            generate = self.nodes[0].cli(rpcwallet2, '-generate').send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), 1)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1)

            self.log.info('Test -generate -rpcwallet with bad args')
            assert_raises_process_error(1, JSON_PARSING_ERROR, self.nodes[0].cli(rpcwallet2, '-generate', 'foo').echo)
            assert_raises_process_error(1, BLOCKS_VALUE_OF_ZERO, self.nodes[0].cli(rpcwallet2, '-generate', 0).echo)
            assert_raises_process_error(1, TOO_MANY_ARGS, self.nodes[0].cli(rpcwallet2, '-generate', 1, 2, 3).echo)

            self.log.info('Test -generate -rpcwallet with nblocks')
            generate = self.nodes[0].cli(rpcwallet2, '-generate', n3).send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), n3)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1 + n3)

            self.log.info('Test -generate -rpcwallet with nblocks and maxtries')
            generate = self.nodes[0].cli(rpcwallet2, '-generate', n4, 1000000).send_cli()
            assert_equal(set(generate.keys()), {'address', 'blocks'})
            assert_equal(len(generate["blocks"]), n4)
            assert_equal(self.nodes[0].getblockcount(), blocks + 1 + n3 + n4)

            self.log.info('Test -generate without -rpcwallet in multiwallet mode raises RPC error')
            assert_raises_rpc_error(-19, WALLET_NOT_SPECIFIED, self.nodes[0].cli('-generate').echo)
            assert_raises_rpc_error(-19, WALLET_NOT_SPECIFIED, self.nodes[0].cli('-generate', 'foo').echo)
            assert_raises_rpc_error(-19, WALLET_NOT_SPECIFIED, self.nodes[0].cli('-generate', 0).echo)
            assert_raises_rpc_error(-19, WALLET_NOT_SPECIFIED, self.nodes[0].cli('-generate', 1, 2, 3).echo)
        else:
            self.log.info("*** Wallet not compiled; cli getwalletinfo and -getinfo wallet tests skipped")
            self.nodes[0].generate(25)  # maintain block parity with the wallet_compiled conditional branch

        self.log.info("Test -version with node stopped")
        self.stop_node(0)
        cli_response = self.nodes[0].cli('-version').send_cli()
        assert "{} RPC client version".format(self.config['environment']['PACKAGE_NAME']) in cli_response

        self.log.info("Test -rpcwait option successfully waits for RPC connection")
        self.nodes[0].start()  # start node without RPC connection
        self.nodes[0].wait_for_cookie_credentials()  # ensure cookie file is available to avoid race condition
        blocks = self.nodes[0].cli('-rpcwait').send_cli('getblockcount')
        self.nodes[0].wait_for_rpc_connection()
        assert_equal(blocks, BLOCKS + 25)

        self.log.info("Test -rpcwait option waits at most -rpcwaittimeout seconds for startup")
        self.stop_node(0)  # stop the node so we time out
        start_time = time.time()
        assert_raises_process_error(1, "Could not connect to the server", self.nodes[0].cli('-rpcwait', '-rpcwaittimeout=5').echo)
        assert_greater_than_or_equal(time.time(), start_time + 5)


if __name__ == '__main__':
    TestBitcoinCli().main()
