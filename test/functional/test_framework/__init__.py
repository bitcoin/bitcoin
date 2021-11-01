import sys

from .bitcoincore import (
    address,
    authproxy,
    bdb,
    blocktools,
    coverage,
    descriptors,
    key,
    messages,
    muhash,
    netutil,
    p2p,
    script,
    script_util,
    segwit_addr,
    siphash,
    socks5,
    test_node,
    util,
    wallet,
    wallet_util,
)

# XXX this is some ugliness to ensure bitcoincore modules are importable as
# "test_framework.[module]", since otherwise we have to change hundreds of
# imports or create a dummy module file for each.
for i in (
    'address',
    'authproxy',
    'bdb',
    'blocktools',
    'coverage',
    'descriptors',
    'key',
    'messages',
    'muhash',
    'netutil',
    'p2p',
    'script',
    'script_util',
    'segwit_addr',
    'siphash',
    'socks5',
    'test_node',
    'util',
    'wallet',
    'wallet_util',
):
    sys.modules[f'test_framework.{i}'] = sys.modules[f'test_framework.bitcoincore.{i}']
