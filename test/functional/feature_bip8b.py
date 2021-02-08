#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bip8 activation
"""

import random
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal
from test_framework.blocktools import create_block, create_coinbase
from test_framework.p2p import P2PDataStore, p2p_lock
from test_framework.messages import CBlockHeader, msg_headers

SEED = random.randrange(2**128)

VB_SIGNAL = 0x20000000 | (0x01 << 28)
VB_NOSIGNAL = 0x20000000

MAX_HEADERS = 2000

BASE_TIME = int(time.time()) - 12*60*60

class P2PBlockCheck(P2PDataStore):
    def __init__(self, node, blocks):
        super().__init__()
        self.node = node
        with p2p_lock:
            for block in blocks:
                self.block_store[block.sha256] = block
                self.last_block_hash = block.sha256

    def refresh_headers(self):
        headers = []
        with p2p_lock:
            for _, block in self.block_store.items():
                headers.append(CBlockHeader(block))
        while headers:
            self.send_message(msg_headers(headers[:MAX_HEADERS]))
            headers = headers[MAX_HEADERS:]

    def send_blocks(self, blocks, *, reject_reason=None, timeout=20):
        tiphash = blocks[-1].hash

        with p2p_lock:
            for block in blocks:
                self.block_store[block.sha256] = block
                self.last_block_hash = block.sha256

        reject_reason = [reject_reason] if reject_reason else []
        success = reject_reason == []

        with self.node.assert_debug_log(expected_msgs=reject_reason, timeout=timeout):
            self.send_message(msg_headers([CBlockHeader(block) for block in blocks]))
            self.sync_with_ping(timeout)
            if success:
                self.wait_until(lambda: self.node.getbestblockhash() == tiphash, timeout=timeout)

        if not success:
            assert self.node.getbestblockhash() != tiphash

class BIP8Test(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        # don't want to connect the bitcoinds to each other
        self.setup_nodes()

    def generate_blocks(self, versioniter, prevblock, tipheight):
        test_blocks = []

        for version in versioniter:
            blocktime = BASE_TIME + (tipheight * 6)  # 6 seconds between blocks
            block = create_block(prevblock, create_coinbase(tipheight), blocktime)
            block.nVersion = version
            block.rehash()
            block.solve()
            test_blocks.append(block)
            prevblock = block.sha256
            tipheight = tipheight + 1

        return test_blocks

    def apply_blocks_and_check(self, versions, base_height, state, height=None):
        start = base_height + 288
        stop = base_height + 720

        if height is None:
            bci = self.nodes[0].getblockchaininfo()
            prevblock = int("0x" + bci["bestblockhash"], 0)
            tipheight = bci["blocks"] + 1
        else:
            prevblock = int("0x" + self.nodes[0].getblockhash(height), 0)
            tipheight = height + 1

        blocks = self.generate_blocks(versions, prevblock, tipheight)

        self.helper[0].send_blocks(blocks)
        self.helper[1].send_blocks(blocks)
        self.helper[2].send_blocks(blocks)
        if state[0] == "stopped":
            ok_cnt = state[1]-tipheight
            if ok_cnt > 0:
                self.helper[3].send_blocks(blocks[:ok_cnt])
            if ok_cnt >= 0:
                self.helper[3].send_blocks(blocks[ok_cnt:], reject_reason="bad-vbit-unset-testdummy")
        else:
            self.helper[3].send_blocks(blocks)

        heights, status = self.get_softfork_status()

        # compare results
        assert_equal(heights[0], heights[1], heights[2])
        assert_greater_than_or_equal(heights[0], heights[3])
        if state[0] != "stopped":
            assert_equal(heights[0], heights[3])

        if state[0] != "stopped":
            assert_equal(status[2]["bip8"].get("statistics", None), status[3]["bip8"].get("statistics", None))
            assert_equal(status[2]["active"], status[3]["active"])

        # never active
        assert_equal(status[0], None)

        # always active
        assert_equal(status[1], {'type': 'bip8', 'bip8': {'status': 'active', 'startheight': -1, 'timeoutheight': 0, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': 0}, 'height': 0, 'active': True})

        # lockinontimeout=false
        if "statistics" in status[2]["bip8"]:
            status[2]["bip8"]["statistics"] = None

        if state[0] == "defined":
            assert_equal(status[2], {'type': 'bip8', 'bip8': {'status': 'defined', 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': False, 'since': state[1]}, 'active': False})
        elif state[0] == "started" or state[0] == "must_signal":
            since = state[1] if state[0] == "started" else (state[1] - 288)
            assert_equal(status[2], {'type': 'bip8', 'bip8': {'status': 'started', 'bit': 28, 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': False, 'since': since, 'statistics': None}, 'active': False})
        elif state[0] == "locked_in":
            assert_equal(status[2], {'type': 'bip8', 'bip8': {'status': 'locked_in', 'bit': 28, 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': False, 'since': state[1], 'statistics': None}, 'active': False})
        elif state[0] == "active":
            assert_equal(status[2], {'type': 'bip8', 'bip8': {'status': 'active', 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': False, 'since': state[1]}, 'height': state[1], 'active': True})
        elif state[0] == "stopped":
            failat = state[1] - (state[1] % 144) + 144
            assert_equal(status[2], {'type': 'bip8', 'bip8': {'status': 'failed', 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': False, 'since': failat}, 'active': False})
        else:
            assert False, ("bad state %r" % (state))

        # lockinontimeout=true
        if "statistics" in status[3]["bip8"]:
            assert_equal(status[3]["bip8"]["statistics"]["possible"], True)
            status[3]["bip8"]["statistics"] = None

        if state[0] == "defined":
            assert_equal(status[3], {'type': 'bip8', 'bip8': {'status': 'defined', 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': state[1]}, 'active': False})
        elif state[0] == "started":
            assert_equal(status[3], {'type': 'bip8', 'bip8': {'status': 'started', 'bit': 28, 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': state[1], 'statistics': None}, 'active': False})
        elif state[0] == "must_signal" or state[0] == "stopped":
            since = state[1] - state[1] % 144
            assert_equal(status[3], {'type': 'bip8', 'bip8': {'status': 'must_signal', 'bit': 28, 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': since, 'statistics': None}, 'active': False})
        elif state[0] == "locked_in":
            assert_equal(status[3], {'type': 'bip8', 'bip8': {'status': 'locked_in', 'bit': 28, 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': state[1], 'statistics': None}, 'active': False})
        elif state[0] == "active":
            assert_equal(status[3], {'type': 'bip8', 'bip8': {'status': 'active', 'startheight': start, 'timeoutheight': stop, 'minimum_activation_height': 0, 'lockinontimeout': True, 'since': state[1]}, 'height': state[1], 'active': True})
        else:
            assert False, ("bad state %r" % (state))

        return blocks


    def get_softfork_status(self):
        info = [node.getblockchaininfo() for node in self.nodes]
        return zip( *[(i["blocks"], i['softforks'].get('testdummy', None))
                    for i in info] )

    def setup_vbparams(self):
        base_height = self.nodes[0].getblockcount()
        if base_height:
            base_height += 1
        assert base_height % 144 == 0
        start = base_height + 288
        stop = base_height + 720

        for node in self.nodes:
            node.disconnect_p2ps()
        self.helper = []
        self.stop_nodes()

        self.start_nodes(extra_args = [
            # never active
            ["-vbparams=testdummy:@-2:@-2:0"],
            # always active
            ["-vbparams=testdummy:@-1:@0:1"],
            # abandon softfork if not locked in by timeout
            ["-vbparams=testdummy:@%s:@%s:0" % (start, stop)],
            # reject blocks if not locked in by timeout
            ["-vbparams=testdummy:@%s:@%s:1" % (start, stop)],
        ])
        for n in range(1, 4):
            self.connect_nodes(0, n)
        self.sync_all()
        for n in range(1, 4):
            self.disconnect_nodes(0, n)
        self.helper = [n.add_p2p_connection(P2PBlockCheck(n, self.all_blocks)) for n in self.nodes]
        return base_height

    def do_test(self, signalling, expstate, expheight):
        base_height = self.setup_vbparams()

        # track the expected state/height for rejecting-node
        state = "defined", 0

        period = 0
        for cnt in signalling:
            nblocks = 144
            if base_height == 0 and period == 0:
                nblocks -= 1
            cnt = max(0, min(cnt, nblocks))
            bits = [VB_SIGNAL]*cnt + [VB_NOSIGNAL]*(nblocks - cnt)

            random.shuffle(bits)

            # what will the state be after these blocks are mined for a lockinontimeout=true node?
            period += 1
            if state[0] == "defined" and period == 2:
                state = "started", base_height + (period*144)
            elif state[0] == "started":
                if cnt >= 108:
                    state = "locked_in", base_height + (period*144)
                elif period == 4:
                    state = "must_signal", base_height + (period*144)
            elif state[0] == "must_signal":
                if cnt >= 108:
                    state = "locked_in", base_height + (period*144)
                else:
                    oknosig = 144-108
                    howmany = None
                    for i, x in enumerate(bits):
                        if x != VB_SIGNAL:
                            if oknosig == 0:
                                howmany = i
                                break
                            oknosig -= 1
                    state = "stopped", base_height + (period*144) - 144 + howmany
            elif state[0] == "locked_in":
                state = "active", base_height + (period*144)

            self.all_blocks.extend(self.apply_blocks_and_check(bits, base_height, state))

        assert_equal(state[0], expstate)
        assert_greater_than_or_equal(state[1] - base_height, expheight)
        if expstate != "stopped":
            assert_equal(state[1] - base_height, expheight)
        else:
            last_possible = expheight - expheight%144 + 143
            assert_greater_than_or_equal(last_possible, state[1] - base_height)
        self.log.info("Completed test %s %s" % (base_height, state))

    def run_test(self):
        self.all_blocks = []

        random.seed(SEED)
        N = random.randrange(0,108)
        Y = random.randrange(108,145)
        self.log.info("seed %s, signal vals %s/%s" % (SEED, Y, N))

        self.do_test([N,N,N,N,107,144], "stopped", 612)
        self.do_test([144,144,Y,0,0,0], "active", 576)
        self.do_test([Y,Y,0,0,0,Y], "stopped", 612)
        self.do_test([N,N,N,N,N,144], "stopped", 612)
        self.do_test([0,0,0,Y,0,0], "active", 720)
        self.do_test([0,144,N,N,Y,0], "active", 864)

if __name__ == '__main__':
    BIP8Test().main()
