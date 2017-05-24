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
This test exercises BIP135 fork activation thresholds.
It uses a single node with custom forks.csv file.

It is originally derived from bip9-softforks.
'''


class BIP135ForksTest(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.defined_forks = [ "bip135test%d" % i for i in range(0,8) ]

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

            ########## THRESHOLD TESTING BITS (1-6) ############

            # bit 0: test 'csv' fork renaming/reparameterization
            "regtest,0,bip135test0,%d,999999999999,144,108,0,0,true\n" % (self.fork_starttime) +

            # bit 1: test 'segwit' fork renaming/reparameterization
            "regtest,1,bip135test1,%d,999999999999,144,108,0,0,true\n" % (self.fork_starttime) +

            # bit 2: test minimum threshold
            "regtest,2,bip135test2,%d,999999999999,100,1,0,0,true\n" % (self.fork_starttime) +

            # bit 3: small threshold
            "regtest,3,bip135test3,%d,999999999999,100,10,0,0,true\n" % (self.fork_starttime) +

            # bit 4: supermajority threshold
            "regtest,4,bip135test4,%d,999999999999,100,75,0,0,true\n" % (self.fork_starttime) +

            # bit 5: high threshold
            "regtest,5,bip135test5,%d,999999999999,100,95,0,0,true\n" % (self.fork_starttime) +

            # bit 6: max-but-one threshold
            "regtest,6,bip135test6,%d,999999999999,100,99,0,0,true\n" % (self.fork_starttime) +

            # bit 7: max threshold
            "regtest,7,bip135test7,%d,999999999999,100,100,0,0,true\n" % (self.fork_starttime)

            )

        self.nodes = start_nodes(1, self.options.tmpdir,
                                 extra_args=[['-debug=all', '-whitelist=127.0.0.1',
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
            old_tip = self.tip
            block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
            block.nVersion = version
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            self.height += 1
            #print ("generate_blocks: created block %x on tip %x, height %d, time %d" % (self.tip, old_tip, self.height-1, self.last_block_time))
        return test_blocks

    def get_bip135_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        return info['bip135_forks'][key]

    def print_rpc_status(self):
        for f in self.defined_forks:
            info = self.nodes[0].getblockchaininfo()
            print(info['bip135_forks'][f])

    def test_BIP135Thresholds(self):

        print("test_BIP135Thresholds: begin")
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

        # Test 2
        # check initial DEFINED state
        # check initial forks status and getblocktemplate
        print("begin test 2")
        tmpl = self.nodes[0].getblocktemplate({})
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'], 0x20000000)
        print("initial getblocktemplate:\n%s" % tmpl)

        test_blocks = []
        bcinfo = self.nodes[0].getblockchaininfo()
        tip_mediantime = int(bcinfo['mediantime'])
        while tip_mediantime < self.fork_starttime or self.height < 100:
            for f in self.defined_forks:
                assert_equal(self.get_bip135_status(f)['bit'], int(f[10:]))
                assert_equal(self.get_bip135_status(f)['status'], 'defined')
                assert(f not in tmpl['rules'])
                assert(f not in tmpl['vbavailable'])
            test_blocks = self.generate_blocks(1, 0x20000001)
            yield TestInstance(test_blocks, sync_every_block=False)
            bcinfo = self.nodes[0].getblockchaininfo()
            tip_mediantime = int(bcinfo['mediantime'])

        # Test 3
        # Advance from DEFINED to STARTED
        print("begin test 3")
        test_blocks = self.generate_blocks(1, 0x20000001)
        for f in self.defined_forks:
            if int(f[10:]) > 1:
                assert_equal(self.get_bip135_status(f)['status'], 'started')
                assert(f not in tmpl['rules'])
                assert(f not in tmpl['vbavailable'])
            elif int(f[10:]) == 0: # bit 0 only becomes started at height 144
                assert_equal(self.get_bip135_status(f)['status'], 'defined')

        yield TestInstance(test_blocks, sync_every_block=False)

        # Test 4
        # Advance from DEFINED to STARTED
        print("begin test 4")
        test_blocks = self.generate_blocks(1, 0x20000001)
        yield TestInstance(test_blocks, sync_every_block=False)

        for f in self.defined_forks:
            info = node.getblockchaininfo()
            print(info['bip135_forks'][f])

        # Test 5..?
        # Advance from DEFINED to STARTED
        print("begin test 5 .. x - move to height 144 for bit 0 start")
        # we are not yet at height 144, so bit 0 is still defined
        assert_equal(self.get_bip135_status(self.defined_forks[0])['status'], 'defined')
        # move up until it starts
        while self.height < 144:
            #print("last block time has not reached fork_starttime, difference: %d" % (self.fork_starttime - self.last_block_time))
            test_blocks = self.generate_blocks(1, 0x20000001)
            yield TestInstance(test_blocks, sync_every_block=False)
            if int(f[10:]) > 0:
                assert_equal(self.get_bip135_status(f)['status'], 'started')
                assert(f not in tmpl['rules'])
                assert(f not in tmpl['vbavailable'])
            else: # bit 0 only becomes started at height 144
                assert_equal(self.get_bip135_status(f)['status'], 'defined')

        # generate block 144
        test_blocks = []
        # now it should be started

        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.get_bip135_status(self.defined_forks[0])['status'], 'started')

        tmpl = self.nodes[0].getblocktemplate({})
        assert(self.defined_forks[0] not in tmpl['rules'])
        assert_equal(tmpl['vbavailable'][self.defined_forks[0]], 0)
        assert_equal(tmpl['vbrequired'], 0)
        assert(tmpl['version'] & 0x20000000 + 2**0)

        # move to start of new 100 block window
        # start signaling for bit 7 so we can get a full 100 block window for it
        # over the next period
        test_blocks = self.generate_blocks(100 - (self.height % 100), 0x20000080)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert(self.height % 100 == 0)

        # generate enough of bits 2-7 in next 100 blocks to lock in fork bits 2-7
        # bit 0 will only be locked in at next multiple of 144
        test_blocks = []
        # 1 block total for bit 2
        test_blocks = self.generate_blocks(1, 0x200000FD)
        yield TestInstance(test_blocks, sync_every_block=False)
        # check still STARTED until we get to multiple of window size
        assert_equal(self.get_bip135_status(self.defined_forks[1])['status'], 'started')

        # 10 blocks total for bit 3
        test_blocks = self.generate_blocks(9, 0x200000F9)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.get_bip135_status(self.defined_forks[2])['status'], 'started')

        # 75 blocks total for bit 4
        test_blocks = self.generate_blocks(65, 0x200000F1)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.get_bip135_status(self.defined_forks[3])['status'], 'started')

        # 95 blocks total for bit 5
        test_blocks = self.generate_blocks(20, 0x200000E1)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.get_bip135_status(self.defined_forks[4])['status'], 'started')

        # 99 blocks total for bit 6
        test_blocks = self.generate_blocks(4, 0x200000C1)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert_equal(self.get_bip135_status(self.defined_forks[5])['status'], 'started')

        # 100 blocks total for bit 7
        test_blocks = self.generate_blocks(1, 0x20000081)
        yield TestInstance(test_blocks, sync_every_block=False)
        assert(self.height % 100 == 0)

        # debug trace
        for f in self.defined_forks[2:]:
            info = node.getblockchaininfo()
            print(info['bip135_forks'][f])
            assert_equal(self.get_bip135_status(f)['status'], 'locked_in')

        assert_equal(self.get_bip135_status(self.defined_forks[0])['status'], 'started')
        # move until bit 0 locks in
        print("moving until bit 0 locks in")
        one_hundreds_active = False  # to count the 100-block bits 1-6 going active after 100 more
        while self.height % 144 != 0:
            test_blocks = self.generate_blocks(1, 0x20000001)
            yield TestInstance(test_blocks, sync_every_block=False)
            continue
            bcinfo = self.nodes[0].getblockchaininfo()
            # check bit 0 - it is locked in after this loop exits
            if self.height % 144:
                assert_equal(bcinfo['bip135_forks'][self.defined_forks[0]]['status'], 'started')
            else:
                assert_equal(bcinfo['bip135_forks'][self.defined_forks[0]]['status'], 'locked_in')
            # bits 1-6 should remain LOCKED_IN
            for f in self.defined_forks[2:]:
                if self.height % 100:
                    if not one_hundreds_active:
                        assert_equal(bcinfo['bip135_forks'][f]['status'], 'locked_in')
                    else:
                        assert_equal(bcinfo['bip135_forks'][f]['status'], 'active')
                else:
                    # mark them expected active henceforth
                    one_hundreds_active = True
                    assert_equal(bcinfo['bip135_forks'][f]['status'], 'active')

        assert_equal(self.get_bip135_status(self.defined_forks[0])['status'], 'locked_in')

    def get_tests(self):
        '''
        run various tests
        '''
        # CSV (bit 0) for backward compatibility with BIP9
        for test in itertools.chain(
                self.test_BIP135Thresholds(),  # test thresholds on other bits
        ):
            yield test



if __name__ == '__main__':
    BIP135ForksTest().main()
