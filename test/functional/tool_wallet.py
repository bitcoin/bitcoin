#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-wallet."""

import hashlib
import os
import stat
import subprocess
import textwrap

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

BUFFER_SIZE = 16 * 1024

class ToolWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_wallet_tool()

    def bitcoin_wallet_process(self, *args):
        binary = self.config["environment"]["BUILDDIR"] + '/src/bitcoin-wallet' + self.config["environment"]["EXEEXT"]
        args = ['-datadir={}'.format(self.nodes[0].datadir), '-chain=%s' % self.chain] + list(args)
        return subprocess.Popen([binary] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    def assert_raises_tool_error(self, error, *args):
        p = self.bitcoin_wallet_process(*args)
        stdout, stderr = p.communicate()
        assert_equal(p.poll(), 1)
        assert_equal(stdout, '')
        assert_equal(stderr.strip(), error)

    def assert_tool_output(self, output, *args):
        p = self.bitcoin_wallet_process(*args)
        stdout, stderr = p.communicate()
        assert_equal(stderr, '')
        assert_equal(stdout, output)
        assert_equal(p.poll(), 0)

    def wallet_shasum(self):
        h = hashlib.sha1()
        mv = memoryview(bytearray(BUFFER_SIZE))
        with open(self.wallet_path, 'rb', buffering=0) as f:
            for n in iter(lambda : f.readinto(mv), 0):
                h.update(mv[:n])
        return h.hexdigest()

    def wallet_timestamp(self):
        return os.path.getmtime(self.wallet_path)

    def wallet_permissions(self):
        return oct(os.lstat(self.wallet_path).st_mode)[-3:]

    def log_wallet_timestamp_comparison(self, old, new):
        result = 'unchanged' if new == old else 'increased!'
        self.log.debug('Wallet file timestamp {}'.format(result))

    def test_invalid_tool_commands_and_args(self):
        self.log.info('Testing that various invalid commands raise with specific error messages')
        self.assert_raises_tool_error('Invalid command: foo', 'foo')
        # `bitcoin-wallet help` raises an error. Use `bitcoin-wallet -help`.
        self.assert_raises_tool_error('Invalid command: help', 'help')
        self.assert_raises_tool_error('Error: two methods provided (info and create). Only one method should be provided.', 'info', 'create')
        self.assert_raises_tool_error('Error parsing command line arguments: Invalid parameter -foo', '-foo')
        self.assert_raises_tool_error('Error loading wallet.dat. Is wallet being used by other process?', '-wallet=wallet.dat', 'info')
        self.assert_raises_tool_error('Error: no wallet file at nonexistent.dat', '-wallet=nonexistent.dat', 'info')

    def test_tool_wallet_info(self):
        # Stop the node to close the wallet to call the info command.
        self.stop_node(0)
        self.log.info('Calling wallet tool info, testing output')
        #
        # TODO: Wallet tool info should work with wallet file permissions set to
        # read-only without raising:
        # "Error loading wallet.dat. Is wallet being used by another process?"
        # The following lines should be uncommented and the tests still succeed:
        #
        # self.log.debug('Setting wallet file permissions to 400 (read-only)')
        # os.chmod(self.wallet_path, stat.S_IRUSR)
        # assert(self.wallet_permissions() in ['400', '666']) # Sanity check. 666 because Appveyor.
        # shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling info: {}'.format(timestamp_before))
        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 0
            Address Book: 3
        ''')
        self.assert_tool_output(out, '-wallet=wallet.dat', 'info')
        timestamp_after = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp after calling info: {}'.format(timestamp_after))
        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        self.log.debug('Setting wallet file permissions back to 600 (read/write)')
        os.chmod(self.wallet_path, stat.S_IRUSR | stat.S_IWUSR)
        assert(self.wallet_permissions() in ['600', '666']) # Sanity check. 666 because Appveyor.
        #
        # TODO: Wallet tool info should not write to the wallet file.
        # The following lines should be uncommented and the tests still succeed:
        #
        # assert_equal(timestamp_before, timestamp_after)
        # shasum_after = self.wallet_shasum()
        # assert_equal(shasum_before, shasum_after)
        # self.log.debug('Wallet file shasum unchanged\n')

    def test_tool_wallet_info_after_transaction(self):
        """
        Mutate the wallet with a transaction to verify that the info command
        output changes accordingly.
        """
        self.start_node(0)
        self.log.info('Generating transaction to mutate wallet')
        self.nodes[0].generate(1)
        self.stop_node(0)

        self.log.info('Calling wallet tool info after generating a transaction, testing output')
        shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling info: {}'.format(timestamp_before))
        out = textwrap.dedent('''\
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2
            Transactions: 1
            Address Book: 3
        ''')
        self.assert_tool_output(out, '-wallet=wallet.dat', 'info')
        shasum_after = self.wallet_shasum()
        timestamp_after = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp after calling info: {}'.format(timestamp_after))
        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        #
        # TODO: Wallet tool info should not write to the wallet file.
        # This assertion should be uncommented and succeed:
        # assert_equal(timestamp_before, timestamp_after)
        assert_equal(shasum_before, shasum_after)
        self.log.debug('Wallet file shasum unchanged\n')

    def test_tool_wallet_create_on_existing_wallet(self):
        self.log.info('Calling wallet tool create on an existing wallet, testing output')
        shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling create: {}'.format(timestamp_before))
        out = textwrap.dedent('''\
            Topping up keypool...
            Wallet info
            ===========
            Encrypted: no
            HD (hd seed available): yes
            Keypool Size: 2000
            Transactions: 0
            Address Book: 0
        ''')
        self.assert_tool_output(out, '-wallet=foo', 'create')
        shasum_after = self.wallet_shasum()
        timestamp_after = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp after calling create: {}'.format(timestamp_after))
        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        assert_equal(timestamp_before, timestamp_after)
        assert_equal(shasum_before, shasum_after)
        self.log.debug('Wallet file shasum unchanged\n')

    def test_getwalletinfo_on_different_wallet(self):
        self.log.info('Starting node with arg -wallet=foo')
        self.start_node(0, ['-wallet=foo'])

        self.log.info('Calling getwalletinfo on a different wallet ("foo"), testing output')
        shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling getwalletinfo: {}'.format(timestamp_before))
        out = self.nodes[0].getwalletinfo()
        self.stop_node(0)

        shasum_after = self.wallet_shasum()
        timestamp_after = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp after calling getwalletinfo: {}'.format(timestamp_after))

        assert_equal(0, out['txcount'])
        assert_equal(1000, out['keypoolsize'])
        assert_equal(1000, out['keypoolsize_hd_internal'])
        assert_equal(True, 'hdseedid' in out)

        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        assert_equal(timestamp_before, timestamp_after)
        assert_equal(shasum_after, shasum_before)
        self.log.debug('Wallet file shasum unchanged\n')

    def run_test(self):
        self.wallet_path = os.path.join(self.nodes[0].datadir, self.chain, 'wallets', 'wallet.dat')
        self.test_invalid_tool_commands_and_args()
        # Warning: The following tests are order-dependent.
        self.test_tool_wallet_info()
        self.test_tool_wallet_info_after_transaction()
        self.test_tool_wallet_create_on_existing_wallet()
        self.test_getwalletinfo_on_different_wallet()


if __name__ == '__main__':
    ToolWalletTest().main()
