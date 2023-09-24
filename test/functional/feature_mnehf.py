#!/usr/bin/env python3

# Copyright (c) 2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import struct
from io import BytesIO

from test_framework.authproxy import JSONRPCException
from test_framework.key import ECKey
from test_framework.messages import (
    CMnEhf,
    CTransaction,
    hash256,
    ser_string,
)

from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    get_bip9_details,
)

class MnehfTest(DashTestFramework):
    def set_test_params(self):
        extra_args = [["-vbparams=testdummy:0:999999999999:12:12:12:5:0"] for _ in range(4)]
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True, extra_args=extra_args)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def restart_all_nodes(self, params=None):
        for inode in range(self.num_nodes):
            self.log.info(f"Restart node {inode} with {self.extra_args[inode]}")
            if params is not None:
                self.extra_args[inode][0] = f"-vbparams=testdummy:{params[0]}:{params[1]}:12:12:12:5:0"
                self.log.info(f"Actual restart options: {self.extra_args[inode]}")

        self.restart_node(0)
        for mn in self.mninfo:
            index = mn.node.index
            self.stop_node(index)
            self.start_masternode(mn)
        for i in range(1, self.num_nodes):
                self.connect_nodes(i, 0)

    def slowly_generate_batch(self, amount):
        self.log.info(f"Slowly generate {amount} blocks")
        while amount > 0:
            self.log.info(f"Generating batch of blocks {amount} left")
            next = min(10, amount)
            amount -= next
            self.bump_mocktime(next)
            self.nodes[1].generate(next)
            self.sync_all()

    def create_mnehf(self, versionBit, pubkey=None):
        # request ID = sha256("mnhf", versionBit)
        request_id_buf = ser_string(b"mnhf") + struct.pack("<Q", versionBit)
        request_id = hash256(request_id_buf)[::-1].hex()

        quorumHash = self.mninfo[0].node.quorum("selectquorum", 100, request_id)["quorumHash"]
        mnehf_payload = CMnEhf(
            version = 1,
            versionBit = versionBit,
            quorumHash = int(quorumHash, 16),
            quorumSig = b'\00' * 96)

        mnehf_tx = CTransaction()
        mnehf_tx.vin = []
        mnehf_tx.vout = []
        mnehf_tx.nVersion = 3
        mnehf_tx.nType = 7 # mnehf signal
        mnehf_tx.vExtraPayload = mnehf_payload.serialize()

        mnehf_tx.calc_sha256()
        msgHash = format(mnehf_tx.sha256, '064x')

        self.log.info(f"Signing request_id: {request_id} msgHash: {msgHash} quorum: {quorumHash}")
        recsig = self.get_recovered_sig(request_id, msgHash)

        mnehf_payload.quorumSig = bytearray.fromhex(recsig["sig"])
        mnehf_tx.vExtraPayload = mnehf_payload.serialize()
        return mnehf_tx


    def set_sporks(self):
        spork_enabled = 0
        spork_disabled = 4070908800

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", spork_enabled)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", spork_disabled)
        self.nodes[0].sporkupdate("SPORK_3_INSTANTSEND_BLOCK_FILTERING", spork_disabled)
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", spork_disabled)
        self.wait_for_sporks_same()

    def check_fork(self, expected):
        status = get_bip9_details(self.nodes[0], 'testdummy')['status']
        self.log.info(f"height: {self.nodes[0].getblockcount()} status: {status}")
        assert_equal(status, expected)

    def ensure_tx_is_not_mined(self, tx_id):
        try:
            assert_equal(self.nodes[0].getrawtransaction(tx_id, 1)['height'], -1)
            raise AssertionError("Transaction should not be mined")
        except KeyError:
            # KeyError is expected
            pass

    def send_tx(self, tx, expected_error = None, reason = None):
        try:
            self.log.info(f"Send tx with expected_error:'{expected_error}'...")
            tx = self.nodes[0].sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
            if expected_error is None:
                return tx

            # failure didn't happen, but expected:
            message = "Transaction should not be accepted"
            if reason is not None:
                message += ": " + reason

            raise AssertionError(message)
        except JSONRPCException as e:
            self.log.info(f"Send tx triggered an error: {e.error}")
            assert expected_error in e.error['message']


    def run_test(self):
        node = self.nodes[0]

        self.set_sporks()
        self.activate_v19()
        self.log.info(f"After v19 activation should be plenty of blocks: {node.getblockcount()}")
        assert_greater_than(node.getblockcount(), 900)
        assert_equal(get_bip9_details(node, 'testdummy')['status'], 'defined')

        self.log.info("Mine a quorum...")
        self.mine_quorum()
        assert_equal(get_bip9_details(node, 'testdummy')['status'], 'defined')

        key = ECKey()
        key.generate()
        pubkey = key.get_pubkey().get_bytes()
        ehf_tx = self.create_mnehf(28, pubkey)
        ehf_unknown_tx = self.create_mnehf(27, pubkey)
        ehf_invalid_tx = self.create_mnehf(9, pubkey) # deployment that is known as non-EHF

        self.log.info("Checking deserialization of CMnEhf by python's code")
        mnehf_payload = CMnEhf()
        mnehf_payload.deserialize(BytesIO(ehf_tx.vExtraPayload))
        assert_equal(mnehf_payload.version, 1)
        assert_equal(mnehf_payload.versionBit, 28)
        self.log.info("Checking correctness of requestId and quorumHash")
        assert_equal(mnehf_payload.quorumHash, int(self.mninfo[0].node.quorum("selectquorum", 100, 'a0eee872d7d3170dd20d5c5e8380c92b3aa887da5f63d8033289fafa35a90691')["quorumHash"], 16))

        self.send_tx(ehf_tx, expected_error='mnhf-before-v20')

        assert_equal(get_bip9_details(node, 'testdummy')['status'], 'defined')
        self.activate_v20()
        assert_equal(get_bip9_details(node, 'testdummy')['status'], 'defined')

        ehf_tx_sent = self.send_tx(ehf_tx)
        ehf_unknown_tx_sent = self.send_tx(ehf_unknown_tx)
        self.send_tx(ehf_invalid_tx, expected_error='bad-mnhf-non-ehf')
        node.generate(1)
        ehf_height = node.getblockcount()
        self.sync_all()

        ehf_block = node.getbestblockhash()
        self.log.info(f"Check MnEhfTx {ehf_tx_sent} was mined in {ehf_block}")
        assert ehf_tx_sent in node.getblock(ehf_block)['tx']
        assert ehf_unknown_tx_sent in node.getblock(ehf_block)['tx']

        self.log.info(f"MnEhf tx: '{ehf_tx}' is sent: {ehf_tx_sent}")
        self.log.info(f"MnEhf 'unknown' tx: '{ehf_unknown_tx}' is sent: {ehf_unknown_tx_sent}")
        self.log.info(f"mempool: {node.getmempoolinfo()}")
        assert_equal(node.getmempoolinfo()['size'], 0)

        while (node.getblockcount() + 1) % 12 != 0:
            self.check_fork('defined')
            node.generate(1)
            self.sync_all()


        self.restart_all_nodes()

        for i in range(12):
            self.check_fork('started')
            node.generate(1)
            self.sync_all()


        for i in range(12):
            self.check_fork('locked_in')
            node.generate(1)
            self.sync_all()
            if i == 7:
                self.restart_all_nodes()

        self.check_fork('active')

        block_fork_active = node.getbestblockhash()
        self.log.info(f"Invalidate block: {ehf_block} with tip {block_fork_active}")
        for inode in self.nodes:
            inode.invalidateblock(ehf_block)

        self.log.info("Expecting for fork to be defined in next blocks because no MnEHF tx here")
        for i in range(12):
            self.check_fork('defined')
            node.generate(1)
            self.sync_all()


        self.log.info("Re-sending MnEHF for new fork")
        tx_sent_2 = self.send_tx(ehf_tx)
        node.generate(1)
        self.sync_all()

        ehf_block_2 = node.getbestblockhash()
        self.log.info(f"Check MnEhfTx again {tx_sent_2} was mined in {ehf_block_2}")
        assert tx_sent_2 in node.getblock(ehf_block_2)['tx']

        self.log.info(f"Generate some more block to jump to `started` status")
        for i in range(12):
            node.generate(1)
        self.check_fork('started')
        self.restart_all_nodes()
        self.check_fork('started')


        self.log.info(f"Re-consider block {ehf_block} to the old MnEHF and forget new fork")
        for inode in self.nodes:
            inode.reconsiderblock(ehf_block)
        assert_equal(node.getbestblockhash(), block_fork_active)

        self.check_fork('active')
        self.restart_all_nodes()
        self.check_fork('active')

        self.bump_mocktime(int(60 * 60 * 24 * 14))
        node.generate(1)
        self.sync_all()
        self.restart_all_nodes(params=[self.mocktime, self.mocktime + 1000000])
        self.mine_quorum()

        ehf_tx_second = self.create_mnehf(28, pubkey)
        assert_equal(get_bip9_details(node, 'testdummy')['status'], 'defined')

        self.log.info("Testing EHF signal with same bit")
        self.log.info(f"Previous signal at height={ehf_height}, total blocks={node.getblockcount()}, should success at {ehf_height + 576}")
        self.slowly_generate_batch(576 - (node.getblockcount() - ehf_height) - 1)
        ehf_tx_sent = self.send_tx(ehf_tx_second)
        self.log.info("Mine block and ensure not mined yet...")
        node.generate(1)
        self.ensure_tx_is_not_mined(ehf_tx_sent)
        node.generate(1)
        self.sync_all()
        self.log.info("Mine one more block and ensure it is mined")
        block = node.getblock(node.getbestblockhash())
        assert ehf_tx_sent in block['tx']

        self.check_fork('defined')
        self.slowly_generate_batch(12 * 4)
        self.check_fork('active')



if __name__ == '__main__':
    MnehfTest().main()
