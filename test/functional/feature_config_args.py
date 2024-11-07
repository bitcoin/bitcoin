#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various command line arguments and configuration file parameters."""

import os
from pathlib import Path
import platform
import re
import tempfile
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework import util


class ConfArgsTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.supports_cli = False
        self.wallet_names = []
        self.disable_autoconnect = False

    def test_config_file_parser(self):
        self.log.info('Test config file parser')
        self.stop_node(0)

        # Check that startup fails if conf= is set in bitcoin.conf or in an included conf file
        bad_conf_file_path = self.nodes[0].datadir_path / "bitcoin_bad.conf"
        util.write_config(bad_conf_file_path, n=0, chain='', extra_config=f'conf=some.conf\n')
        conf_in_config_file_err = 'Error: Error reading configuration file: conf cannot be set in the configuration file; use includeconf= if you want to include additional config files'
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f'-conf={bad_conf_file_path}'],
            expected_msg=conf_in_config_file_err,
        )
        inc_conf_file_path = self.nodes[0].datadir_path / 'include.conf'
        with open(self.nodes[0].datadir_path / 'bitcoin.conf', 'a', encoding='utf-8') as conf:
            conf.write(f'includeconf={inc_conf_file_path}\n')
        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('conf=some.conf\n')
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=conf_in_config_file_err,
        )

        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Error parsing command line arguments: Invalid parameter -dash_cli=1',
            extra_args=['-dash_cli=1'],
        )
        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('dash_conf=1\n')

        with self.nodes[0].assert_debug_log(expected_msgs=['Ignoring unknown configuration value dash_conf']):
            self.start_node(0)
        self.stop_node(0)

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('reindex=1\n')

        with self.nodes[0].assert_debug_log(expected_msgs=['Warning: reindex=1 is set in the configuration file, which will significantly slow down startup. Consider removing or commenting out this option for better performance, unless there is currently a condition which makes rebuilding the indexes necessary']):
            self.start_node(0)
        self.stop_node(0)

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('-dash=1\n')
        self.nodes[0].assert_start_raises_init_error(expected_msg='Error: Error reading configuration file: parse error on line 1: -dash=1, options in configuration file must be specified without leading -')

        if self.is_wallet_compiled():
            with open(inc_conf_file_path, 'w', encoding='utf8') as conf:
                conf.write("wallet=foo\n")
            self.nodes[0].assert_start_raises_init_error(expected_msg=f'Error: Config setting for -wallet only applied on {self.chain} network when in [{self.chain}] section.')

        main_conf_file_path = self.nodes[0].datadir_path / "bitcoin_main.conf"
        util.write_config(main_conf_file_path, n=0, chain='', extra_config=f'includeconf={inc_conf_file_path}\n')
        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('acceptnonstdtxn=1\n')
        self.nodes[0].assert_start_raises_init_error(extra_args=[f"-conf={main_conf_file_path}", "-allowignoredconf"], expected_msg='Error: acceptnonstdtxn is not currently supported for main chain')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('nono\n')
        self.nodes[0].assert_start_raises_init_error(expected_msg='Error: Error reading configuration file: parse error on line 1: nono, if you intended to specify a negated option, use nono=1 instead')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('server=1\nrpcuser=someuser\nrpcpassword=some#pass')
        self.nodes[0].assert_start_raises_init_error(expected_msg='Error: Error reading configuration file: parse error on line 3, using # in rpcpassword can be ambiguous and should be avoided')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('server=1\nrpcuser=someuser\nmain.rpcpassword=some#pass')
        self.nodes[0].assert_start_raises_init_error(expected_msg='Error: Error reading configuration file: parse error on line 3, using # in rpcpassword can be ambiguous and should be avoided')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('server=1\nrpcuser=someuser\n[main]\nrpcpassword=some#pass')
        self.nodes[0].assert_start_raises_init_error(expected_msg='Error: Error reading configuration file: parse error on line 4, using # in rpcpassword can be ambiguous and should be avoided')

        inc_conf_file2_path = self.nodes[0].datadir_path / 'include2.conf'
        with open(self.nodes[0].datadir_path / 'bitcoin.conf', 'a', encoding='utf-8') as conf:
            conf.write(f'includeconf={inc_conf_file2_path}\n')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('testnot.datadir=1\n')
        with open(inc_conf_file2_path, 'w', encoding='utf-8') as conf:
            conf.write('[testnet]\n')
        self.restart_node(0)
        self.nodes[0].stop_node(expected_stderr=f'Warning: {inc_conf_file_path}:1 Section [testnot] is not recognized.{os.linesep}{inc_conf_file2_path}:1 Section [testnet] is not recognized.')

        with open(inc_conf_file_path, 'w', encoding='utf-8') as conf:
            conf.write('')  # clear
        with open(inc_conf_file2_path, 'w', encoding='utf-8') as conf:
            conf.write('')  # clear

    def test_config_file_log(self):
        # Disable this test for windows currently because trying to override
        # the default datadir through the environment does not seem to work.
        if platform.system() == "Windows":
            return

        self.log.info('Test that correct configuration path is changed when configuration file changes the datadir')

        # Create a temporary directory that will be treated as the default data
        # directory by bitcoind.
        env, default_datadir = util.get_temp_default_datadir(Path(self.options.tmpdir, "test_config_file_log"))
        default_datadir.mkdir(parents=True)

        # Write a bitcoin.conf file in the default data directory containing a
        # datadir= line pointing at the node datadir.
        node = self.nodes[0]
        conf_text = node.bitcoinconf.read_text()
        conf_path = default_datadir / "bitcoin.conf"
        conf_path.write_text(f"datadir={node.datadir_path}\n{conf_text}")

        # Drop the node -datadir= argument during this test, because if it is
        # specified it would take precedence over the datadir setting in the
        # config file.
        node_args = node.args
        node.args = [arg for arg in node.args if not arg.startswith("-datadir=")]

        # Check that correct configuration file path is actually logged
        # (conf_path, not node.bitcoinconf)
        with self.nodes[0].assert_debug_log(expected_msgs=[f"Config file: {conf_path}"]):
            self.start_node(0, ["-allowignoredconf"], env=env)
            self.stop_node(0)

        # Restore node arguments after the test
        node.args = node_args

    def test_invalid_command_line_options(self):
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Error parsing command line arguments: Can not set -proxy with no value. Please specify value with -proxy=value.',
            extra_args=['-proxy'],
        )
        # Provide a value different from 1 to the -wallet negated option
        if self.is_wallet_compiled():
            for value in [0, 'not_a_boolean']:
                self.nodes[0].assert_start_raises_init_error(
                    expected_msg="Error: Invalid value detected for '-wallet' or '-nowallet'. '-wallet' requires a string value, while '-nowallet' accepts only '1' to disable all wallets",
                    extra_args=[f'-nowallet={value}'],
                )

    def test_log_buffer(self):
        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['Warning: parsed potentially confusing double-negative -connect=0\n']):
            self.start_node(0, extra_args=['-noconnect=0'])

    def test_args_log(self):
        self.stop_node(0)
        self.log.info('Test config args logging')
        with self.nodes[0].assert_debug_log(
                expected_msgs=[
                    'Command-line arg: addnode="some.node"',
                    'Command-line arg: rpcauth=****',
                    'Command-line arg: rpcpassword=****',
                    'Command-line arg: rpcuser=****',
                    'Command-line arg: torpassword=****',
                    f'Config file arg: {self.chain}="1"',
                    f'Config file arg: [{self.chain}] server="1"',
                ],
                unexpected_msgs=[
                    'alice:f7efda5c189b999524f151318c0c86$d5b51b3beffbc0',
                    'secret-rpcuser',
                    'secret-torpassword',
                    'Command-line arg: rpcbind=****',
                    'Command-line arg: rpcallowip=****',
                ]):
            self.start_node(0, extra_args=[
                '-addnode=some.node',
                '-rpcauth=alice:f7efda5c189b999524f151318c0c86$d5b51b3beffbc0',
                '-rpcbind=127.1.1.1',
                '-rpcbind=127.0.0.1',
                "-rpcallowip=127.0.0.1",
                '-rpcpassword=',
                '-rpcuser=secret-rpcuser',
                '-torpassword=secret-torpassword',
            ])

    def test_networkactive(self):
        self.log.info('Test -networkactive option')
        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: true\n']):
            self.start_node(0)

        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: true\n']):
            self.start_node(0, extra_args=['-networkactive'])

        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: true\n']):
            self.start_node(0, extra_args=['-networkactive=1'])

        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: false\n']):
            self.start_node(0, extra_args=['-networkactive=0'])

        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: false\n']):
            self.start_node(0, extra_args=['-nonetworkactive'])

        self.stop_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: false\n']):
            self.start_node(0, extra_args=['-nonetworkactive=1'])

    def test_seed_peers(self):
        self.log.info('Test seed peers')
        default_data_dir = self.nodes[0].datadir_path
        peer_dat = default_data_dir / 'peers.dat'
        # Only regtest has no fixed seeds. To avoid connections to random
        # nodes, regtest is the only network where it is safe to enable
        # -fixedseeds in tests
        util.assert_equal(self.nodes[0].getblockchaininfo()['chain'],'regtest')
        self.stop_node(0)

        # No peers.dat exists and -dnsseed=1
        # We expect the node will use DNS Seeds, but Regtest mode does not have
        # any valid DNS seeds. So after 60 seconds, the node should fallback to
        # fixed seeds
        assert not peer_dat.exists()
        start = int(time.time())
        with self.nodes[0].assert_debug_log(
                expected_msgs=[
                    "Loaded 0 addresses from peers.dat",
                    "0 addresses found from DNS seeds",
                    "opencon thread start",  # Ensure ThreadOpenConnections::start time is properly set
                ],
                timeout=10,
        ):
            self.start_node(0, extra_args=['-dnsseed=1', '-fixedseeds=1', f'-mocktime={start}'])
        with self.nodes[0].assert_debug_log(expected_msgs=[
                "Adding fixed seeds as 60 seconds have passed and addrman is empty",
        ]):
            self.nodes[0].setmocktime(start + 65)
        self.stop_node(0)

        # No peers.dat exists and -dnsseed=0
        # We expect the node will fallback immediately to fixed seeds
        assert not peer_dat.exists()
        with self.nodes[0].assert_debug_log(expected_msgs=[
                "Loaded 0 addresses from peers.dat",
                "DNS seeding disabled",
                "Adding fixed seeds as -dnsseed=0 (or IPv4/IPv6 connections are disabled via -onlynet) and neither -addnode nor -seednode are provided\n",
        ]):
            self.start_node(0, extra_args=['-dnsseed=0', '-fixedseeds=1'])
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(['-dnsseed=1', '-onlynet=i2p', '-i2psam=127.0.0.1:7656'], "Error: Incompatible options: -dnsseed=1 was explicitly specified, but -onlynet forbids connections to IPv4/IPv6")

        # No peers.dat exists and dns seeds are disabled.
        # We expect the node will not add fixed seeds when explicitly disabled.
        assert not peer_dat.exists()
        with self.nodes[0].assert_debug_log(expected_msgs=[
                "Loaded 0 addresses from peers.dat",
                "DNS seeding disabled",
                "Fixed seeds are disabled",
        ]):
            self.start_node(0, extra_args=['-dnsseed=0', '-fixedseeds=0'])
        self.stop_node(0)

        # No peers.dat exists and -dnsseed=0, but a -addnode is provided
        # We expect the node will allow 60 seconds prior to using fixed seeds
        assert not peer_dat.exists()
        start = int(time.time())
        with self.nodes[0].assert_debug_log(
                expected_msgs=[
                    "Loaded 0 addresses from peers.dat",
                    "DNS seeding disabled",
                    "opencon thread start",  # Ensure ThreadOpenConnections::start time is properly set
                ],
                timeout=10,
        ):
            self.start_node(0, extra_args=['-dnsseed=0', '-fixedseeds=1', '-addnode=fakenodeaddr', f'-mocktime={start}'])
        with self.nodes[0].assert_debug_log(expected_msgs=[
                "Adding fixed seeds as 60 seconds have passed and addrman is empty",
        ]):
            self.nodes[0].setmocktime(start + 65)

    def test_connect_with_seednode(self):
        self.log.info('Test -connect with -seednode')
        seednode_ignored = ['-seednode is ignored when -connect is used\n']
        dnsseed_ignored = ['-dnsseed is ignored when -connect is used and -proxy is specified\n']
        addcon_thread_started = ['addcon thread start\n']
        self.stop_node(0)

        # When -connect is supplied, expanding addrman via getaddr calls to ADDR_FETCH(-seednode)
        # nodes is irrelevant and -seednode is ignored.
        with self.nodes[0].assert_debug_log(expected_msgs=seednode_ignored):
            self.start_node(0, extra_args=['-connect=fakeaddress1', '-seednode=fakeaddress2'])

        # With -proxy, an ADDR_FETCH connection is made to a peer that the dns seed resolves to.
        # ADDR_FETCH connections are not used when -connect is used.
        with self.nodes[0].assert_debug_log(expected_msgs=dnsseed_ignored):
            self.restart_node(0, extra_args=['-connect=fakeaddress1', '-dnsseed=1', '-proxy=1.2.3.4'])

        # If the user did not disable -dnsseed, but it was soft-disabled because they provided -connect,
        # they shouldn't see a warning about -dnsseed being ignored.
        with self.nodes[0].assert_debug_log(expected_msgs=addcon_thread_started,
                unexpected_msgs=dnsseed_ignored):
            self.restart_node(0, extra_args=['-connect=fakeaddress1', '-proxy=1.2.3.4'])

        # We have to supply expected_msgs as it's a required argument
        # The expected_msg must be something we are confident will be logged after the unexpected_msg
        # These cases test for -connect being supplied but only to disable it
        for connect_arg in ['-connect=0', '-noconnect']:
            with self.nodes[0].assert_debug_log(expected_msgs=addcon_thread_started,
                    unexpected_msgs=seednode_ignored):
                self.restart_node(0, extra_args=[connect_arg, '-seednode=fakeaddress2'])

    def test_ignored_conf(self):
        self.log.info('Test error is triggered when the datadir in use contains a bitcoin.conf file that would be ignored '
                      'because a conflicting -conf file argument is passed.')
        node = self.nodes[0]
        with tempfile.NamedTemporaryFile(dir=self.options.tmpdir, mode="wt", delete=False) as temp_conf:
            temp_conf.write(f"datadir={node.datadir_path}\n")
        node.assert_start_raises_init_error([f"-conf={temp_conf.name}"], re.escape(
            f'Error: Data directory "{node.datadir_path}" contains a "bitcoin.conf" file which is ignored, because a '
            f'different configuration file "{temp_conf.name}" from command line argument "-conf={temp_conf.name}" '
            f'is being used instead.') + r"[\s\S]*", match=ErrorMatch.FULL_REGEX)

        # Test that passing a redundant -conf command line argument pointing to
        # the same bitcoin.conf that would be loaded anyway does not trigger an
        # error.
        self.start_node(0, [f'-conf={node.datadir_path}/bitcoin.conf'])
        self.stop_node(0)

    def test_ignored_default_conf(self):
        # Disable this test for windows currently because trying to override
        # the default datadir through the environment does not seem to work.
        if platform.system() == "Windows":
            return

        self.log.info('Test error is triggered when bitcoin.conf in the default data directory sets another datadir '
                      'and it contains a different bitcoin.conf file that would be ignored')

        # Create a temporary directory that will be treated as the default data
        # directory by bitcoind.
        env, default_datadir = util.get_temp_default_datadir(Path(self.options.tmpdir, "home"))
        default_datadir.mkdir(parents=True)

        # Write a bitcoin.conf file in the default data directory containing a
        # datadir= line pointing at the node datadir. This will trigger a
        # startup error because the node datadir contains a different
        # bitcoin.conf that would be ignored.
        node = self.nodes[0]
        (default_datadir / "bitcoin.conf").write_text(f"datadir={node.datadir_path}\n")

        # Drop the node -datadir= argument during this test, because if it is
        # specified it would take precedence over the datadir setting in the
        # config file.
        node_args = node.args
        node.args = [arg for arg in node.args if not arg.startswith("-datadir=")]
        node.assert_start_raises_init_error([], re.escape(
            f'Error: Data directory "{node.datadir_path}" contains a "bitcoin.conf" file which is ignored, because a '
            f'different configuration file "{default_datadir}/bitcoin.conf" from data directory "{default_datadir}" '
            f'is being used instead.') + r"[\s\S]*", env=env, match=ErrorMatch.FULL_REGEX)
        node.args = node_args

    def test_acceptstalefeeestimates_arg_support(self):
        self.log.info("Test -acceptstalefeeestimates option support")
        conf_file = self.nodes[0].datadir_path / "bitcoin.conf"
        for chain, chain_name in {("main", ""), ("test", "testnet3"), ("signet", "signet"), ("testnet4", "testnet4")}:
            util.write_config(conf_file, n=0, chain=chain_name, extra_config='acceptstalefeeestimates=1\n')
            self.nodes[0].assert_start_raises_init_error(expected_msg=f'Error: acceptstalefeeestimates is not supported on {chain} chain.')
        util.write_config(conf_file, n=0, chain="regtest")  # Reset to regtest

    def test_testnet3_deprecation_msg(self):
        self.log.info("Test testnet3 deprecation warning")
        t3_warning_log = "Warning: Support for testnet3 is deprecated and will be removed in an upcoming release. Consider switching to testnet4."

        def warning_msg(node, approx_size):
            return f'Warning: Disk space for "{node.datadir_path / node.chain / "blocks" }" may not accommodate the block files. Approximately {approx_size} GB of data will be stored in this directory.'

        # Testnet3 node will log the warning
        self.nodes[0].chain = 'testnet3'
        self.nodes[0].replace_in_config([('regtest=', 'testnet='), ('[regtest]', '[test]')])
        with self.nodes[0].assert_debug_log([t3_warning_log]):
            self.start_node(0)
        # Some CI environments will have limited space and some others won't
        # so we need to handle both cases as a valid result.
        self.nodes[0].stderr.seek(0)
        err = self.nodes[0].stdout.read()
        self.nodes[0].stderr.seek(0)
        self.nodes[0].stderr.truncate()
        if err != b'' and err != warning_msg(self.nodes[0], 42):
            raise AssertionError("Unexpected stderr after shutdown of Testnet3 node")
        self.stop_node(0)

        # Testnet4 node will not log the warning
        self.nodes[0].chain = 'testnet4'
        self.nodes[0].replace_in_config([('testnet=', 'testnet4='), ('[test]', '[testnet4]')])
        with self.nodes[0].assert_debug_log([], unexpected_msgs=[t3_warning_log]):
            self.start_node(0)
        self.stop_node(0)

        # Reset to regtest
        self.nodes[0].chain = 'regtest'
        self.nodes[0].replace_in_config([('testnet4=', 'regtest='), ('[testnet4]', '[regtest]')])

    def run_test(self):
        self.test_log_buffer()
        self.test_args_log()
        self.test_seed_peers()
        self.test_networkactive()
        self.test_connect_with_seednode()

        self.test_config_file_parser()
        self.test_config_file_log()
        self.test_invalid_command_line_options()
        self.test_ignored_conf()
        self.test_ignored_default_conf()
        self.test_acceptstalefeeestimates_arg_support()
        self.test_testnet3_deprecation_msg()

        # Remove the -datadir argument so it doesn't override the config file
        self.nodes[0].args = [arg for arg in self.nodes[0].args if not arg.startswith("-datadir")]

        default_data_dir = self.nodes[0].datadir_path
        new_data_dir = default_data_dir / 'newdatadir'
        new_data_dir_2 = default_data_dir / 'newdatadir2'

        # Check that using -datadir argument on non-existent directory fails
        self.nodes[0].datadir_path = new_data_dir
        self.nodes[0].assert_start_raises_init_error([f'-datadir={new_data_dir}'], f'Error: Specified data directory "{new_data_dir}" does not exist.')

        # Check that using non-existent datadir in conf file fails
        conf_file = default_data_dir / "bitcoin.conf"

        # datadir needs to be set before [chain] section
        with open(conf_file, encoding='utf8') as f:
            conf_file_contents = f.read()
        with open(conf_file, 'w', encoding='utf8') as f:
            f.write(f"datadir={new_data_dir}\n")
            f.write(conf_file_contents)

        self.nodes[0].assert_start_raises_init_error([f'-conf={conf_file}'], f'Error: Error reading configuration file: specified data directory "{new_data_dir}" does not exist.')

        # Check that an explicitly specified config file that cannot be opened fails
        none_existent_conf_file = default_data_dir / "none_existent_bitcoin.conf"
        self.nodes[0].assert_start_raises_init_error(['-conf=' + f'{none_existent_conf_file}'], 'Error: Error reading configuration file: specified config file "' + f'{none_existent_conf_file}' + '" could not be opened.')

        # Create the directory and ensure the config file now works
        new_data_dir.mkdir()
        self.start_node(0, [f'-conf={conf_file}'])
        self.stop_node(0)
        assert (new_data_dir / self.chain / 'blocks').exists()

        # Ensure command line argument overrides datadir in conf
        new_data_dir_2.mkdir()
        self.nodes[0].datadir_path = new_data_dir_2
        self.start_node(0, [f'-datadir={new_data_dir_2}', f'-conf={conf_file}'])
        assert (new_data_dir_2 / self.chain / 'blocks').exists()


if __name__ == '__main__':
    ConfArgsTest(__file__).main()
