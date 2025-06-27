#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC help output."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

from collections import defaultdict
import os
import re


def parse_string(s):
    assert s[0] == '"'
    assert s[-1] == '"'
    return s[1:-1]

def process_mapping(fname):
    """Find and parse conversion table in implementation file `fname`."""
    cmds = []
    string_params = []
    in_rpcs = False
    with open(fname, "r", encoding="utf8") as f:
        for line in f:
            line = line.rstrip()
            if not in_rpcs:
                if re.match(r'static const CRPCConvertParam vRPCConvertParams\[] =', line):
                    in_rpcs = True
            else:
                if line.startswith('};'):
                    in_rpcs = False
                elif '{' in line and '"' in line:
                    # Match lines with ParamFormat::STRING
                    m_string = re.search(r'{ *("[^"]*") *, *([0-9]+) *, *("[^"]*") *, *ParamFormat::STRING *},?', line)
                    if m_string:
                        name = parse_string(m_string.group(1))
                        idx = int(m_string.group(2))
                        argname = parse_string(m_string.group(3))
                        string_params.append((name, idx, argname))
                        continue

                    # Match lines with ParamFormat::JSON and ParamFormat::JSON_OR_STRING
                    m_json = re.search(r'{ *("[^"]*") *, *([0-9]+) *, *("[^"]*") *(?:, *ParamFormat::(JSON_OR_STRING|JSON))? *},?', line)
                    if m_json:
                        name = parse_string(m_json.group(1))
                        idx = int(m_json.group(2))
                        argname = parse_string(m_json.group(3))
                        cmds.append((name, idx, argname))

    assert not in_rpcs
    return cmds, string_params

class HelpRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None

    def run_test(self):
        self.test_client_conversion_table()
        self.test_client_string_conversion_table()
        self.test_categories()
        self.dump_help()
        if self.is_wallet_compiled():
            self.wallet_help()

    def test_client_conversion_table(self):
        file_conversion_table = os.path.join(self.config["environment"]["SRCDIR"], 'src', 'rpc', 'client.cpp')
        mapping_client, _ = process_mapping(file_conversion_table)
        # Ignore echojson in client table
        mapping_client = [m for m in mapping_client if m[0] != 'echojson']

        mapping_server = self.nodes[0].help("dump_all_command_conversions")
        # Filter all RPCs whether they need conversion
        mapping_server_conversion = [tuple(m[:3]) for m in mapping_server if not m[3]]

        # Only check if all RPC methods have been compiled (i.e. wallet is enabled)
        if self.is_wallet_compiled() and sorted(mapping_client) != sorted(mapping_server_conversion):
            raise AssertionError("RPC client conversion table ({}) and RPC server named arguments mismatch!\n{}".format(
                file_conversion_table,
                set(mapping_client).symmetric_difference(mapping_server_conversion),
            ))

        # Check for conversion difference by argument name.
        # It is preferable for API consistency that arguments with the same name
        # have the same conversion, so bin by argument name.
        all_methods_by_argname = defaultdict(list)
        converts_by_argname = defaultdict(list)
        for m in mapping_server:
            all_methods_by_argname[m[2]].append(m[0])
            converts_by_argname[m[2]].append(m[3])

        for argname, convert in converts_by_argname.items():
            if all(convert) != any(convert):
                # Only allow dummy and psbt to fail consistency check
                assert argname in ['dummy', "psbt"], ('WARNING: conversion mismatch for argument named %s (%s)' % (argname, list(zip(all_methods_by_argname[argname], converts_by_argname[argname]))))

    def test_client_string_conversion_table(self):
        file_conversion_table = os.path.join(self.config["environment"]["SRCDIR"], 'src', 'rpc', 'client.cpp')
        _, string_params_client = process_mapping(file_conversion_table)
        mapping_server = self.nodes[0].help("dump_all_command_conversions")
        server_tuples = {tuple(m[:3]) for m in mapping_server}

        # Filter string parameters based on wallet compilation status
        if self.is_wallet_compiled():
            # Check that every entry in string parameters exists on the server
            stale_entries = [entry for entry in string_params_client if entry not in server_tuples]
            if stale_entries:
                raise AssertionError(f"String parameters contains entries not present on the server: {stale_entries}")
            filtered_string_params = string_params_client
        else:
            available_string_params = [entry for entry in string_params_client if entry in server_tuples]
            filtered_string_params = available_string_params

        # Validate that all entries are legitimate server parameters
        server_method_param_tuples = {(m[0], m[1], m[2]) for m in mapping_server}
        invalid_entries = [entry for entry in filtered_string_params if entry not in server_method_param_tuples]
        if invalid_entries:
            raise AssertionError(f"String parameters contains invalid entries: {invalid_entries}")

    def test_categories(self):
        node = self.nodes[0]

        # wrong argument count
        assert_raises_rpc_error(-1, 'help', node.help, 'foo', 'bar')

        # invalid argument
        if not self.options.usecli:
            assert_raises_rpc_error(-3, "JSON value of type number is not of expected type string", node.help, 0)

        # help of unknown command
        assert_equal(node.help('foo'), 'help: unknown command: foo')

        # command titles
        titles = [line[3:-3] for line in node.help().splitlines() if line.startswith('==')]

        components = ['Blockchain', 'Control', 'Mining', 'Network', 'Rawtransactions', 'Util']

        if self.is_wallet_compiled():
            components.append('Wallet')

        if self.is_external_signer_compiled():
            components.append('Signer')

        if self.is_zmq_compiled():
            components.append('Zmq')

        assert_equal(titles, sorted(components))

    def dump_help(self):
        dump_dir = os.path.join(self.options.tmpdir, 'rpc_help_dump')
        os.mkdir(dump_dir)
        calls = [line.split(' ', 1)[0] for line in self.nodes[0].help().splitlines() if line and not line.startswith('==')]
        for call in calls:
            with open(os.path.join(dump_dir, call), 'w', encoding='utf-8') as f:
                # Make sure the node can generate the help at runtime without crashing
                f.write(self.nodes[0].help(call))

    def wallet_help(self):
        assert 'getnewaddress ( "label" "address_type" )' in self.nodes[0].help('getnewaddress')
        self.restart_node(0, extra_args=['-nowallet=1'])
        assert 'getnewaddress ( "label" "address_type" )' in self.nodes[0].help('getnewaddress')

if __name__ == '__main__':
    HelpRpcTest(__file__).main()
