#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the REST API."""

import binascii
from decimal import Decimal
from enum import Enum
from io import BytesIO
import json
from struct import pack, unpack

import http.client
import urllib.parse

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
# SYSCOIN
from test_framework.auxpow_testing import mineAuxpowBlock
from test_framework.messages import BLOCK_HEADER_SIZE

class ReqType(Enum):
    JSON = 1
    BIN = 2
    HEX = 3

class RetType(Enum):
    OBJ = 1
    BYTES = 2
    JSON = 3

def filter_output_indices_by_value(vouts, value):
    for vout in vouts:
        if vout['value'] == value:
            yield vout['n']

class RESTTest (SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-rest"], []]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_rest_request(self, uri, http_method='GET', req_type=ReqType.JSON, body='', status=200, ret_type=RetType.JSON):
        rest_uri = '/rest' + uri
        if req_type == ReqType.JSON:
            rest_uri += '.json'
        elif req_type == ReqType.BIN:
            rest_uri += '.bin'
        elif req_type == ReqType.HEX:
            rest_uri += '.hex'

        conn = http.client.HTTPConnection(self.url.hostname, self.url.port)
        self.log.debug('%s %s %s', http_method, rest_uri, body)
        if http_method == 'GET':
            conn.request('GET', rest_uri)
        elif http_method == 'POST':
            conn.request('POST', rest_uri, body)
        resp = conn.getresponse()

        assert_equal(resp.status, status)

        if ret_type == RetType.OBJ:
            return resp
        elif ret_type == RetType.BYTES:
            return resp.read()
        elif ret_type == RetType.JSON:
            return json.loads(resp.read().decode('utf-8'), parse_float=Decimal)

    def run_test(self):
        self.url = urllib.parse.urlparse(self.nodes[0].url)
        self.log.info("Mine blocks and send Syscoin to node 1")

        # Random address so node1's balance doesn't increase
        not_related_address = "2MxqoHEdNQTyYeX1mHcbrrpzgojbosTpCvJ"

        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[1].generatetoaddress(100, not_related_address)
        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 50)

        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        self.sync_all()

        self.log.info("Test the /tx URI")

        json_obj = self.test_rest_request("/tx/{}".format(txid))
        assert_equal(json_obj['txid'], txid)

        # Check hex format response
        hex_response = self.test_rest_request("/tx/{}".format(txid), req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_greater_than_or_equal(int(hex_response.getheader('content-length')),
                                     json_obj['size']*2)

        spent = (json_obj['vin'][0]['txid'], json_obj['vin'][0]['vout'])  # get the vin to later check for utxo (should be spent by then)
        # get n of 0.1 outpoint
        n, = filter_output_indices_by_value(json_obj['vout'], Decimal('0.1'))
        spending = (txid, n)

        self.log.info("Query an unspent TXO using the /getutxos URI")

        self.nodes[1].generatetoaddress(1, not_related_address)
        self.sync_all()
        bb_hash = self.nodes[0].getbestblockhash()

        assert_equal(self.nodes[1].getbalance(), Decimal("0.1"))

        # Check chainTip response
        json_obj = self.test_rest_request("/getutxos/{}-{}".format(*spending))
        assert_equal(json_obj['chaintipHash'], bb_hash)

        # Make sure there is one utxo
        assert_equal(len(json_obj['utxos']), 1)
        assert_equal(json_obj['utxos'][0]['value'], Decimal('0.1'))

        self.log.info("Query a spent TXO using the /getutxos URI")

        json_obj = self.test_rest_request("/getutxos/{}-{}".format(*spent))

        # Check chainTip response
        assert_equal(json_obj['chaintipHash'], bb_hash)

        # Make sure there is no utxo in the response because this outpoint has been spent
        assert_equal(len(json_obj['utxos']), 0)

        # Check bitmap
        assert_equal(json_obj['bitmap'], "0")

        self.log.info("Query two TXOs using the /getutxos URI")

        json_obj = self.test_rest_request("/getutxos/{}-{}/{}-{}".format(*(spending + spent)))

        assert_equal(len(json_obj['utxos']), 1)
        assert_equal(json_obj['bitmap'], "10")

        self.log.info("Query the TXOs using the /getutxos URI with a binary response")

        bin_request = b'\x01\x02'
        for txid, n in [spending, spent]:
            bin_request += bytes.fromhex(txid)
            bin_request += pack("i", n)

        bin_response = self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.BIN, body=bin_request, ret_type=RetType.BYTES)
        output = BytesIO(bin_response)
        chain_height, = unpack("<i", output.read(4))
        response_hash = output.read(32)[::-1].hex()

        assert_equal(bb_hash, response_hash)  # check if getutxo's chaintip during calculation was fine
        assert_equal(chain_height, 102)  # chain height must be 102

        self.log.info("Test the /getutxos URI with and without /checkmempool")
        # Create a transaction, check that it's found with /checkmempool, but
        # not found without. Then confirm the transaction and check that it's
        # found with or without /checkmempool.

        # do a tx and don't sync
        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        json_obj = self.test_rest_request("/tx/{}".format(txid))
        # get the spent output to later check for utxo (should be spent by then)
        spent = (json_obj['vin'][0]['txid'], json_obj['vin'][0]['vout'])
        # get n of 0.1 outpoint
        n, = filter_output_indices_by_value(json_obj['vout'], Decimal('0.1'))
        spending = (txid, n)

        json_obj = self.test_rest_request("/getutxos/{}-{}".format(*spending))
        assert_equal(len(json_obj['utxos']), 0)

        json_obj = self.test_rest_request("/getutxos/checkmempool/{}-{}".format(*spending))
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request("/getutxos/{}-{}".format(*spent))
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request("/getutxos/checkmempool/{}-{}".format(*spent))
        assert_equal(len(json_obj['utxos']), 0)

        self.nodes[0].generate(1)
        self.sync_all()

        json_obj = self.test_rest_request("/getutxos/{}-{}".format(*spending))
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request("/getutxos/checkmempool/{}-{}".format(*spending))
        assert_equal(len(json_obj['utxos']), 1)

        # Do some invalid requests
        self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.JSON, body='{"checkmempool', status=400, ret_type=RetType.OBJ)
        self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.BIN, body='{"checkmempool', status=400, ret_type=RetType.OBJ)
        self.test_rest_request("/getutxos/checkmempool", http_method='POST', req_type=ReqType.JSON, status=400, ret_type=RetType.OBJ)

        # Test limits
        long_uri = '/'.join(["{}-{}".format(txid, n_) for n_ in range(20)])
        self.test_rest_request("/getutxos/checkmempool/{}".format(long_uri), http_method='POST', status=400, ret_type=RetType.OBJ)

        long_uri = '/'.join(['{}-{}'.format(txid, n_) for n_ in range(15)])
        self.test_rest_request("/getutxos/checkmempool/{}".format(long_uri), http_method='POST', status=200)

        mineAuxpowBlock(self.nodes[0])  # generate block to not affect upcoming tests
        self.sync_all()

        self.log.info("Test the /block, /blockhashbyheight and /headers URIs")
        bb_hash = self.nodes[0].getbestblockhash()

        # Check result if block does not exists
        assert_equal(self.test_rest_request('/headers/1/0000000000000000000000000000000000000000000000000000000000000000'), [])
        self.test_rest_request('/block/0000000000000000000000000000000000000000000000000000000000000000', status=404, ret_type=RetType.OBJ)

        # Check result if block is not in the active chain
        self.nodes[0].invalidateblock(bb_hash)
        assert_equal(self.test_rest_request('/headers/1/{}'.format(bb_hash)), [])
        self.test_rest_request('/block/{}'.format(bb_hash))
        self.nodes[0].reconsiderblock(bb_hash)

        # Check binary format
        response = self.test_rest_request("/block/{}".format(bb_hash), req_type=ReqType.BIN, ret_type=RetType.OBJ)
        assert_greater_than(int(response.getheader('content-length')), BLOCK_HEADER_SIZE)
        response_bytes = response.read()

        # Compare with block header
        response_header = self.test_rest_request("/headers/1/{}".format(bb_hash), req_type=ReqType.BIN, ret_type=RetType.OBJ)
        headerLen = int(response_header.getheader('content-length'))
        assert_greater_than(headerLen, BLOCK_HEADER_SIZE)
        response_header_bytes = response_header.read()
        assert_equal(response_bytes[:headerLen], response_header_bytes)

        # Check block hex format
        response_hex = self.test_rest_request("/block/{}".format(bb_hash), req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_greater_than(int(response_hex.getheader('content-length')), BLOCK_HEADER_SIZE*2)
        response_hex_bytes = response_hex.read().strip(b'\n')
        assert_equal(binascii.hexlify(response_bytes), response_hex_bytes)

        # Compare with hex block header
        response_header_hex = self.test_rest_request("/headers/1/{}".format(bb_hash), req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_greater_than(int(response_header_hex.getheader('content-length')), BLOCK_HEADER_SIZE*2)
        response_header_hex_bytes = response_header_hex.read().strip()
        headerLen = len (response_header_hex_bytes) // 2
        assert_equal(binascii.hexlify(response_bytes[:headerLen]), response_header_hex_bytes)

        # Check json format
        block_json_obj = self.test_rest_request("/block/{}".format(bb_hash))
        assert_equal(block_json_obj['hash'], bb_hash)
        assert_equal(self.test_rest_request("/blockhashbyheight/{}".format(block_json_obj['height']))['blockhash'], bb_hash)

        # Check hex/bin format
        resp_hex = self.test_rest_request("/blockhashbyheight/{}".format(block_json_obj['height']), req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_equal(resp_hex.read().decode('utf-8').rstrip(), bb_hash)
        resp_bytes = self.test_rest_request("/blockhashbyheight/{}".format(block_json_obj['height']), req_type=ReqType.BIN, ret_type=RetType.BYTES)
        blockhash = resp_bytes[::-1].hex()
        assert_equal(blockhash, bb_hash)

        # Check invalid blockhashbyheight requests
        resp = self.test_rest_request("/blockhashbyheight/abc", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Invalid height: abc")
        resp = self.test_rest_request("/blockhashbyheight/1000000", ret_type=RetType.OBJ, status=404)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Block height out of range")
        resp = self.test_rest_request("/blockhashbyheight/-1", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Invalid height: -1")
        self.test_rest_request("/blockhashbyheight/", ret_type=RetType.OBJ, status=400)

        # Compare with json block header
        json_obj = self.test_rest_request("/headers/1/{}".format(bb_hash))
        assert_equal(len(json_obj), 1)  # ensure that there is one header in the json response
        assert_equal(json_obj[0]['hash'], bb_hash)  # request/response hash should be the same

        # Compare with normal RPC block response
        rpc_block_json = self.nodes[0].getblock(bb_hash)
        for key in ['hash', 'confirmations', 'height', 'version', 'merkleroot', 'time', 'nonce', 'bits', 'difficulty', 'chainwork', 'previousblockhash']:
            assert_equal(json_obj[0][key], rpc_block_json[key])

        # See if we can get 5 headers in one response
        self.nodes[1].generate(5)
        self.sync_all()
        json_obj = self.test_rest_request("/headers/5/{}".format(bb_hash))
        assert_equal(len(json_obj), 5)  # now we should have 5 header objects

        self.log.info("Test tx inclusion in the /mempool and /block URIs")

        # Make 3 tx and mine them on node 1
        txs = []
        txs.append(self.nodes[0].sendtoaddress(not_related_address, 11))
        txs.append(self.nodes[0].sendtoaddress(not_related_address, 11))
        txs.append(self.nodes[0].sendtoaddress(not_related_address, 11))
        self.sync_all()

        # Check that there are exactly 3 transactions in the TX memory pool before generating the block
        json_obj = self.test_rest_request("/mempool/info")
        assert_equal(json_obj['size'], 3)
        # the size of the memory pool should be greater than 3x ~100 bytes
        assert_greater_than(json_obj['bytes'], 300)

        # Check that there are our submitted transactions in the TX memory pool
        json_obj = self.test_rest_request("/mempool/contents")
        for i, tx in enumerate(txs):
            assert tx in json_obj
            assert_equal(json_obj[tx]['spentby'], txs[i + 1:i + 2])
            assert_equal(json_obj[tx]['depends'], txs[i - 1:i])

        # Now mine the transactions
        newblockhash = self.nodes[1].generate(1)
        self.sync_all()

        # Check if the 3 tx show up in the new block
        json_obj = self.test_rest_request("/block/{}".format(newblockhash[0]))
        non_coinbase_txs = {tx['txid'] for tx in json_obj['tx']
                            if 'coinbase' not in tx['vin'][0]}
        assert_equal(non_coinbase_txs, set(txs))

        # Check the same but without tx details
        json_obj = self.test_rest_request("/block/notxdetails/{}".format(newblockhash[0]))
        for tx in txs:
            assert tx in json_obj['tx']

        self.log.info("Test the /chaininfo URI")

        bb_hash = self.nodes[0].getbestblockhash()

        json_obj = self.test_rest_request("/chaininfo")
        assert_equal(json_obj['bestblockhash'], bb_hash)

if __name__ == '__main__':
    RESTTest().main()
