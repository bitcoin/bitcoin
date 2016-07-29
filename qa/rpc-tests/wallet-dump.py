#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import os
import shutil


class WalletDumpTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = False
        self.num_nodes = 1

    def setup_network(self, split=False):
        extra_args = [["-keypool=100"]]
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, extra_args)

    def run_test (self):
        tmpdir = self.options.tmpdir

        #generate 20 addresses to compare against the dump
        test_addr_count = 20
        addrs = []
        for i in range(0,test_addr_count):
            addr = self.nodes[0].getnewaddress()
            vaddr= self.nodes[0].validateaddress(addr) #required to get hd keypath
            addrs.append(vaddr)

        # dump unencrypted wallet
        self.nodes[0].dumpwallet(tmpdir + "/node0/wallet.unencrypted.dump")

        #open file
        inputfile = open(tmpdir + "/node0/wallet.unencrypted.dump")
        found_addr = 0
        found_addr_chg = 0
        found_addr_rsv = 0
        hdmasteraddr = ""
        for line in inputfile:
            #only read non comment lines
            if line[0] != "#" and len(line) > 10:
                #split out some data
                keyLabel, comment = line.split("#")
                key = keyLabel.split(" ")[0]
                keytype = keyLabel.split(" ")[2]
                if len(comment) > 1:
                    addrKeypath = comment.split(" addr=")[1]
                    addr = addrKeypath.split(" ")[0]
                    keypath = ""
                    if keytype != "hdmaster=1":
                        keypath = addrKeypath.rstrip().split("hdkeypath=")[1]
                    else:
                        #keep hd master for later comp.
                        hdmasteraddr = addr

                    #count key types
                    for addrObj in addrs:
                        if (addrObj['address'] == addr and addrObj['hdkeypath'] == keypath and keytype == "label="):
                            found_addr+=1
                            break
                        elif (keytype == "change=1"):
                            found_addr_chg+=1
                            break
                        elif (keytype == "reserve=1"):
                            found_addr_rsv+=1
                            break
        assert(found_addr == test_addr_count) #all keys must be in the dump
        assert(found_addr_chg == 50) #50 blocks where mined
        assert(found_addr_rsv == 100) #100 reserve keys (keypool)

        #encrypt wallet, restart, unlock and dump
        self.nodes[0].encryptwallet('test')
        bitcoind_processes[0].wait()
        self.nodes[0] = start_node(0, self.options.tmpdir)
        self.nodes[0].walletpassphrase('test', 10)
        self.nodes[0].dumpwallet(tmpdir + "/node0/wallet.encrypted.dump")

        #open dump done with an encrypted wallet
        inputfile = open(tmpdir + "/node0/wallet.encrypted.dump")
        found_addr = 0
        found_addr_chg = 0
        found_addr_rsv = 0
        for line in inputfile:
            if line[0] != "#" and len(line) > 10:
                keyLabel, comment = line.split("#")
                key = keyLabel.split(" ")[0]
                keytype = keyLabel.split(" ")[2]
                if len(comment) > 1:
                    addrKeypath = comment.split(" addr=")[1]
                    addr = addrKeypath.split(" ")[0]
                    keypath = ""
                    if keytype != "hdmaster=1":
                        keypath = addrKeypath.rstrip().split("hdkeypath=")[1]
                    else:
                        #ensure we have generated a new hd master key
                        assert(hdmasteraddr != addr)
                    if keytype == "inactivehdmaster=1":
                        #ensure the old master is still available
                        assert(hdmasteraddr == addr)
                    for addrObj in addrs:
                        if (addrObj['address'] == addr and addrObj['hdkeypath'] == keypath and keytype == "label="):
                            found_addr+=1
                            break
                        elif (keytype == "change=1"):
                            found_addr_chg+=1
                            break
                        elif (keytype == "reserve=1"):
                            found_addr_rsv+=1
                            break

        assert(found_addr == test_addr_count)
        assert(found_addr_chg == 150) #old reserve keys are marked as change now
        assert(found_addr_rsv == 100) #keypool size

if __name__ == '__main__':
    WalletDumpTest().main ()
