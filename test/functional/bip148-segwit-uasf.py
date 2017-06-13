#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the Mandatory Activation of Segregated Witness (BIP148) soft-fork logic."""

from test_framework.mininode import wait_until
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

import re

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
        for ea in self.extra_args:
            ea.append('-whitelist=::/0')

        self.day = -30 # 30 days before BIP148 activation
        self.blockrate = [0,1,3,4,3,0,0]
        self.ancestor_cache = {}

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

    def is_block_ancestor(self, rpc, tip, ancestor, ancestor_height):
        cache_key = tip + ancestor
        if cache_key in self.ancestor_cache:
            return self.ancestor_cache[cache_key]
        blockinfo = rpc.getblock(tip)
        while blockinfo['height'] > ancestor_height:
            blockinfo = rpc.getblock(blockinfo['previousblockhash'])
            parent_cache_key = blockinfo['hash'] + ancestor
            if parent_cache_key in self.ancestor_cache:
                rv = self.ancestor_cache[parent_cache_key]
                self.ancestor_cache[cache_key] = rv
                return rv
        rv = (blockinfo['hash'] == ancestor)
        self.ancestor_cache[cache_key] = rv
        return rv

    def sync_chaintips(self, nodes=None, timeout=60, extra_check=None):
        if nodes is None:
            nodes = list(range(len(self.nodes)))
        statuses_where_node_has_the_block = ('valid-fork', 'active')
        def chaintip_check():
            if extra_check is not None:
                if not extra_check():
                    return False
            all_known_tips = {}
            chaintips_replies = [(r, self.nodes[r].getchaintips()) for r in nodes]
            for (r, tips) in chaintips_replies:
                for tip in tips:
                    if tip['hash'] in all_known_tips and tip['status'] not in statuses_where_node_has_the_block:
                        continue
                    all_known_tips[tip['hash']] = (r, tip)

            # Make sure we know a node we can fetch the block from
            for tip in all_known_tips.keys():
                if all_known_tips[tip][1]['status'] not in statuses_where_node_has_the_block:
                    for r in nodes:
                        try:
                            all_known_tips[tip] = (r, self.nodes[r].getblock(tip))
                            break
                        except:
                            pass

            self.log.debug('There are %d tips: %s' % (len(all_known_tips), tuple(sorted(all_known_tips.keys()))))
            for (r, tips) in chaintips_replies:
                invalid_blocks = []
                my_known_tips = set()
                active = None
                # Ideally, best should use chainwork, but that's not in getchaintips...
                best = {'height':0}
                for tip in tips:
                    my_known_tips.add(tip['hash'])
                    if tip['status'] == 'invalid':
                        invalid_blocks.append(tip)
                    else:
                        if tip['height'] > best['height']:
                            best = tip
                    if tip['status'] == 'active':
                        active = tip
                        if tip['height'] == best['height']:
                            best = tip
                if best != active:
                    self.log.debug("Best potentially-valid block is not active on node %s" % (r,))
                    return False
                missing_tips = all_known_tips.keys() - my_known_tips
                for tip in set(missing_tips):
                    for inv_tip in invalid_blocks:
                        if self.is_block_ancestor(self.nodes[all_known_tips[tip][0]], tip, inv_tip['hash'], inv_tip['height']):
                            # One of our invalid tips is a parent of the missing tip
                            missing_tips.remove(tip)
                            break
                    for known_tip in my_known_tips:
                        # NOTE: Can't assume this node has the block, in case it's invalid
                        if self.is_block_ancestor(self.nodes[all_known_tips[known_tip][0]], known_tip, tip, all_known_tips[tip][1]['height']):
                            # We have a valid tip that descends from the missing tip
                            missing_tips.remove(tip)
                            break
                    if tip in missing_tips:
                        self.log.debug('Node %s missing tip %s' % (r, tip))
                        return False
            self.log.debug('All nodes have all syncable tips')
            return True
        assert wait_until(chaintip_check, timeout=timeout)

    def get_peerset(self, rpc):
        def extract_nodenum(subver):
            m = re.search(r'\((.*?)[;)]', subver)
            return m.group(1)
        return {pi['id']: extract_nodenum(pi['subver']) for pi in rpc.getpeerinfo()}

    def mining(self):
        time = self.AUG_1 + (self.day*24*60*60)
        to_mine = self.blockrate[:]

        while sum(to_mine) > 0:
            for peer,blks in enumerate(to_mine):
                if blks == 0: continue
                set_node_times(self.nodes, time)

                peerset_before = tuple(self.get_peerset(r) for r in self.nodes)
                def check_peerset():
                    peerset_after = tuple(self.get_peerset(r) for r in self.nodes)
                    if peerset_before != peerset_after:
                        raise AssertionError("Network break!\nBefore=%s\nAfter =%s" % (peerset_before, peerset_after))
                    return True

                to_mine[peer] -= 1
                self.nodes[peer].generate(1)

                self.sync_chaintips(extra_check=check_peerset)
                time += 10*60

        self.day += 1

    def bip148_restart(self, peer):
        self.log.info("Restarting node %d with -bip148" % (peer,))
        self.stop_node(peer)
        time = self.AUG_1 + (self.day*24*60*60)
        self.nodes[peer] = self.start_node(peer, self.options.tmpdir, self.extra_args[4] + ["-bip148", "-mocktime=%d" % (time)])

        n = self.nodes[peer]
        blk = n.getblock(n.getbestblockhash())
        while blk["mediantime"] >= self.AUG_1:
            if blk["version"] | 0x20000002 != blk["version"]:
                raise AssertionError("Invalid BIP148 block in node running with -bip148")
            blk = n.getblock(blk["previousblockhash"])

        self.log.info("Reconnecting nodes")
        self.connect_all()

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
            swstatus = get_bip9_status(self.nodes[0], 'segwit')["status"]
            if self.day == 0:
                if swstatus != "started":
                    raise AssertionError("segwit soft-fork in state %s rather than started at day 0" % (swstatus))

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

            self.log.info("Day %d:%s: status: %s:" % (self.day, swstatus, out))

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
