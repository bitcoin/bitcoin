#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the dumpwallet RPC."""

import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def read_dump(file_name, addrs, hd_master_addr_old):
    """
    Read the given dump, count the addrs that match, count change and reserve.
    Also check that the old hd_master is inactive
    """
    with open(file_name, encoding='utf8') as inputfile:
        found_addr = 0
        found_addr_chg = 0
        found_addr_rsv = 0
        hd_master_addr_ret = None
        for line in inputfile:
            # only read non comment lines
            if line[0] != "#" and len(line) > 10:
                # split out some data
                key_label, comment = line.split("#")
                # key = key_label.split(" ")[0]
                keytype = key_label.split(" ")[2]
                if len(comment) > 1:
                    addr_keypath = comment.split(" addr=")[1]
                    addr = addr_keypath.split(" ")[0]
                    keypath = None
                    if keytype == "inactivehdmaster=1":
                        # ensure the old master is still available
                        assert(hd_master_addr_old == addr)
                    elif keytype == "hdmaster=1":
                        # ensure we have generated a new hd master key
                        assert(hd_master_addr_old != addr)
                        hd_master_addr_ret = addr
                    else:
                        keypath = addr_keypath.rstrip().split("hdkeypath=")[1]

                    # count key types
                    for addrObj in addrs:
                        if addrObj['address'] == addr and addrObj['hdkeypath'] == keypath and keytype == "label=":
                            found_addr += 1
                            break
                        elif keytype == "change=1":
                            found_addr_chg += 1
                            break
                        elif keytype == "reserve=1":
                            found_addr_rsv += 1
                            break
        return found_addr, found_addr_chg, found_addr_rsv, hd_master_addr_ret


class WalletDumpTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-keypool=90"]]

    def setup_network(self, split=False):
        # Use 1 minute timeout because the initial getnewaddress RPC can take
        # longer than the default 30 seconds due to an expensive
        # CWallet::TopUpKeyPool call, and the encryptwallet RPC made later in
        # the test often takes even longer.
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args, timewait=60)

    def run_test (self):
        tmpdir = self.options.tmpdir

        # generate 20 addresses to compare against the dump
        test_addr_count = 20
        addrs = []
        for i in range(0,test_addr_count):
            addr = self.nodes[0].getnewaddress()
            vaddr= self.nodes[0].validateaddress(addr) #required to get hd keypath
            addrs.append(vaddr)
        # Should be a no-op:
        self.nodes[0].keypoolrefill()

        # dump unencrypted wallet
        result = self.nodes[0].dumpwallet(tmpdir + "/node0/wallet.unencrypted.dump")
        assert_equal(result['filename'], os.path.abspath(tmpdir + "/node0/wallet.unencrypted.dump"))

        found_addr, found_addr_chg, found_addr_rsv, hd_master_addr_unenc = \
            read_dump(tmpdir + "/node0/wallet.unencrypted.dump", addrs, None)
        assert_equal(found_addr, test_addr_count)  # all keys must be in the dump
        assert_equal(found_addr_chg, 50)  # 50 blocks where mined
        assert_equal(found_addr_rsv, 90*2) # 90 keys plus 100% internal keys

        #encrypt wallet, restart, unlock and dump
        self.nodes[0].encryptwallet('test')
        self.bitcoind_processes[0].wait()
        self.nodes[0] = self.start_node(0, self.options.tmpdir, self.extra_args[0])
        self.nodes[0].walletpassphrase('test', 10)
        # Should be a no-op:
        self.nodes[0].keypoolrefill()
        self.nodes[0].dumpwallet(tmpdir + "/node0/wallet.encrypted.dump")

        found_addr, found_addr_chg, found_addr_rsv, hd_master_addr_enc = \
            read_dump(tmpdir + "/node0/wallet.encrypted.dump", addrs, hd_master_addr_unenc)
        assert_equal(found_addr, test_addr_count)
        assert_equal(found_addr_chg, 90*2 + 50)  # old reserve keys are marked as change now
        assert_equal(found_addr_rsv, 90*2) 

if __name__ == '__main__':
    WalletDumpTest().main ()
