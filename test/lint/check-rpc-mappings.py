#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Check RPC argument consistency."""

from collections import defaultdict
import os
import re
import sys

# Source files (relative to root) to scan for dispatch tables
SOURCES = [
    "src/rpc/server.cpp",
    "src/rpc/blockchain.cpp",
    "src/rpc/mining.cpp",
    "src/rpc/misc.cpp",
    "src/rpc/net.cpp",
    "src/rpc/rawtransaction.cpp",
    "src/wallet/rpcwallet.cpp",
]
# Source file (relative to root) containing conversion mapping
SOURCE_CLIENT = 'src/rpc/client.cpp'
# Argument names that should be ignored in consistency checks
IGNORE_DUMMY_ARGS = {'dummy', 'arg0', 'arg1', 'arg2', 'arg3', 'arg4', 'arg5', 'arg6', 'arg7', 'arg8', 'arg9'}

class RPCCommand:
    def __init__(self, name, args):
        self.name = name
        self.args = args

class RPCArgument:
    def __init__(self, names, idx):
        self.names = names
        self.idx = idx
        self.convert = False

def parse_string(s):
    assert s[0] == '"'
    assert s[-1] == '"'
    return s[1:-1]

def process_commands(fname):
    """Find and parse dispatch table in implementation file `fname`."""
    cmds = []
    in_rpcs = False
    with open(fname, "r", encoding="utf8") as f:
        for line in f:
            line = line.rstrip()
            if not in_rpcs:
                if re.match(r"static const CRPCCommand .*\[\] =", line):
                    in_rpcs = True
            else:
                if line.startswith('};'):
                    in_rpcs = False
                elif '{' in line and '"' in line:
                    m = re.search(r'{ *("[^"]*"), *("[^"]*"), *&([^,]*), *{([^}]*)} *},', line)
                    assert m, 'No match to table expression: %s' % line
                    name = parse_string(m.group(2))
                    args_str = m.group(4).strip()
                    if args_str:
                        args = [RPCArgument(parse_string(x.strip()).split('|'), idx) for idx, x in enumerate(args_str.split(','))]
                    else:
                        args = []
                    cmds.append(RPCCommand(name, args))
    assert not in_rpcs and cmds, "Something went wrong with parsing the C++ file: update the regexps"
    return cmds

def process_mapping(fname):
    """Find and parse conversion table in implementation file `fname`."""
    cmds = []
    in_rpcs = False
    with open(fname, "r", encoding="utf8") as f:
        for line in f:
            line = line.rstrip()
            if not in_rpcs:
                if line == 'static const CRPCConvertParam vRPCConvertParams[] =':
                    in_rpcs = True
            else:
                if line.startswith('};'):
                    in_rpcs = False
                elif '{' in line and '"' in line:
                    m = re.search(r'{ *("[^"]*"), *([0-9]+) *, *("[^"]*") *},', line)
                    assert m, 'No match to table expression: %s' % line
                    name = parse_string(m.group(1))
                    idx = int(m.group(2))
                    argname = parse_string(m.group(3))
                    cmds.append((name, idx, argname))
    assert not in_rpcs and cmds
    return cmds

def main():
    if len(sys.argv) != 2:
        print('Usage: {} ROOT-DIR'.format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)

    root = sys.argv[1]

    # Get all commands from dispatch tables
    cmds = []
    for fname in SOURCES:
        cmds += process_commands(os.path.join(root, fname))

    cmds_by_name = {}
    for cmd in cmds:
        cmds_by_name[cmd.name] = cmd

    # Get current convert mapping for client
    client = SOURCE_CLIENT
    mapping = set(process_mapping(os.path.join(root, client)))

    print('* Checking consistency between dispatch tables and vRPCConvertParams')

    # Check mapping consistency
    errors = 0
    for (cmdname, argidx, argname) in mapping:
        try:
            rargnames = cmds_by_name[cmdname].args[argidx].names
        except IndexError:
            print('ERROR: %s argument %i (named %s in vRPCConvertParams) is not defined in dispatch table' % (cmdname, argidx, argname))
            errors += 1
            continue
        if argname not in rargnames:
            print('ERROR: %s argument %i is named %s in vRPCConvertParams but %s in dispatch table' % (cmdname, argidx, argname, rargnames), file=sys.stderr)
            errors += 1

    # Check for conflicts in vRPCConvertParams conversion
    # All aliases for an argument must either be present in the
    # conversion table, or not. Anything in between means an oversight
    # and some aliases won't work.
    for cmd in cmds:
        for arg in cmd.args:
            convert = [((cmd.name, arg.idx, argname) in mapping) for argname in arg.names]
            if any(convert) != all(convert):
                print('ERROR: %s argument %s has conflicts in vRPCConvertParams conversion specifier %s' % (cmd.name, arg.names, convert))
                errors += 1
            arg.convert = all(convert)

    # Check for conversion difference by argument name.
    # It is preferable for API consistency that arguments with the same name
    # have the same conversion, so bin by argument name.
    all_methods_by_argname = defaultdict(list)
    converts_by_argname = defaultdict(list)
    for cmd in cmds:
        for arg in cmd.args:
            for argname in arg.names:
                all_methods_by_argname[argname].append(cmd.name)
                converts_by_argname[argname].append(arg.convert)

    for argname, convert in converts_by_argname.items():
        if all(convert) != any(convert):
            if argname in IGNORE_DUMMY_ARGS:
                # these are testing or dummy, don't warn for them
                continue
            print('WARNING: conversion mismatch for argument named %s (%s)' %
                  (argname, list(zip(all_methods_by_argname[argname], converts_by_argname[argname]))))

    sys.exit(errors > 0)


if __name__ == '__main__':
    main()
