#!/usr/bin/env python3
# Copyright (c) 2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Copyright (c) 2017 The Bitcoin developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
from test_framework.util import sync_blocks
from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread
from test_framework.blocktools import create_coinbase, create_block
from test_framework.comptool import TestInstance, TestManager
from test_framework.script import CScript, OP_1NEGATE, OP_CHECKSEQUENCEVERIFY, OP_DROP
from io import BytesIO
import time
import itertools
import tempfile

'''
This test exercises BIP135 fork grace periods.
It uses a single node with custom forks.csv file.

It is originally derived from bip9-softforks.
'''


class BIP135ForksTest(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.defined_forks = [ "bip135test%d" % i for i in range(7,22) ]

    def setup_network(self):
        '''
        sets up with a custom forks.csv to test threshold / grace conditions
        also recomputes deployment start times since node is shut down and
        restarted between main test sections.
        '''
        # write a custom forks.csv file containing test deployments.
        # blocks are 1 second apart by default in this regtest
        fork_csv_filename = os.path.join(self.options.tmpdir, "forks.csv")
        # forks.csv fields:
        # network,bit,name,starttime,timeout,windowsize,threshold,minlockedblocks,minlockedtime,gbtforce
        with open(fork_csv_filename, 'wt') as fh:
            # use current time to compute offset starttimes
            self.init_time = int(time.time())
            # starttimes are offset by 30 seconds from init_time
            self.fork_starttime = self.init_time + 30
            fh.write(
            "# deployment info for network 'regtest':\n" +

            ########## GRACE PERIOD TESTING BITS (7-21) ############

            # bit 7: one minlockedblock
            "regtest,7,bip135test7,%d,999999999999,10,9,1,0,true\n" % (self.fork_starttime) +

            # bit 8: half a window of minlockedblocks
            "regtest,8,bip135test8,%d,999999999999,10,9,5,0,true\n" % (self.fork_starttime) +

            # bit 9: full window of minlockedblocks
            "regtest,9,bip135test9,%d,999999999999,10,9,10,0,true\n" % (self.fork_starttime) +

            # bit 10: just over full window of minlockedblocks
            "regtest,10,bip135test10,%d,999999999999,10,9,11,0,true\n" % (self.fork_starttime) +

            # bit 11: one second minlockedtime
            "regtest,11,bip135test11,%d,999999999999,10,9,0,1,true\n" % (self.fork_starttime) +

            # bit 12: half window of minlockedtime
            "regtest,12,bip135test12,%d,999999999999,10,9,0,%d,true\n" % (self.fork_starttime, 5) +

            # bit 13: just under one full window of minlockedtime
            "regtest,13,bip135test13,%d,999999999999,10,9,0,%d,true\n" % (self.fork_starttime, 9) +

            # bit 14: exactly one window of minlockedtime
            "regtest,14,bip135test14,%d,999999999999,10,9,0,%d,true\n" % (self.fork_starttime, 10) +

            # bit 15: just over one window of minlockedtime
            "regtest,15,bip135test15,%d,999999999999,10,9,0,%d,true\n" % (self.fork_starttime, 11) +

            # bit 16: one and a half window of minlockedtime
            "regtest,16,bip135test16,%d,999999999999,10,9,0,%d,true\n" % (self.fork_starttime, 15) +

            # bit 17: one window of minblockedblocks plus one window of minlockedtime
            "regtest,17,bip135test17,%d,999999999999,10,9,10,%d,true\n" % (self.fork_starttime, 10) +

            # bit 18: one window of minblockedblocks plus just under two windows of minlockedtime
            "regtest,18,bip135test18,%d,999999999999,10,9,10,%d,true\n" % (self.fork_starttime, 19) +

            # bit 19: one window of minblockedblocks plus two windows of minlockedtime
            "regtest,19,bip135test19,%d,999999999999,10,9,10,%d,true\n" % (self.fork_starttime, 20) +

            # bit 20: two windows of minblockedblocks plus two windows of minlockedtime
            "regtest,20,bip135test20,%d,999999999999,10,9,20,%d,true\n" % (self.fork_starttime, 21) +

            # bit 21: just over two windows of minblockedblocks plus two windows of minlockedtime
            "regtest,21,bip135test21,%d,999999999999,10,9,21,%d,true\n" % (self.fork_starttime, 20) +

            ########## NOT USED SO FAR ############
            "regtest,22,bip135test22,%d,999999999999,100,9,0,0,true\n" % (self.fork_starttime)
            )

        self.nodes = start_nodes(1, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1',
                                              "-forks=%s" % fork_csv_filename]],
                                 binary=[self.options.testbinary])

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.test.run()

    def create_transaction(self, node, coinbase, to_address, amount):
        from_txid = node.getblock(coinbase)['tx'][0]
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(rawtx))
        tx.deserialize(f)
        tx.nVersion = 2
        return tx

    def sign_transaction(self, node, tx):
        signresult = node.signrawtransaction(bytes_to_hex_str(tx.serialize()))
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(signresult['hex']))
        tx.deserialize(f)
        return tx

    def generate_blocks(self, number, version, test_blocks = []):
        for i in range(number):
            #print ("generate_blocks: creating block on tip %x, height %d, time %d" % (self.tip, self.height, self.last_block_time + 1))
            block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
            block.nVersion = version
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            self.height += 1
        return test_blocks

    def get_bip135_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        return info['bip135_forks'][key]

    def print_rpc_status(self):
        for f in self.defined_forks:
            info = self.nodes[0].getblockchaininfo()
            print(info['bip135_forks'][f])

    def test_BIP135GraceConditions(self):
        print("begin test_BIP135GraceConditions test")
        node = self.nodes[0]
        self.tip = int("0x" + node.getbestblockhash(), 0)
        header = node.getblockheader("0x%x" % self.tip)
        assert_equal(header['height'], 0)

        # Test 1
        # generate a block, seems necessary to get RPC working
        self.coinbase_blocks = self.nodes[0].generate(1)
        self.height = 2
        self.last_block_time = int(time.time())
        self.tip = int("0x" + node.getbestblockhash(), 0)
        test_blocks = self.generate_blocks(1, 0x20000000)  # do not set bit 0 yet
        yield TestInstance(test_blocks, sync_every_block=False)

        bcinfo = self.nodes[0].getblockchaininfo()
        # check bits 7-15 , they should be in DEFINED
        for f in self.defined_forks:
            assert_equal(bcinfo['bip135_forks'][f]['bit'], int(f[10:]))
            assert_equal(bcinfo['bip135_forks'][f]['status'], 'defined')

        # move to starttime
        moved_to_started = False
        bcinfo = self.nodes[0].getblockchaininfo()
        tip_mediantime = int(bcinfo['mediantime'])
        while tip_mediantime < self.fork_starttime or self.height % 10:
            test_blocks = self.generate_blocks(1, 0x20000000)
            yield TestInstance(test_blocks, sync_every_block=False)
            bcinfo = self.nodes[0].getblockchaininfo()
            tip_mediantime = int(bcinfo['mediantime'])
            for f in self.defined_forks:
                # transition to STARTED must occur if this is true
                if tip_mediantime >= self.fork_starttime and self.height % 10 == 0:
                    moved_to_started = True

                if moved_to_started:
                    assert_equal(bcinfo['bip135_forks'][f]['status'], 'started')
                else:
                    assert_equal(bcinfo['bip135_forks'][f]['status'], 'defined')

        # lock all of them them in by producing 9 signaling blocks out of 10
        test_blocks = self.generate_blocks(9, 0x203fff80)
        # tenth block doesn't need to signal
        test_blocks = self.generate_blocks(1, 0x20000000, test_blocks)
        yield TestInstance(test_blocks, sync_every_block=False)
        # check bits 7-15 , they should all be in LOCKED_IN
        bcinfo = self.nodes[0].getblockchaininfo()
        print("checking all grace period forks are locked in")
        for f in self.defined_forks:
            assert_equal(bcinfo['bip135_forks'][f]['status'], 'locked_in')

        # now we just check that they turn ACTIVE only when their configured
        # conditions are all met. Reminder: window size is 10 blocks, inter-
        # block time is 1 sec for the synthesized chain.
        #
        # Grace conditions for the bits 7-21:
        # -----------------------------------
        # bit 7:  minlockedblocks= 1, minlockedtime= 0  -> activate next sync
        # bit 8:  minlockedblocks= 5, minlockedtime= 0  -> activate next sync
        # bit 9:  minlockedblocks=10, minlockedtime= 0  -> activate next sync
        # bit 10: minlockedblocks=11, minlockedtime= 0  -> activate next plus one sync
        # bit 11: minlockedblocks= 0, minlockedtime= 1  -> activate next sync
        # bit 12: minlockedblocks= 0, minlockedtime= 5  -> activate next sync
        # bit 13: minlockedblocks= 0, minlockedtime= 9  -> activate next sync
        # bit 14: minlockedblocks= 0, minlockedtime=10  -> activate next sync
        # bit 15: minlockedblocks= 0, minlockedtime=11  -> activate next plus one sync
        # bit 16: minlockedblocks= 0, minlockedtime=15  -> activate next plus one sync
        # bit 17: minlockedblocks=10, minlockedtime=10  -> activate next sync
        # bit 18: minlockedblocks=10, minlockedtime=19  -> activate next plus one sync
        # bit 19: minlockedblocks=10, minlockedtime=20  -> activate next plus one sync
        # bit 20: minlockedblocks=20, minlockedtime=21  -> activate next plus two sync
        # bit 21: minlockedblocks=21, minlockedtime=20  -> activate next plus two sync

        # check the forks supposed to activate just one period after lock-in ("at next sync")

        test_blocks = self.generate_blocks(10, 0x20000000)
        yield TestInstance(test_blocks, sync_every_block=False)
        bcinfo = self.nodes[0].getblockchaininfo()
        activation_states = [ bcinfo['bip135_forks'][f]['status'] for f in self.defined_forks ]
        assert_equal(activation_states, ['active',
                                         'active',
                                         'active',
                                         'locked_in',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'locked_in',
                                         'locked_in',
                                         'active',
                                         'locked_in',
                                         'locked_in',
                                         'locked_in',
                                         'locked_in'
                                         ])

        # check the ones supposed to activate at next+1 sync
        test_blocks = self.generate_blocks(10, 0x20000000)
        yield TestInstance(test_blocks, sync_every_block=False)
        bcinfo = self.nodes[0].getblockchaininfo()
        activation_states = [ bcinfo['bip135_forks'][f]['status'] for f in self.defined_forks ]
        assert_equal(activation_states, ['active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'locked_in',
                                         'locked_in'
                                         ])

        # check the ones supposed to activate at next+2 period
        test_blocks = self.generate_blocks(10, 0x20000000)
        yield TestInstance(test_blocks, sync_every_block=False)
        bcinfo = self.nodes[0].getblockchaininfo()
        activation_states = [ bcinfo['bip135_forks'][f]['status'] for f in self.defined_forks ]
        assert_equal(activation_states, ['active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active'  # locked_in
                                         ])

        # check the ones supposed to activate at next+2 period
        test_blocks = self.generate_blocks(10, 0x20000000)
        yield TestInstance(test_blocks, sync_every_block=False)
        bcinfo = self.nodes[0].getblockchaininfo()
        activation_states = [ bcinfo['bip135_forks'][f]['status'] for f in self.defined_forks ]
        assert_equal(activation_states, ['active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active',
                                         'active'
                                         ])


    def get_tests(self):
        '''
        run various tests
        '''
        # CSV (bit 0) for backward compatibility with BIP9
        for test in itertools.chain(
                self.test_BIP135GraceConditions(), # test grace periods
        ):
            yield test



if __name__ == '__main__':
    BIP135ForksTest().main()
