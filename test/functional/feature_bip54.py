#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test Consensus Cleanup soft fork (BIP 54)

from test_framework.authproxy import JSONRPCException
from test_framework.blocktools import (
    add_witness_commitment,
    create_block,
    NORMAL_GBT_REQUEST_PARAMS,
    VERSIONBITS_LAST_OLD_BLOCK_VERSION,
)
from test_framework.messages import (
    CBlockHeader,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_ENDIF,
    OP_NOT,
    OP_NOTIF,
    OP_1,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class Bip54Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # BIP54 will be activated halfway through the test.
        self.extra_args = [["-vbparams=consensuscleanup:0:3999999999"]]

    def generate_bulk(self, node, count, version=VERSIONBITS_LAST_OLD_BLOCK_VERSION):
        """Generate a number of blocks at once, controlling the block version."""
        chain_info = node.getblockchaininfo()
        prev_height = chain_info["blocks"]
        prev_hash = chain_info["bestblockhash"]
        prev_time = chain_info["time"]
        for _ in range(count):
            block = create_block(tmpl={
                "previousblockhash": prev_hash,
                "height": prev_height + 1,
                "curtime": prev_time + 1,
                "version": version,
            })
            block.solve()
            res = node.submitblock(block.serialize().hex())
            assert_equal(res, None)
            prev_time += 1
            prev_height += 1
            prev_hash = block.hash_hex

    def try_submit_block(self, node, block):
        """Submit this block to that node, raising a JSONRPC error if it is rejected."""
        err = node.submitblock(block.serialize().hex())
        if err is not None:
            raise JSONRPCException({"message": err, "code": -25})

    def mine_and_submit(self, node, txs):
        """Mine these transactions into a block and submit it to the provided node."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(txlist=txs, tmpl=template)
        add_witness_commitment(block)
        block.solve()
        self.try_submit_block(node, block)

    def reach_end_retarget_period(self, node):
        """Mine all the way to the very last block in the current retarget period."""
        blk_count = 144 - node.getblockcount() % 144 - 1
        self.generate_bulk(node, blk_count)

    def create_prep_tx(self, spk):
        """Create a transaction that spends the full value of a wallet coin to the given scriptPubKey."""
        prevout = self.wallet.get_utxo(confirmed_only=True)
        prevout_sats = int(COIN * prevout["value"])
        prep_tx = CTransaction()
        prep_tx.vin = [CTxIn(COutPoint(int(prevout['txid'], 16), prevout['vout']))]
        prep_tx.vout = [CTxOut(prevout_sats, spk)]
        self.wallet.sign_tx(prep_tx)
        return prep_tx

    def create_spend_tx(self, prev_tx, vout, script_sig):
        """Create a transaction that spends the provided transaction output to the wallet scriptPubKey."""
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(prev_tx.txid_int, vout), scriptSig=script_sig)]
        tx.vout = [CTxOut(prev_tx.vout[vout].nValue, self.wallet.get_output_script())]
        return tx

    def create_txs_many_sigops(self):
        """Create a list of transactions that fits in a single block and violates the BIP54 sigops limit. """
        # A bare scriptPubKey that accounts for 2500 sigops
        prep_spk = CScript([OP_DUP, OP_NOTIF] + [OP_CHECKMULTISIG] * 125 + [OP_ENDIF])
        prep_tx = self.create_prep_tx(prep_spk)

        # One more sigop in the scriptSig to make it exceed the limit
        _, dummy_pubkey = generate_keypair()
        script_sig = CScript([b"", dummy_pubkey, OP_CHECKSIG, OP_NOT])
        tx = self.create_spend_tx(prep_tx, 0, script_sig)

        return [prep_tx, tx]

    def submit_block_many_sigops(self, node):
        """Create and submit a block that violates the BIP54 sigops limit."""
        txs = [tx.serialize().hex() for tx in self.create_txs_many_sigops()]
        self.mine_and_submit(node, txs)

    def submit_block_many_sigops_split(self, node):
        """Create and submit a block that exceeds the BIP54 sigops limit but in multiple txs."""
        # Get the very same transactions, except we drop the sigop in the last tx..
        txs = self.create_txs_many_sigops()
        last_tx = txs[-1]
        last_tx.vin[0].scriptSig = CScript([OP_1])

        # ..And add it to a separate transaction instead.
        _, dummy_pubkey = generate_keypair()
        prep_spk = CScript([b"", dummy_pubkey, OP_CHECKSIG, OP_NOT])
        prep_tx = self.create_prep_tx(prep_spk)
        tx = self.create_spend_tx(prep_tx, 0, script_sig=CScript())
        txs += [prep_tx, tx]

        # Now submit the two pairs of transactions in a single block.
        self.mine_and_submit(node, txs)

    def timewarp_attack(self, node):
        """Perform a pseudo Timewarp attack. Pseudo because regtest does not have retargets, so we only do the first period."""
        # Reach the end of the current difficulty adjustment period.
        blk_count = 144 - node.getblockcount() % 144
        self.generate_bulk(node, blk_count)

        # Record the start time to later mock elapsed time as we mined an entire period of blocks.
        start_time = node.getblockheader(node.getbestblockhash())["time"]

        # Now mine all blocks until the very last by holding back timestamps as much as MTP allows.
        height = node.getblockcount() + 1
        prev_hash = node.getbestblockhash()
        ntime = start_time + 1
        while height % 144 < 143:
            block = create_block(tmpl={
                "previousblockhash": prev_hash,
                "height": height,
                "curtime": ntime,
            })
            block.solve()
            node.submitheader(CBlockHeader(block).serialize().hex())
            prev_hash = block.hash_hex
            height += 1
            if height % 6 == 0:
                ntime += 1

        # Pretend it takes us 10 minutes per block interval to mine this difficulty adjustment
        # (including the 144th block below).
        elapsed_time = 143 * 10 * 60
        mocked_time = int(start_time + elapsed_time)
        node.setmocktime(mocked_time)

        # Mine the last block of the period as far in the future as possible.
        block = create_block(tmpl={
            "previousblockhash": prev_hash,
            "height": height,
            "curtime": mocked_time + 2 * 60 * 60,  # 2h in the future.
        })
        block.solve()
        node.submitheader(CBlockHeader(block).serialize().hex())
        prev_hash = block.hash_hex
        height += 1

        # Finally, reset the time back to that of the 5 blocks prior when mining the first block
        # of the next period. If we were not on regtest the difficulty would have been dropped
        # slightly and we would be set to reduce it by 4x at the end of the upcoming difficulty
        # adjustment period. If BIP54 is active this block will be refused by the node.
        assert_equal(height % 144, 0)
        ntime = start_time + int(144 / 6) + 1
        block = create_block(tmpl={
            "previousblockhash": prev_hash,
            "height": height,
            "curtime": ntime,
        })
        block.solve()

        try:
            node.submitheader(CBlockHeader(block).serialize().hex())
        except Exception as e:
            raise e
        finally:
            # We only submit headers because it's sufficient to test the mitigation and so that we don't
            # affect the other tests. Reset the time so those can mine blocks normally.
            node.setmocktime(0)

    def murch_zawy_attack(self, node):
        """Perform a pseudo Timewarp attack. Pseudo because regtest does not have retargets, so we only do the first 3 periods."""
        # Expected duration in seconds of a regtest retarget interval (by the incorrect formula).
        interval_expected_sec = 144 * 10 * 60

        # Start on a fresh retarget period.
        self.reach_end_retarget_period(node)

        # Record the start time to later mock elapsed time as we mined several periods of blocks.
        start_time = node.getblockheader(node.getbestblockhash())["time"]

        # The Murch-Zawy attack reduces the difficulty by pushing timestamp forwards and back
        # to the minimum time, exploiting the difficulty reduction floor of /4 and the fact
        # that the elapsed time in a retarget period is allowed to be negative. See for more
        # https://delvingbitcoin.org/t/zawy-s-alternating-timestamp-attack/1062 (especially
        # the "Variant on Zawy's Attack" section).
        # Because timestamps are postdated, blocks can't be submitted until later. Record
        # them in the meantime then submit them all at once.
        blocks = []

        # Mine the first retarget period, holding back the timestamps of all blocks in the
        # period except the very last one.
        prev_hash = node.getbestblockhash()
        prev_header = node.getblockheader(prev_hash)
        height = prev_header["height"] + 1
        ntime = prev_header["time"] + 1
        while height % 144 < 143:
            block = create_block(tmpl={
                "previousblockhash": prev_hash,
                "height": height,
                "curtime": ntime,
            })
            block.solve()
            blocks.append(block)
            prev_hash = block.hash_hex
            height += 1
            if height % 6 == 0:
                ntime += 1

        # The timestamp of the last block of the period is set to maximally reduce difficulty.
        block = create_block(tmpl={
            "previousblockhash": blocks[-1].hash_hex,
            "height": height,
            "curtime": start_time +  4 * interval_expected_sec,
        })
        block.solve()
        blocks.append(block)
        height += 1

        # This attack also works with a strict timewarp fix. Mine the first block of the
        # next retarget period with a timestamp no less than that of the last block of the
        # previous period.
        block = create_block(tmpl={
            "previousblockhash": blocks[-1].hash_hex,
            "height": height,
            "curtime": blocks[-1].nTime,
        })
        block.solve()
        blocks.append(block)
        height += 1
        ntime += 1

        # Mine the second retarget period, holding back timestamps.
        while height % 144 < 143:
            block = create_block(tmpl={
                "previousblockhash": blocks[-1].hash_hex,
                "height": height,
                "curtime": ntime,
            })
            block.solve()
            blocks.append(block)
            height += 1
            if height % 4 == 0:
                ntime += 1

        # The timestamp of the last block of the period is again set to maximally reduce difficulty.
        block = create_block(tmpl={
            "previousblockhash": blocks[-1].hash_hex,
            "height": height,
            "curtime": start_time + 4 * interval_expected_sec,
        })
        block.solve()
        blocks.append(block)
        height += 1

        # And the first block of the following period with the same time, to respect the timewarp fix.
        block = create_block(tmpl={
            "previousblockhash": blocks[-1].hash_hex,
            "height": height,
            "curtime": blocks[-1].nTime,
        })
        block.solve()
        blocks.append(block)
        height += 1
        ntime += 1

        # Mine one last full period of blocks, holding back timestamps. This time, include the last
        # block of the period.
        while height % 144 != 0:
            block = create_block(tmpl={
                "previousblockhash": blocks[-1].hash_hex,
                "height": height,
                "curtime": ntime,
            })
            block.solve()
            blocks.append(block)
            height += 1
            if height % 4 == 0:
                ntime += 1

        # Now if regtest actually had difficulty adjustment we'd be in a situation where the time
        # has been reset to its minimal value, yet the difficulty is 4x lower than when we started
        # the attack (because it was /4 twice and only *4 once). In an actual Murch-Zawy attack,
        # Mallory would repeat the process until the post-dated blocks can be submitted. A single
        # round is sufficient to demonstrate the attack, so we'll move forward and submit our chain.
        node.setmocktime(start_time + 2 * 4 * interval_expected_sec)
        try:
            for block in blocks:
                node.submitheader(CBlockHeader(block).serialize().hex())
        except Exception as e:
            raise e
        finally:
            # We only submit headers because it's sufficient to test the mitigation and so that we don't
            # affect the other tests. Reset the time so those can mine blocks normally.
            node.setmocktime(0)

    def submit_nontimelocked_cb(self, node):
        """Submit a block with its coinbase transaction not timelocked to its height."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=template)
        block.vtx[0].nLockTime -= 1
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        self.try_submit_block(node, block)

    def submit_block_64byte(self, node):
        """Submit a block that contains a 64-byte (Segwit) transaction."""
        # Create a 64-byte tx that spends a single Segwit input and contains a single p2a output.
        prevout = self.wallet.get_utxo(confirmed_only=True)
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(prevout['txid'], 16), prevout['vout']))]
        anchor_spk = CScript([OP_1, b"\x4e\x73"])
        tx.vout = [CTxOut(0, anchor_spk)]
        self.wallet.sign_tx(tx)
        assert_equal(len(tx.serialize_without_witness()), 64)

        # Mine a block containing that transaction.
        self.mine_and_submit(node, [tx])

    def submit_block_64byte_coinbase(self, node):
        """Submit a block that contains a 64-byte coinbase transaction."""
        template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=template)
        block.vtx[0].vout[0].scriptPubKey = CScript([0x00])
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        self.try_submit_block(node, block)

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.log.info("BIP54 tests before activation")
        # When BIP54 is not active we:
        # - Accept a block with more than 2500 BIP54-sigops in either a single transaction or
        #   split across multiple transactions.
        self.submit_block_many_sigops(node)
        self.submit_block_many_sigops_split(node)
        # - Can perform Timewarp and Murch-Zawy attacks.
        self.timewarp_attack(node)
        self.murch_zawy_attack(node)
        # - Accept a block with its coinbase not timelocked to the block height.
        self.submit_nontimelocked_cb(node)
        # - Accept a block containing a 64-byte transaction.
        self.submit_block_64byte(node)
        # - Even if that is the coinbase transaction.
        self.submit_block_64byte_coinbase(node)

        self.log.info("Activating BIP54")
        self.reach_end_retarget_period(node)

        # Mine enough signaling blocks to transition to locked in and reach the next period.
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "started")
        signal_bip54 = (1 << 29) | (1 << cc_dep["bit"])
        thresh = cc_dep["statistics"]["threshold"]
        self.generate_bulk(node, thresh, version=signal_bip54)
        self.generate_bulk(node, 144 - thresh + 1)

        # Mine one more retarget period to transition from locked in to active.
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "locked_in")
        self.generate_bulk(node, 144)
        cc_dep = node.getdeploymentinfo()["deployments"]["consensuscleanup"]["bip9"]
        assert_equal(cc_dep["status"], "active")

        self.log.info("BIP54 tests after activation")
        # Once BIP54 is active we:
        # - Refuse blocks with more than 2500 BIP54-sigops in a single transaction but still
        #   accept a block with the same number of BIP54-sigops split in more than one transaction.
        assert_raises_rpc_error(-25, "bad-txns-legacy-sigops", self.submit_block_many_sigops, node)
        self.submit_block_many_sigops_split(node)
        # - Cannot perform Timewarp and Murch-Zawy attacks
        assert_raises_rpc_error(-25, "time-timewarp-attack", self.timewarp_attack, node)
        assert_raises_rpc_error(-25, "time-negative-interval", self.murch_zawy_attack, node)
        # - Refuse blocks with a coinbase not timelocked to the block height
        assert_raises_rpc_error(-25, "bad-cb-locktime", self.submit_nontimelocked_cb, node)
        # - Refuse a block containing a 64-byte transaction
        assert_raises_rpc_error(-25, "bad-txns-size", self.submit_block_64byte, node)
        # - Even if that is the coinbase transaction
        assert_raises_rpc_error(-25, "bad-txns-size", self.submit_block_64byte_coinbase, node)


if __name__ == "__main__":
    Bip54Test(__file__).main()
