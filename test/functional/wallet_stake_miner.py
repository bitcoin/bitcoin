#!/usr/bin/env python3
"""Verify stake miner thread lifecycle and block creation."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import COIN, hash256, uint256_from_compact
from test_framework.util import assert_equal

STAKE_TIMESTAMP_MASK = 0xF
MIN_STAKE_AGE = 60 * 60


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


class StakeMinerLifecycleTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def find_ntime(self, prev_hash, prev_height, prev_time, nbits, stake_hash, stake_time, amount, prevout):
        ntime = prev_time + 16
        while not check_kernel(
            prev_hash,
            prev_height,
            prev_time,
            nbits,
            stake_hash,
            stake_time,
            amount,
            prevout,
            ntime,
        ):
            ntime += 16
        return ntime

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(200, addr)

        unspent = node.listunspent()
        u1 = unspent[0]
        u2 = unspent[1]

        # First block
        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(u1["txid"])["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]
        amount = int(u1["amount"] * COIN)
        prevout = {"txid": u1["txid"], "vout": u1["vout"]}
        ntime1 = self.find_ntime(prev_hash, prev_height, prev_time, nbits, stake_block_hash, stake_time, amount, prevout)

        node.setmocktime(ntime1)
        assert node.walletstaking(True)
        self.wait_until(lambda: node.getblockcount() == prev_height + 1)
        assert_equal(node.getblockcount(), prev_height + 1)
        assert not node.walletstaking(False)

        # Second block after restart
        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(u2["txid"])["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]
        amount = int(u2["amount"] * COIN)
        prevout = {"txid": u2["txid"], "vout": u2["vout"]}
        ntime2 = self.find_ntime(prev_hash, prev_height, prev_time, nbits, stake_block_hash, stake_time, amount, prevout)

        node.setmocktime(ntime2)
        assert node.walletstaking(True)
        self.wait_until(lambda: node.getblockcount() == prev_height + 1)
        assert_equal(node.getblockcount(), prev_height + 1)
        assert not node.walletstaking(False)


if __name__ == "__main__":
    StakeMinerLifecycleTest(__file__).main()
