"""
:author: Bryan Bishop <kanzure@gmail.com>
"""

import unittest
import random
from copy import copy

from bitcoin.core import (
    # bytes to hex (see x)
    b2x,

    # convert hex string to bytes (see b2x)
    x,

    # convert little-endian hex string to bytes (see b2lx)
    lx,

    # convert bytes to little-endian hex string (see lx)
    b2lx,

    # number of satoshis per bitcoin
    COIN,

    # a type for a transaction that isn't finished building
    CMutableTransaction,
    CMutableTxIn,
    CMutableTxOut,
    COutPoint,
    CTxIn,
)

from bitcoin.wallet import (
    # has a nifty function from_pubkey
    P2PKHBitcoinAddress,
)

from bitcoin.tests.fakebitcoinproxy import (
    FakeBitcoinProxy,
    make_txout,
    make_blocks_from_blockhashes,
    make_rpc_batch_request_entry,

    # TODO: import and test with FakeBitcoinProxyException
)

class FakeBitcoinProxyTestCase(unittest.TestCase):
    def test_constructor(self):
        FakeBitcoinProxy()

    def test_constructor_accepts_blocks(self):
        blockhash0 = "blockhash0"
        blockhash1 = "blockhash1"

        blocks = [
            {"hash": blockhash0},
            {"hash": blockhash1},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)

        self.assertNotEqual(proxy.blocks, {})
        self.assertTrue(proxy.blocks is blocks)

    def test_getblock_with_string(self):
        blockhash0 = "blockhash0"
        blockhash1 = "blockhash1"

        blocks = [
            {"hash": blockhash0},
            {"hash": blockhash1},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)
        result = proxy.getblock(blockhash0)

        self.assertEqual(type(result), dict)

        # should have a height now
        self.assertTrue("height" in result.keys())
        self.assertEqual(result["height"], 0)

    def test_blockheight_extensively(self):
        blockhashes = ["blockhash{}".format(x) for x in range(0, 10)]

        blocks = []
        for blockhash in blockhashes:
            blocks.append({"hash": blockhash})

        proxy = FakeBitcoinProxy(blocks=blocks)

        for (expected_height, blockhash) in enumerate(blockhashes):
            blockdata = proxy.getblock(blockhash)
            self.assertEqual(blockdata["height"], expected_height)

    def test_getblock_with_bytes(self):
        blockhash0 = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936301"

        blocks = [
            {"hash": blockhash0},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)
        result = proxy.getblock(lx(blockhash0))

        self.assertEqual(type(result), dict)

    def test_getblock_returns_transaction_data(self):
        blockhash0 = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936302"

        transaction_txids = ["foo", "bar"]

        blocks = [
            {"hash": blockhash0, "tx": transaction_txids},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)
        result = proxy.getblock(blockhash0)

        self.assertTrue("tx" in result.keys())
        self.assertEqual(type(result["tx"]), list)
        self.assertEqual(result["tx"], transaction_txids)

    def test_getblockhash_zero(self):
        blockhash0 = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936302"

        blocks = [
            {"hash": blockhash0},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)

        blockhash_result = proxy.getblockhash(0)

        self.assertEqual(blockhash_result, blockhash0)

    def test_getblockhash_many(self):
        blockhashes = [
            "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936302",
            "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936303",
            "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936304",
            "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936305",
        ]

        blocks = make_blocks_from_blockhashes(blockhashes)
        proxy = FakeBitcoinProxy(blocks=blocks)

        for (height, expected_blockhash) in enumerate(blockhashes):
            blockhash_result = proxy.getblockhash(height)
            self.assertEqual(blockhash_result, expected_blockhash)

    def test_getblockcount_zero(self):
        blocks = []
        proxy = FakeBitcoinProxy(blocks=blocks)
        count = proxy.getblockcount()
        self.assertEqual(count, len(blocks) - 1)

    def test_getblockcount_many(self):
        blockhash0 = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea64936301"
        blockhash1 = "0000b9fc70e9a8bb863a8c37f0f3d10d4d3e99e0e94bd42b35f0fdbb497399eb"

        blocks = [
            {"hash": blockhash0},
            {"hash": blockhash1, "previousblockhash": blockhash0},
        ]

        proxy = FakeBitcoinProxy(blocks=blocks)
        count = proxy.getblockcount()
        self.assertEqual(count, len(blocks) - 1)

    def test_getrawtransaction(self):
        txids = [
            "b9e3b3366b31136bd262c64ff6e4c29918a457840c5f53cbeeeefa41ca3b7005",
            "21886e5c02049751acea9d462359d68910bef938d9a58288c961d18b6e50daef",
            "b2d0cc6840be86516ce79ed7249bfd09f95923cc27784c2fa0dd910bfdb03173",
            "eacb81d11a539047af73b5d810d3683af3b1171e683dcbfcfb2819286d762c0c",
            "eee72e928403fea7bca29f366483c3e0ab629ac5dce384a0f541aacf6f810d30",
        ]

        txdefault = {"tx": "", "confirmations": 0}

        transactions = dict([(txid, copy(txdefault)) for txid in txids])

        # just making sure...
        self.assertTrue(transactions[txids[0]] is not transactions[txids[1]])

        proxy = FakeBitcoinProxy(transactions=transactions)

        for txid in txids:
            txdata = proxy.getrawtransaction(txid)

            self.assertEqual(type(txdata), dict)
            self.assertTrue("tx" in txdata.keys())
            self.assertEqual(type(txdata["tx"]), str)
            self.assertEqual(txdata["tx"], "")
            self.assertTrue("confirmations" in txdata.keys())
            self.assertEqual(txdata["confirmations"], 0)

    def test_getnewaddress_returns_different_addresses(self):
        num_addresses = random.randint(10, 100)
        addresses = set()

        proxy = FakeBitcoinProxy()

        for each in range(num_addresses):
            address = proxy.getnewaddress()
            addresses.add(str(address))

        self.assertEqual(len(addresses), num_addresses)

    def test_getnewaddress_returns_cbitcoinaddress(self):
        proxy = FakeBitcoinProxy()
        address = proxy.getnewaddress()
        self.assertEqual(type(address), P2PKHBitcoinAddress)

    def test_importaddress(self):
        proxy = FakeBitcoinProxy()
        proxy.importaddress("foo")

    def test_importaddress_with_parameters(self):
        proxy = FakeBitcoinProxy()
        address = "some_address"
        label = ""
        rescan = False
        proxy.importaddress(address, label, rescan)

    def test_fundrawtransaction(self):
        unfunded_transaction = CMutableTransaction([], [make_txout() for x in range(0, 5)])
        proxy = FakeBitcoinProxy()

        funded_transaction_hex = proxy.fundrawtransaction(unfunded_transaction)["hex"]
        funded_transaction = CMutableTransaction.deserialize(x(funded_transaction_hex))

        self.assertTrue(unfunded_transaction is not funded_transaction)
        self.assertEqual(type(funded_transaction), type(unfunded_transaction))
        self.assertNotEqual(len(funded_transaction.vin), 0)
        self.assertTrue(len(funded_transaction.vin) > len(unfunded_transaction.vin))
        self.assertEqual(type(funded_transaction.vin[0]), CTxIn)

    def test_fundrawtransaction_hex_hash(self):
        unfunded_transaction = CMutableTransaction([], [make_txout() for x in range(0, 5)])
        proxy = FakeBitcoinProxy()

        funded_transaction_hex = proxy.fundrawtransaction(b2x(unfunded_transaction.serialize()))["hex"]
        funded_transaction = CMutableTransaction.deserialize(x(funded_transaction_hex))

        self.assertTrue(unfunded_transaction is not funded_transaction)
        self.assertEqual(type(funded_transaction), type(unfunded_transaction))
        self.assertNotEqual(len(funded_transaction.vin), 0)
        self.assertTrue(len(funded_transaction.vin) > len(unfunded_transaction.vin))
        self.assertEqual(type(funded_transaction.vin[0]), CTxIn)

    def test_fundrawtransaction_adds_output(self):
        num_outputs = 5
        unfunded_transaction = CMutableTransaction([], [make_txout() for x in range(0, num_outputs)])
        proxy = FakeBitcoinProxy()

        funded_transaction_hex = proxy.fundrawtransaction(b2x(unfunded_transaction.serialize()))["hex"]
        funded_transaction = CMutableTransaction.deserialize(x(funded_transaction_hex))

        self.assertTrue(len(funded_transaction.vout) > num_outputs)
        self.assertEqual(len(funded_transaction.vout), num_outputs + 1)

    def test_signrawtransaction(self):
        num_outputs = 5
        given_transaction = CMutableTransaction([], [make_txout() for x in range(0, num_outputs)])
        proxy = FakeBitcoinProxy()

        result = proxy.signrawtransaction(given_transaction)

        self.assertEqual(type(result), dict)
        self.assertTrue("hex" in result.keys())

        result_transaction_hex = result["hex"]
        result_transaction = CMutableTransaction.deserialize(x(result_transaction_hex))

        self.assertTrue(result_transaction is not given_transaction)
        self.assertTrue(result_transaction.vin is not given_transaction.vin)
        self.assertTrue(result_transaction.vout is not given_transaction.vout)
        self.assertEqual(len(result_transaction.vout), len(given_transaction.vout))
        self.assertEqual(result_transaction.vout[0].scriptPubKey, given_transaction.vout[0].scriptPubKey)

    def test_sendrawtransaction(self):
        num_outputs = 5
        given_transaction = CMutableTransaction([], [make_txout() for x in range(0, num_outputs)])
        expected_txid = b2lx(given_transaction.GetHash())
        given_transaction_hex = b2x(given_transaction.serialize())
        proxy = FakeBitcoinProxy()
        resulting_txid = proxy.sendrawtransaction(given_transaction_hex)
        self.assertEqual(resulting_txid, expected_txid)

    def test__batch_empty_list_input(self):
        requests = []
        self.assertEqual(len(requests), 0) # must be empty for test
        proxy = FakeBitcoinProxy()
        results = proxy._batch(requests)
        self.assertEqual(type(results), list)
        self.assertEqual(len(results), 0)

    def test__batch_raises_when_no_params(self):
        proxy = FakeBitcoinProxy()
        with self.assertRaises(TypeError):
            proxy._batch()

    def test__batch_same_count_results_as_requests(self):
        proxy = FakeBitcoinProxy()
        request = make_rpc_batch_request_entry("getblockcount", [])
        requests = [request]
        results = proxy._batch(requests)
        self.assertEqual(len(requests), len(results))

    def test__batch_gives_reasonable_getblockcount_result(self):
        proxy = FakeBitcoinProxy()
        request = make_rpc_batch_request_entry("getblockcount", [])
        requests = [request]
        results = proxy._batch(requests)
        self.assertEqual(results[0]["result"], -1)

    def test__batch_result_keys(self):
        expected_keys = ["error", "id", "result"]
        proxy = FakeBitcoinProxy()
        request = make_rpc_batch_request_entry("getblockcount", [])
        requests = [request]
        results = proxy._batch(requests)
        result = results[0]
        self.assertTrue(all([expected_key in result.keys() for expected_key in expected_keys]))

    def test__batch_result_error_is_none(self):
        proxy = FakeBitcoinProxy()
        request = make_rpc_batch_request_entry("getblockcount", [])
        requests = [request]
        results = proxy._batch(requests)
        result = results[0]
        self.assertEqual(result["error"], None)

    def test__batch_returns_error_when_given_invalid_params(self):
        params = 1
        proxy = FakeBitcoinProxy()
        request = make_rpc_batch_request_entry("getblockcount", params)
        requests = [request]
        results = proxy._batch(requests)
        result = results[0]
        self.assertEqual(type(result), dict)
        self.assertIn("error", result.keys())
        self.assertIn("id", result.keys())
        self.assertIn("result", result.keys())
        self.assertEqual(result["result"], None)
        self.assertEqual(type(result["error"]), dict)
        self.assertIn("message", result["error"].keys())
        self.assertIn("code", result["error"].keys())
        self.assertEqual(result["error"]["message"], "Params must be an array")

    def test__batch_getblockhash_many(self):
        block_count = 100
        blocks = []

        previous_blockhash = None
        for counter in range(0, block_count):
            blockhash = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea649{}".format(counter)
            block_data = {
                "hash": blockhash,
            }

            if previous_blockhash:
                block_data["previous_blockhash"] = previous_blockhash

            blocks.append(block_data)
            previous_blockhash = blockhash

        proxy = FakeBitcoinProxy(blocks=blocks)
        count = proxy.getblockcount()
        self.assertEqual(count, len(blocks) - 1)

        requests = []
        for counter in range(0, count + 1): # from 0 to len(blocks)
            request = make_rpc_batch_request_entry("getblockhash", [counter])
            requests.append(request)

        results = proxy._batch(requests)

        self.assertEqual(type(results), list)
        self.assertEqual(len(results), len(requests))

        for (counter, result) in enumerate(results):
            self.assertEqual(result["error"], None)

            expected_blockhash = "00008c0c84aee66413f1e8ff95fdca5e8ebf35c94b090290077cdcea649{}".format(counter)
            self.assertEqual(result["result"], expected_blockhash)

    def test_make_blocks_from_blockhashes(self):
        blockhashes = ["blah{}".format(x) for x in range(0, 25)]
        blocks = make_blocks_from_blockhashes(blockhashes)

        self.assertEqual(type(blocks), list)
        self.assertEqual(len(blocks), len(blockhashes))
        self.assertEqual(type(blocks[0]), dict)
        self.assertTrue(all(["hash" in block.keys() for block in blocks]))
        self.assertTrue(all(["height" in block.keys() for block in blocks]))
        self.assertTrue(all(["tx" in block.keys() for block in blocks]))
        self.assertNotIn("previousblockhash", blocks[0].keys())
        self.assertTrue(all(["previousblockhash" in block.keys() for block in blocks[1:]]))
        self.assertEqual(blocks[-1]["previousblockhash"], blocks[-2]["hash"])
        self.assertEqual(sorted([block["hash"] for block in blocks]), sorted(blockhashes))

    def test_make_blocks_from_blockhashes_empty(self):
        blockhashes = []
        blocks = make_blocks_from_blockhashes(blockhashes)
        self.assertEqual(type(blocks), list)
        self.assertEqual(len(blocks), len(blockhashes))

        # Just in case for some reason the function ever appends to the given
        # list (incorrectly)..
        self.assertEqual(len(blockhashes), 0)
        self.assertEqual(len(blocks), 0)

        # and because paranoia
        self.assertTrue(blockhashes is not blocks)

if __name__ == "__main__":
    unittest.main()
