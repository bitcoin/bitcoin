#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the Mandatory Activation of Segregated Witness (BIP148) soft-fork logic."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

## Scenario:
#
# Four miners, whose hashrate and strategies are:
#
#     A ( 9%): mine longest chain, signalling segwit
#     B (27%): mine longest chain, not signalling segwit
#     C (37%): bip 148 enforcing, signalling segwit
#     D (27%): switches from (A) to (C) during August
#
# (1, 3, 4 and 3 elevenths of total hashrate respectively; mining
#  11 blocks a day, gives 143 blocks in 13 days, just one block
#  short of a retarget cycle)
#
# B, with 27% of hashrate is sufficient to block activation (which
#    requires 25% on regtest)
# A+B+D has 65% of hashrate prior to D switching
# C+D has 55% of hashrate post D's switch, and since C's hashrate is larger
#   than D's the original BIP-148 chain will eventually have more work
#   than D's chain, forcing a reorg and eventual consensus

def bip9_blockver(*bits):
    return "-blockversion=%d" % (0x20000000 + sum(1<<b for b in bits))

class BIP148Test(BitcoinTestFramework):
    AUG_1 = 1501545600

    def __init__(self):
        super().__init__()
        self.num_nodes = 8
        self.setup_clean_chain = False
        self.extra_args = [
             [bip9_blockver(0)],   # for initial CSV activation only
             [bip9_blockver(1)],
             [bip9_blockver()],
             [bip9_blockver(1), "-bip148"],
             [bip9_blockver(1)],
             [],           # non-mining user, restarts with -bip148 on Aug 5th
             [],           # well connected, non BIP148 peer
             ["-bip148"]]  # well connected, BIP148 peer

        self.day = -30 # 30 days before BIP168 activation
        self.blockrate = [0,1,3,4,3,0,0]
        self.sync_groups = [set((0,1,2,4,5,6)), set((3,7))]

    def connect_all(self):
        connect_nodes(self.nodes[6],7)
        for i in range(6):
            connect_nodes(self.nodes[i], 6)
            connect_nodes(self.nodes[i], 7)
            self.log.info("connecting %d and 6, 7" % (i))

    def setup_network(self):
        self.setup_nodes()
        self.connect_all()
        self.sync_all()

    def mining(self):
        time = self.AUG_1 + (self.day*24*60*60)
        to_mine = self.blockrate[:]
        groups = [ [self.nodes[i] for i in g] for g in self.sync_groups ]

        while sum(to_mine) > 0:
            for peer,blks in enumerate(to_mine):
                if blks == 0: continue
                set_node_times(self.nodes, time)
                to_mine[peer] -= 1
                self.nodes[peer].generate(1)
                for g in groups:
                    if peer in g:
                        self.sync_all( [g] )
                time += 10*60

        self.day += 1

    def bip148_restart(self, peer):
        self.log.info("Restarting node %d with -bip148" % (peer,))
        stop_node(self.nodes[peer], peer)
        time = self.AUG_1 + (self.day*24*60*60)
        self.nodes[peer] = start_node(peer, self.options.tmpdir, self.extra_args[4] + ["-bip148", "-mocktime=%d" % (time)])

        n = self.nodes[peer]
        blk = n.getblock(n.getbestblockhash())
        worstblock = None
        while blk["mediantime"] >= self.AUG_1:
            if blk["version"] | 0x20000002 != blk["version"]:
                worstblock = blk
            blk = n.getblock(blk["previousblockhash"])
        if worstblock is not None:
            self.log.info("Invalidating block %d:%x:%s" % (worstblock["height"], worstblock["version"], worstblock["hash"]))
            n.invalidateblock(worstblock["hash"])

        self.log.info("Reconnecting nodes")
        self.connect_all()
        self.log.info("Removing %d from non-BIP148 sync group" % (peer,))
        self.sync_groups[0].remove(peer)
        self.log.info("Adding %d to BIP148 sync group" % (peer,))
        self.sync_groups[1].add(peer)

    def run_test(self):
        cnt = self.nodes[0].getblockcount()

        # Lock in CSV
        self.nodes[0].generate(500)
        if (self.nodes[0].getblockcount() != cnt + 500):
            raise AssertionError("Failed to mine 500 bip9 bit 0 blocks")
        cnt += 500

        if get_bip9_status(self.nodes[0], 'csv')["status"] != "active":
            raise AssertionError("Failed to activate OP_CSV")

        self.sync_all()

        for d in range(120):
            if self.day == 0:
                swstatus = get_bip9_status(self.nodes[0], 'segwit')["status"]
                if swstatus != "started":
                    raise AssertionError("segwit soft-fork in state %s rather than started" % (swstatus))

            self.mining()
            tips = set(n.getbestblockhash() for n in self.nodes)
            heights = [n.getblockcount() for n in self.nodes]
            segwit = [get_bip9_status(n, 'segwit')["status"] for n in self.nodes]
            connect = [len(n.getpeerinfo()) for n in self.nodes]
            maxh = max(heights)
            out = ["%d:" % (maxh)]
            for c,h,s in zip(connect,heights,segwit):
                a = "S" if s == "active" else "n"
                a = "%d/%s" % (c,a)
                if h < maxh:
                    out.append("%s%d" % (a, h-maxh))
                else:
                    out.append(a)

            self.log.info("Day %d: status: %s" % (self.day, out))

            if self.day == 7:
                self.bip148_restart(5)

            if self.day == 20:
                self.bip148_restart(4)

            for i in range(6):
                if connect[i] == 0:
                    raise AssertionError("Peer %d has no connected peers after %d blocks" % (i,maxh))

            if self.day > 5 and len(tips) == 1:
                synccount += 1
            else:
                synccount = 0
            if synccount > 5:
                self.log.info("In sync for five days, consensus achieved")
                break

        if len(tips) > 1:
            raise AssertionError("Chain split still exists at day %d" % (self.day))

        return

if __name__ == '__main__':
    BIP148Test().main()
