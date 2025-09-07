#!/usr/bin/env python3
"""Test proof-of-stake reorg behavior."""

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


def stake_block(node):
    unspent = node.listunspent()[0]
    txid = unspent["txid"]
    vout = unspent["vout"]
    amount = int(unspent["amount"] * COIN)
    prevout = {"txid": txid, "vout": vout}

    prev_height = node.getblockcount()
    prev_hash = node.getbestblockhash()
    prev_block = node.getblock(prev_hash)
    nbits = int(prev_block["bits"], 16)
    prev_time = prev_block["time"]

    stake_block_hash = node.gettransaction(txid)["blockhash"]
    stake_time = node.getblock(stake_block_hash)["time"]

    ntime = prev_time + 16
    while not check_kernel(
        prev_hash,
        prev_height,
        prev_time,
        nbits,
        stake_block_hash,
        stake_time,
        amount,
        prevout,
        ntime,
    ):
        ntime += 16

    script = CScript(bytes.fromhex(unspent["scriptPubKey"]))
    coinstake = CTransaction()
    coinstake.nLockTime = prev_height + 1
    coinstake.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
    coinstake.vout.append(CTxOut(0, CScript()))
    reward = 50 * COIN
    coinstake.vout.append(CTxOut(amount + reward, script))
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
    assert_equal(node.getblockcount(), prev_height + 1)


class PosReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def run_test(self):
        node0, node1 = self.nodes
        addr0 = node0.getnewaddress()
        node0.generatetoaddress(150, addr0)
        self.sync_all()
        start_height = node0.getblockcount()

        # Give node1 access to staking funds
        node1.importprivkey(node0.dumpprivkey(addr0))

        self.disconnect_nodes(0, 1)

        stake_block(node0)
        stake_block(node1)
        stake_block(node1)

        self.connect_nodes(0, 1)
        self.sync_blocks()
        expected_height = start_height + 2
        assert_equal(node0.getblockcount(), expected_height)
        assert_equal(node1.getblockcount(), expected_height)
        assert_equal(node0.getbestblockhash(), node1.getbestblockhash())


if __name__ == "__main__":
    PosReorgTest(__file__).main()
