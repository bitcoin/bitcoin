#!/usr/bin/env python3
# Copyright (c) 2021-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import random
from test_framework.test_framework import DashTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

'''
rpc_verifychainlock.py

Test the following RPC:
 - gettxchainlocks
 - verifychainlock

'''


class RPCVerifyChainLockTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        
    def set_test_params(self):
        # -whitelist is needed to avoid the trickling logic on node0
        self.set_dash_test_params(6, 5, [["-whitelist=noban@127.0.0.1"]] * 6, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def cl_helper(self, height, chainlock, mempool):
        return {'height': height, 'chainlock': chainlock, 'mempool': mempool}

    def mutate_signature(self, signature):
        # Convert the hex string to a list of bytes
        sig_bytes = bytearray.fromhex(signature)
        
        # Randomly select a byte to mutate
        index_to_mutate = random.randint(0, len(sig_bytes) - 1)
        
        # Mutate the selected byte by flipping one of its bits
        sig_bytes[index_to_mutate] ^= 1 << random.randint(0, 7)
        
        # Convert the mutated bytes back to a hex string
        mutated_signature = sig_bytes.hex()
        
        return mutated_signature

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()
        
        for i in range(4):
            self.mine_quorum()
       
        cl0 = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl0)
        

        chainlock = node0.getbestchainlock()
        block_hash = chainlock["blockhash"]
        chainlock_signature = chainlock["signature"]
        chainlock_signers = chainlock["signers"]
        block_hash_prev = chainlock["blockhash"]
        chainlock_signature_prev = chainlock["signature"]
        chainlock_signers_prev = chainlock["signers"]
        assert node0.verifychainlock(block_hash, chainlock_signature, chainlock_signers)
        # Invalid "blockHash"
        assert not node0.verifychainlock(node0.getblockhash(0), chainlock_signature, chainlock_signers)
        # Invalid "signers"
        assert not node0.verifychainlock(block_hash, chainlock_signature, "")
        # Invalid "sig"
        assert_raises_rpc_error(-8, "invalid signature format", node0.verifychainlock, block_hash, self.mutate_signature(chainlock_signature), chainlock_signers)
        # Isolate node1, mine a block on node0 and wait for its ChainLock
        node1.setnetworkactive(False)
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        self.sync_blocks(nodes=(self.nodes[0], self.nodes[2], self.nodes[3], self.nodes[4], self.nodes[5]))
        cl0 = self.nodes[0].getbestblockhash()
        
        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        self.wait_for_chainlocked_block(node0, cl0)
        
        chainlock = node0.getbestchainlock()
        assert chainlock != node1.getbestchainlock()
        block_hash = chainlock["blockhash"]
        chainlock_signature = chainlock["signature"]
        chainlock_signers = chainlock["signers"]
        height0 = chainlock["height"]
        # Now it should verify on node0 but fail on node1
        assert node0.verifychainlock(block_hash, chainlock_signature, chainlock_signers)
        assert_raises_rpc_error(-32603, "blockHash not found", node1.verifychainlock, block_hash, chainlock_signature, chainlock_signers)

        # node1 can verify the previous one still
        assert node1.verifychainlock(block_hash_prev, chainlock_signature_prev, chainlock_signers_prev)
        self.generate(self.nodes[1], 5, sync_fun=self.no_op)
        cl1 = self.nodes[1].getbestblockhash()
        height1 = self.nodes[1].getblockcount()
        self.generate(self.nodes[1], 5, sync_fun=self.no_op)
        tx0 = node0.getblock(cl0)['tx'][0]
        tx1 = node1.getblock(cl1)['tx'][0]
        tx2 = node0.sendtoaddress(node0.getnewaddress(), 1)
        locks0 = node0.gettxchainlocks([tx0, tx1, tx2])
        locks1 = node1.gettxchainlocks([tx0, tx1, tx2])
        unknown_cl_helper = self.cl_helper(0, False, False)
        assert_equal(locks0, [self.cl_helper(height0, True, False), unknown_cl_helper, self.cl_helper(0, False, True)])
        assert_equal(locks1, [unknown_cl_helper, self.cl_helper(height1, False, False), unknown_cl_helper])

if __name__ == '__main__':
    RPCVerifyChainLockTest().main()
