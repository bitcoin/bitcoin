#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-wallet."""

import hashlib
import os
import stat
import subprocess
import textwrap

from collections import OrderedDict

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
        default_args = ['-datadir={}'.format(self.nodes[0].datadir), '-chain=%s' % self.chain]
        if self.options.descriptors and 'create' in args:
            default_args.append('-descriptors')

        return subprocess.Popen([binary] + default_args + list(args), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

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
            for n in iter(lambda: f.readinto(mv), 0):
                h.update(mv[:n])
        return h.hexdigest()

    def wallet_timestamp(self):
        return os.path.getmtime(self.wallet_path)

    def wallet_permissions(self):
        return oct(os.lstat(self.wallet_path).st_mode)[-3:]

    def log_wallet_timestamp_comparison(self, old, new):
        result = 'unchanged' if new == old else 'increased!'
        self.log.debug('Wallet file timestamp {}'.format(result))

    def get_expected_info_output(self, name="", transactions=0, keypool=2, address=0):
        wallet_name = self.default_wallet_name if name == "" else name
        output_types = 3  # p2pkh, p2sh, segwit
        if self.options.descriptors:
            return textwrap.dedent('''\
                Wallet info
                ===========
                Name: %s
                Format: sqlite
                Descriptors: yes
                Encrypted: no
                HD (hd seed available): yes
                Keypool Size: %d
                Transactions: %d
                Address Book: %d
            ''' % (wallet_name, keypool * output_types, transactions, address))
        else:
            return textwrap.dedent('''\
                Wallet info
                ===========
                Name: %s
                Format: bdb
                Descriptors: no
                Encrypted: no
                HD (hd seed available): yes
                Keypool Size: %d
                Transactions: %d
                Address Book: %d
            ''' % (wallet_name, keypool, transactions, address * output_types))

    def read_dump(self, filename):
        dump = OrderedDict()
        with open(filename, "r", encoding="utf8") as f:
            for row in f:
                row = row.strip()
                key, value = row.split(',')
                dump[key] = value
        return dump

    def assert_is_sqlite(self, filename):
        with open(filename, 'rb') as f:
            file_magic = f.read(16)
            assert file_magic == b'SQLite format 3\x00'

    def assert_is_bdb(self, filename):
        with open(filename, 'rb') as f:
            f.seek(12, 0)
            file_magic = f.read(4)
            assert file_magic == b'\x00\x05\x31\x62' or file_magic == b'\x62\x31\x05\x00'

    def write_dump(self, dump, filename, magic=None, skip_checksum=False):
        if magic is None:
            magic = "BITCOIN_CORE_WALLET_DUMP"
        with open(filename, "w", encoding="utf8") as f:
            row = ",".join([magic, dump[magic]]) + "\n"
            f.write(row)
            for k, v in dump.items():
                if k == magic or k == "checksum":
                    continue
                row = ",".join([k, v]) + "\n"
                f.write(row)
            if not skip_checksum:
                row = ",".join(["checksum", dump["checksum"]]) + "\n"
                f.write(row)

    def assert_dump(self, expected, received):
        e = expected.copy()
        r = received.copy()

        # BDB will add a "version" record that is not present in sqlite
        # In that case, we should ignore this record in both
        # But because this also effects the checksum, we also need to drop that.
        v_key = "0776657273696f6e" # Version key
        if v_key in e and v_key not in r:
            del e[v_key]
            del e["checksum"]
            del r["checksum"]
        if v_key not in e and v_key in r:
            del r[v_key]
            del e["checksum"]
            del r["checksum"]

        assert_equal(len(e), len(r))
        for k, v in e.items():
            assert_equal(v, r[k])

    def do_tool_createfromdump(self, wallet_name, dumpfile, file_format=None):
        dumppath = os.path.join(self.nodes[0].datadir, dumpfile)
        rt_dumppath = os.path.join(self.nodes[0].datadir, "rt-{}.dump".format(wallet_name))

        dump_data = self.read_dump(dumppath)

        args = ["-wallet={}".format(wallet_name),
                "-dumpfile={}".format(dumppath)]
        if file_format is not None:
            args.append("-format={}".format(file_format))
        args.append("createfromdump")

        load_output = ""
        if file_format is not None and file_format != dump_data["format"]:
            load_output += "Warning: Dumpfile wallet format \"{}\" does not match command line specified format \"{}\".\n".format(dump_data["format"], file_format)
        self.assert_tool_output(load_output, *args)
        assert os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", wallet_name))

        self.assert_tool_output("The dumpfile may contain private keys. To ensure the safety of your Bitcoin, do not share the dumpfile.\n", '-wallet={}'.format(wallet_name), '-dumpfile={}'.format(rt_dumppath), 'dump')

        rt_dump_data = self.read_dump(rt_dumppath)
        wallet_dat = os.path.join(self.nodes[0].datadir, "regtest/wallets/", wallet_name, "wallet.dat")
        if rt_dump_data["format"] == "bdb":
            self.assert_is_bdb(wallet_dat)
        else:
            self.assert_is_sqlite(wallet_dat)

    def test_invalid_tool_commands_and_args(self):
        self.log.info('Testing that various invalid commands raise with specific error messages')
        self.assert_raises_tool_error("Error parsing command line arguments: Invalid command 'foo'", 'foo')
        # `bitcoin-wallet help` raises an error. Use `bitcoin-wallet -help`.
        self.assert_raises_tool_error("Error parsing command line arguments: Invalid command 'help'", 'help')
        self.assert_raises_tool_error('Error: Additional arguments provided (create). Methods do not take arguments. Please refer to `-help`.', 'info', 'create')
        self.assert_raises_tool_error('Error parsing command line arguments: Invalid parameter -foo', '-foo')
        self.assert_raises_tool_error('No method provided. Run `bitcoin-wallet -help` for valid methods.')
        self.assert_raises_tool_error('Wallet name must be provided when creating a new wallet.', 'create')
        locked_dir = os.path.join(self.options.tmpdir, "node0", "regtest", "wallets")
        error = 'Error initializing wallet database environment "{}"!'.format(locked_dir)
        if self.options.descriptors:
            error = "SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another bitcoind?"
        self.assert_raises_tool_error(
            error,
            '-wallet=' + self.default_wallet_name,
            'info',
        )
        path = os.path.join(self.options.tmpdir, "node0", "regtest", "wallets", "nonexistent.dat")
        self.assert_raises_tool_error("Failed to load database path '{}'. Path does not exist.".format(path), '-wallet=nonexistent.dat', 'info')

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
        # assert self.wallet_permissions() in ['400', '666'] # Sanity check. 666 because Appveyor.
        # shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling info: {}'.format(timestamp_before))
        out = self.get_expected_info_output(address=1)
        self.assert_tool_output(out, '-wallet=' + self.default_wallet_name, 'info')
        timestamp_after = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp after calling info: {}'.format(timestamp_after))
        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        self.log.debug('Setting wallet file permissions back to 600 (read/write)')
        os.chmod(self.wallet_path, stat.S_IRUSR | stat.S_IWUSR)
        assert self.wallet_permissions() in ['600', '666']  # Sanity check. 666 because Appveyor.
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
        self.generate(self.nodes[0], 1)
        self.stop_node(0)

        self.log.info('Calling wallet tool info after generating a transaction, testing output')
        shasum_before = self.wallet_shasum()
        timestamp_before = self.wallet_timestamp()
        self.log.debug('Wallet file timestamp before calling info: {}'.format(timestamp_before))
        out = self.get_expected_info_output(transactions=1, address=1)
        self.assert_tool_output(out, '-wallet=' + self.default_wallet_name, 'info')
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
        out = "Topping up keypool...\n" + self.get_expected_info_output(name="foo", keypool=2000)
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
        self.start_node(0, ['-nowallet', '-wallet=foo'])

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
        if not self.options.descriptors:
            assert_equal(1000, out['keypoolsize'])
            assert_equal(1000, out['keypoolsize_hd_internal'])
            assert_equal(True, 'hdseedid' in out)
        else:
            assert_equal(3000, out['keypoolsize'])
            assert_equal(3000, out['keypoolsize_hd_internal'])

        self.log_wallet_timestamp_comparison(timestamp_before, timestamp_after)
        assert_equal(timestamp_before, timestamp_after)
        assert_equal(shasum_after, shasum_before)
        self.log.debug('Wallet file shasum unchanged\n')

    def test_salvage(self):
        # TODO: Check salvage actually salvages and doesn't break things. https://github.com/bitcoin/bitcoin/issues/7463
        self.log.info('Check salvage')
        self.start_node(0)
        self.nodes[0].createwallet("salvage")
        self.stop_node(0)

        self.assert_tool_output('', '-wallet=salvage', 'salvage')

    def test_dump_createfromdump(self):
        self.start_node(0)
        self.nodes[0].createwallet("todump")
        file_format = self.nodes[0].get_wallet_rpc("todump").getwalletinfo()["format"]
        self.nodes[0].createwallet("todump2")
        self.stop_node(0)

        self.log.info('Checking dump arguments')
        self.assert_raises_tool_error('No dump file provided. To use dump, -dumpfile=<filename> must be provided.', '-wallet=todump', 'dump')

        self.log.info('Checking basic dump')
        wallet_dump = os.path.join(self.nodes[0].datadir, "wallet.dump")
        self.assert_tool_output('The dumpfile may contain private keys. To ensure the safety of your Bitcoin, do not share the dumpfile.\n', '-wallet=todump', '-dumpfile={}'.format(wallet_dump), 'dump')

        dump_data = self.read_dump(wallet_dump)
        orig_dump = dump_data.copy()
        # Check the dump magic
        assert_equal(dump_data['BITCOIN_CORE_WALLET_DUMP'], '1')
        # Check the file format
        assert_equal(dump_data["format"], file_format)

        self.log.info('Checking that a dumpfile cannot be overwritten')
        self.assert_raises_tool_error('File {} already exists. If you are sure this is what you want, move it out of the way first.'.format(wallet_dump),  '-wallet=todump2', '-dumpfile={}'.format(wallet_dump), 'dump')

        self.log.info('Checking createfromdump arguments')
        self.assert_raises_tool_error('No dump file provided. To use createfromdump, -dumpfile=<filename> must be provided.', '-wallet=todump', 'createfromdump')
        non_exist_dump = os.path.join(self.nodes[0].datadir, "wallet.nodump")
        self.assert_raises_tool_error('Unknown wallet file format "notaformat" provided. Please provide one of "bdb" or "sqlite".', '-wallet=todump', '-format=notaformat', '-dumpfile={}'.format(wallet_dump), 'createfromdump')
        self.assert_raises_tool_error('Dump file {} does not exist.'.format(non_exist_dump), '-wallet=todump', '-dumpfile={}'.format(non_exist_dump), 'createfromdump')
        wallet_path = os.path.join(self.nodes[0].datadir, 'regtest/wallets/todump2')
        self.assert_raises_tool_error('Failed to create database path \'{}\'. Database already exists.'.format(wallet_path), '-wallet=todump2', '-dumpfile={}'.format(wallet_dump), 'createfromdump')
        self.assert_raises_tool_error("The -descriptors option can only be used with the 'create' command.", '-descriptors', '-wallet=todump2', '-dumpfile={}'.format(wallet_dump), 'createfromdump')

        self.log.info('Checking createfromdump')
        self.do_tool_createfromdump("load", "wallet.dump")
        if self.is_bdb_compiled():
            self.do_tool_createfromdump("load-bdb", "wallet.dump", "bdb")
        if self.is_sqlite_compiled():
            self.do_tool_createfromdump("load-sqlite", "wallet.dump", "sqlite")

        self.log.info('Checking createfromdump handling of magic and versions')
        bad_ver_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_ver1.dump")
        dump_data["BITCOIN_CORE_WALLET_DUMP"] = "0"
        self.write_dump(dump_data, bad_ver_wallet_dump)
        self.assert_raises_tool_error('Error: Dumpfile version is not supported. This version of bitcoin-wallet only supports version 1 dumpfiles. Got dumpfile with version 0', '-wallet=badload', '-dumpfile={}'.format(bad_ver_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))
        bad_ver_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_ver2.dump")
        dump_data["BITCOIN_CORE_WALLET_DUMP"] = "2"
        self.write_dump(dump_data, bad_ver_wallet_dump)
        self.assert_raises_tool_error('Error: Dumpfile version is not supported. This version of bitcoin-wallet only supports version 1 dumpfiles. Got dumpfile with version 2', '-wallet=badload', '-dumpfile={}'.format(bad_ver_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))
        bad_magic_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_magic.dump")
        del dump_data["BITCOIN_CORE_WALLET_DUMP"]
        dump_data["not_the_right_magic"] = "1"
        self.write_dump(dump_data, bad_magic_wallet_dump, "not_the_right_magic")
        self.assert_raises_tool_error('Error: Dumpfile identifier record is incorrect. Got "not_the_right_magic", expected "BITCOIN_CORE_WALLET_DUMP".', '-wallet=badload', '-dumpfile={}'.format(bad_magic_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))

        self.log.info('Checking createfromdump handling of checksums')
        bad_sum_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_sum1.dump")
        dump_data = orig_dump.copy()
        checksum = dump_data["checksum"]
        dump_data["checksum"] = "1" * 64
        self.write_dump(dump_data, bad_sum_wallet_dump)
        self.assert_raises_tool_error('Error: Dumpfile checksum does not match. Computed {}, expected {}'.format(checksum, "1" * 64), '-wallet=bad', '-dumpfile={}'.format(bad_sum_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))
        bad_sum_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_sum2.dump")
        del dump_data["checksum"]
        self.write_dump(dump_data, bad_sum_wallet_dump, skip_checksum=True)
        self.assert_raises_tool_error('Error: Missing checksum', '-wallet=badload', '-dumpfile={}'.format(bad_sum_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))
        bad_sum_wallet_dump = os.path.join(self.nodes[0].datadir, "wallet-bad_sum3.dump")
        dump_data["checksum"] = "2" * 10
        self.write_dump(dump_data, bad_sum_wallet_dump)
        self.assert_raises_tool_error('Error: Dumpfile checksum does not match. Computed {}, expected {}{}'.format(checksum, "2" * 10, "0" * 54), '-wallet=badload', '-dumpfile={}'.format(bad_sum_wallet_dump), 'createfromdump')
        assert not os.path.isdir(os.path.join(self.nodes[0].datadir, "regtest/wallets", "badload"))


    def run_test(self):
        self.wallet_path = os.path.join(self.nodes[0].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename)
        self.test_invalid_tool_commands_and_args()
        # Warning: The following tests are order-dependent.
        self.test_tool_wallet_info()
        self.test_tool_wallet_info_after_transaction()
        self.test_tool_wallet_create_on_existing_wallet()
        self.test_getwalletinfo_on_different_wallet()
        if not self.options.descriptors:
            # Salvage is a legacy wallet only thing
            self.test_salvage()
        self.test_dump_createfromdump()

if __name__ == '__main__':
    ToolWalletTest().main()
