#!/usr/bin/env python3
"""Verify PoS difficulty retargeting over multiple blocks."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import (
    CTransaction,
    CTxIn,
    CTxOut,
    COutPoint,
    COIN,
    hash256,
    uint256_from_compact,
)
from test_framework.script import CScript
from test_framework.util import assert_equal

STAKE_TIMESTAMP_MASK = 0xF
MIN_STAKE_AGE = 60 * 60
TARGET_SPACING = 16
INTERVAL = 24 * 60 * 60 // (8 * 60)  # matches regtest params


def check_kernel(prev_hash, prev_height, prev_time, nbits, stake_hash, stake_time, amount, prevout, ntime):
    if ntime & STAKE_TIMESTAMP_MASK:
        return False
    if ntime <= stake_time or ntime - stake_time < MIN_STAKE_AGE:
        return False
    stake_modifier = hash256(
        bytes.fromhex(prev_hash)[::-1]
        + prev_height.to_bytes(4, "little")
        + prev_time.to_bytes(4, "little")
    )
    ntime_masked = ntime & ~STAKE_TIMESTAMP_MASK
    stake_time_masked = stake_time & ~STAKE_TIMESTAMP_MASK
    data = (
        stake_modifier
        + bytes.fromhex(stake_hash)[::-1]
        + stake_time_masked.to_bytes(4, "little")
        + bytes.fromhex(prevout["txid"])[::-1]
        + prevout["vout"].to_bytes(4, "little")
        + ntime_masked.to_bytes(4, "little")
    )
    proof = hash256(data)
    target = uint256_from_compact(nbits) * (amount // COIN)
    return int.from_bytes(proof[::-1], "big") <= target


def next_target(prev_bits, prev_mtp, block_time):
    actual_spacing = block_time - prev_mtp
    if actual_spacing < 0:
        actual_spacing = TARGET_SPACING
    target = uint256_from_compact(prev_bits)
    target *= ((INTERVAL - 1) * TARGET_SPACING + 2 * actual_spacing)
    target //= ((INTERVAL + 1) * TARGET_SPACING)
    return target


class PosDifficultyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def stake_block(self, node, stake, spacing):
        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        prev_time = prev_block["time"]
        prev_bits = int(prev_block["bits"], 16)
        prev_mtp = prev_block["mediantime"]

        ntime = prev_time + spacing
        while not check_kernel(
            prev_hash,
            prev_height,
            prev_time,
            prev_bits,
            stake["block_hash"],
            stake["time"],
            stake["amount"],
            {"txid": stake["txid"], "vout": stake["vout"]},
            ntime,
        ):
            ntime += TARGET_SPACING

        coinstake = CTransaction()
        coinstake.nLockTime = prev_height + 1
        coinstake.vin.append(CTxIn(COutPoint(int(stake["txid"], 16), stake["vout"])))
        coinstake.vout.append(CTxOut(0, CScript()))
        reward = 50 * COIN
        coinstake.vout.append(CTxOut(stake["amount"] + reward, stake["script"]))
        signed_hex = node.signrawtransactionwithwallet(coinstake.serialize().hex())["hex"]
        coinstake = CTransaction()
        coinstake.deserialize(bytes.fromhex(signed_hex))

        coinbase = create_coinbase(prev_height + 1, nValue=0)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[coinstake],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        node.submitblock(block.serialize().hex())

        new_block_hash = node.getbestblockhash()
        new_block = node.getblock(new_block_hash)
        expected = next_target(prev_bits, prev_mtp, ntime)
        new_target = uint256_from_compact(int(new_block["bits"], 16))
        assert_equal(new_target, expected)

        txid = hash256(coinstake.serialize_without_witness())[::-1].hex()
        return {
            "txid": txid,
            "vout": 1,
            "amount": stake["amount"] + reward,
            "script": stake["script"],
            "block_hash": new_block_hash,
            "time": ntime,
        }

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(150, addr)

        unspent = node.listunspent()[0]
        stake_block_hash = node.gettransaction(unspent["txid"]) ["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]
        stake = {
            "txid": unspent["txid"],
            "vout": unspent["vout"],
            "amount": int(unspent["amount"] * COIN),
            "script": CScript(bytes.fromhex(unspent["scriptPubKey"])),
            "block_hash": stake_block_hash,
            "time": stake_time,
        }

        for spacing in [TARGET_SPACING, TARGET_SPACING * 10, TARGET_SPACING]:
            stake = self.stake_block(node, stake, spacing)


if __name__ == "__main__":
    PosDifficultyTest(__file__).main()
