#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the REST API."""

from decimal import Decimal
from enum import Enum
import http.client
import json
import typing
import urllib.parse


from test_framework.messages import (
    BLOCK_HEADER_SIZE,
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
from test_framework.wallet import (
    MiniWallet,
    getnewdestination,
)
from typing import Optional


INVALID_PARAM = "abc"
UNKNOWN_PARAM = "0000000000000000000000000000000000000000000000000000000000000000"


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

class RESTTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-rest", "-blockfilterindex=1"], []]
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.supports_cli = False

    def test_rest_request(
            self,
            uri: str,
            http_method: str = 'GET',
            req_type: ReqType = ReqType.JSON,
            body: str = '',
            status: int = 200,
            ret_type: RetType = RetType.JSON,
            query_params: Optional[dict[str, typing.Any]] = None,
            ) -> typing.Union[http.client.HTTPResponse, bytes, str, None]:
        rest_uri = '/rest' + uri
        if req_type in ReqType:
            rest_uri += f'.{req_type.name.lower()}'
        if query_params:
            rest_uri += f'?{urllib.parse.urlencode(query_params)}'

        conn = http.client.HTTPConnection(self.url.hostname, self.url.port)
        self.log.debug(f'{http_method} {rest_uri} {body}')
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

        return None

    def run_test(self):
        self.url = urllib.parse.urlparse(self.nodes[0].url)
        self.wallet = MiniWallet(self.nodes[0])

        self.log.info("Broadcast test transaction and sync nodes")
        txid = self.wallet.send_to(from_node=self.nodes[0], scriptPubKey=getnewdestination()[1], amount=int(0.1 * COIN))["txid"]
        self.sync_all()

        self.log.info("Test the /tx URI")

        json_obj = self.test_rest_request(f"/tx/{txid}")
        assert_equal(json_obj['txid'], txid)

        # Check hex format response
        hex_response = self.test_rest_request(f"/tx/{txid}", req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_greater_than_or_equal(int(hex_response.getheader('content-length')),
                                     json_obj['size']*2)

        spent = (json_obj['vin'][0]['txid'], json_obj['vin'][0]['vout'])  # get the vin to later check for utxo (should be spent by then)
        # get n of 0.1 outpoint
        n, = filter_output_indices_by_value(json_obj['vout'], Decimal('0.1'))
        spending = (txid, n)

        # Test /tx with an invalid and an unknown txid
        resp = self.test_rest_request(uri=f"/tx/{INVALID_PARAM}", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"Invalid hash: {INVALID_PARAM}")
        resp = self.test_rest_request(uri=f"/tx/{UNKNOWN_PARAM}", ret_type=RetType.OBJ, status=404)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"{UNKNOWN_PARAM} not found")

        self.log.info("Query an unspent TXO using the /getutxos URI")

        self.generate(self.wallet, 1)
        bb_hash = self.nodes[0].getbestblockhash()

        # Check chainTip response
        json_obj = self.test_rest_request(f"/getutxos/{spending[0]}-{spending[1]}")
        assert_equal(json_obj['chaintipHash'], bb_hash)

        # Make sure there is one utxo
        assert_equal(len(json_obj['utxos']), 1)
        assert_equal(json_obj['utxos'][0]['value'], Decimal('0.1'))

        self.log.info("Query a spent TXO using the /getutxos URI")

        json_obj = self.test_rest_request(f"/getutxos/{spent[0]}-{spent[1]}")

        # Check chainTip response
        assert_equal(json_obj['chaintipHash'], bb_hash)

        # Make sure there is no utxo in the response because this outpoint has been spent
        assert_equal(len(json_obj['utxos']), 0)

        # Check bitmap
        assert_equal(json_obj['bitmap'], "0")

        self.log.info("Query two TXOs using the /getutxos URI")

        json_obj = self.test_rest_request(f"/getutxos/{spending[0]}-{spending[1]}/{spent[0]}-{spent[1]}")

        assert_equal(len(json_obj['utxos']), 1)
        assert_equal(json_obj['bitmap'], "10")

        self.log.info("Query the TXOs using the /getutxos URI with a binary response")

        bin_request = b'\x01\x02'
        for txid, n in [spending, spent]:
            bin_request += bytes.fromhex(txid)
            bin_request += n.to_bytes(4, 'little')

        bin_response = self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.BIN, body=bin_request, ret_type=RetType.BYTES)
        chain_height = int.from_bytes(bin_response[0:4], 'little')
        response_hash = bin_response[4:36][::-1].hex()

        assert_equal(bb_hash, response_hash)  # check if getutxo's chaintip during calculation was fine
        assert_equal(chain_height, 201)  # chain height must be 201 (pre-mined chain [200] + generated block [1])

        self.log.info("Test the /getutxos URI with and without /checkmempool")
        # Create a transaction, check that it's found with /checkmempool, but
        # not found without. Then confirm the transaction and check that it's
        # found with or without /checkmempool.

        # do a tx and don't sync
        txid = self.wallet.send_to(from_node=self.nodes[0], scriptPubKey=getnewdestination()[1], amount=int(0.1 * COIN))["txid"]
        json_obj = self.test_rest_request(f"/tx/{txid}")
        # get the spent output to later check for utxo (should be spent by then)
        spent = (json_obj['vin'][0]['txid'], json_obj['vin'][0]['vout'])
        # get n of 0.1 outpoint
        n, = filter_output_indices_by_value(json_obj['vout'], Decimal('0.1'))
        spending = (txid, n)

        json_obj = self.test_rest_request(f"/getutxos/{spending[0]}-{spending[1]}")
        assert_equal(len(json_obj['utxos']), 0)

        json_obj = self.test_rest_request(f"/getutxos/checkmempool/{spending[0]}-{spending[1]}")
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request(f"/getutxos/{spent[0]}-{spent[1]}")
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request(f"/getutxos/checkmempool/{spent[0]}-{spent[1]}")
        assert_equal(len(json_obj['utxos']), 0)

        self.generate(self.nodes[0], 1)

        json_obj = self.test_rest_request(f"/getutxos/{spending[0]}-{spending[1]}")
        assert_equal(len(json_obj['utxos']), 1)

        json_obj = self.test_rest_request(f"/getutxos/checkmempool/{spending[0]}-{spending[1]}")
        assert_equal(len(json_obj['utxos']), 1)

        self.log.info("Check some invalid requests")
        self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.JSON, body='{"checkmempool', status=400, ret_type=RetType.OBJ)
        self.test_rest_request("/getutxos", http_method='POST', req_type=ReqType.BIN, body='{"checkmempool', status=400, ret_type=RetType.OBJ)
        self.test_rest_request("/getutxos/checkmempool", http_method='POST', req_type=ReqType.JSON, status=400, ret_type=RetType.OBJ)
        self.test_rest_request(f"/getutxos/{spending[0]}_+1", ret_type=RetType.OBJ, status=400)
        self.test_rest_request(f"/getutxos/{spending[0]}-+1", ret_type=RetType.OBJ, status=400)
        self.test_rest_request(f"/getutxos/{spending[0]}--1", ret_type=RetType.OBJ, status=400)
        self.test_rest_request(f"/getutxos/{spending[0]}aa-1234", ret_type=RetType.OBJ, status=400)
        self.test_rest_request("/getutxos/aa-1234", ret_type=RetType.OBJ, status=400)

        # Test limits
        long_uri = '/'.join([f"{txid}-{n_}" for n_ in range(20)])
        self.test_rest_request(f"/getutxos/checkmempool/{long_uri}", http_method='POST', status=400, ret_type=RetType.OBJ)

        long_uri = '/'.join([f'{txid}-{n_}' for n_ in range(15)])
        self.test_rest_request(f"/getutxos/checkmempool/{long_uri}", http_method='POST', status=200)

        self.generate(self.nodes[0], 1)  # generate block to not affect upcoming tests

        self.log.info("Test the /block, /blockhashbyheight, /headers, and /blockfilterheaders URIs")
        bb_hash = self.nodes[0].getbestblockhash()

        # Check result if block does not exists
        assert_equal(self.test_rest_request(f"/headers/{UNKNOWN_PARAM}", query_params={"count": 1}), [])
        self.test_rest_request(f"/block/{UNKNOWN_PARAM}", status=404, ret_type=RetType.OBJ)

        # Check result if block is not in the active chain
        self.nodes[0].invalidateblock(bb_hash)
        assert_equal(self.test_rest_request(f'/headers/{bb_hash}', query_params={'count': 1}), [])
        self.test_rest_request(f'/block/{bb_hash}')
        self.nodes[0].reconsiderblock(bb_hash)

        # Check binary format
        response = self.test_rest_request(f"/block/{bb_hash}", req_type=ReqType.BIN, ret_type=RetType.OBJ)
        assert_greater_than(int(response.getheader('content-length')), BLOCK_HEADER_SIZE)
        response_bytes = response.read()

        # Compare with block header
        response_header = self.test_rest_request(f"/headers/{bb_hash}", req_type=ReqType.BIN, ret_type=RetType.OBJ, query_params={"count": 1})
        assert_equal(int(response_header.getheader('content-length')), BLOCK_HEADER_SIZE)
        response_header_bytes = response_header.read()
        assert_equal(response_bytes[:BLOCK_HEADER_SIZE], response_header_bytes)

        # Check block hex format
        response_hex = self.test_rest_request(f"/block/{bb_hash}", req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_greater_than(int(response_hex.getheader('content-length')), BLOCK_HEADER_SIZE*2)
        response_hex_bytes = response_hex.read().strip(b'\n')
        assert_equal(response_bytes.hex().encode(), response_hex_bytes)

        # Compare with hex block header
        response_header_hex = self.test_rest_request(f"/headers/{bb_hash}", req_type=ReqType.HEX, ret_type=RetType.OBJ, query_params={"count": 1})
        assert_greater_than(int(response_header_hex.getheader('content-length')), BLOCK_HEADER_SIZE*2)
        response_header_hex_bytes = response_header_hex.read(BLOCK_HEADER_SIZE*2)
        assert_equal(response_bytes[:BLOCK_HEADER_SIZE].hex().encode(), response_header_hex_bytes)

        # Check json format
        block_json_obj = self.test_rest_request(f"/block/{bb_hash}")
        assert_equal(block_json_obj['hash'], bb_hash)
        assert_equal(self.test_rest_request(f"/blockhashbyheight/{block_json_obj['height']}")['blockhash'], bb_hash)

        # Check hex/bin format
        resp_hex = self.test_rest_request(f"/blockhashbyheight/{block_json_obj['height']}", req_type=ReqType.HEX, ret_type=RetType.OBJ)
        assert_equal(resp_hex.read().decode('utf-8').rstrip(), bb_hash)
        resp_bytes = self.test_rest_request(f"/blockhashbyheight/{block_json_obj['height']}", req_type=ReqType.BIN, ret_type=RetType.BYTES)
        blockhash = resp_bytes[::-1].hex()
        assert_equal(blockhash, bb_hash)

        # Check invalid blockhashbyheight requests
        resp = self.test_rest_request(f"/blockhashbyheight/{INVALID_PARAM}", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"Invalid height: {INVALID_PARAM}")
        resp = self.test_rest_request("/blockhashbyheight/+1", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Invalid height: +1")
        resp = self.test_rest_request("/blockhashbyheight/1000000", ret_type=RetType.OBJ, status=404)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Block height out of range")
        resp = self.test_rest_request("/blockhashbyheight/-1", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Invalid height: -1")
        self.test_rest_request("/blockhashbyheight/", ret_type=RetType.OBJ, status=400)

        # Compare with json block header
        json_obj = self.test_rest_request(f"/headers/{bb_hash}", query_params={"count": 1})
        assert_equal(len(json_obj), 1)  # ensure that there is one header in the json response
        assert_equal(json_obj[0]['hash'], bb_hash)  # request/response hash should be the same

        # Check invalid uri (% symbol at the end of the request)
        for invalid_uri in [f"/headers/{bb_hash}%", f"/blockfilterheaders/basic/{bb_hash}%", "/mempool/contents.json?%"]:
            resp = self.test_rest_request(invalid_uri, ret_type=RetType.OBJ, status=400)
            assert_equal(resp.read().decode('utf-8').rstrip(), "URI parsing failed, it likely contained RFC 3986 invalid characters")

        # Compare with normal RPC block response
        rpc_block_json = self.nodes[0].getblock(bb_hash)
        for key in ['hash', 'confirmations', 'height', 'version', 'merkleroot', 'time', 'nonce', 'bits', 'target', 'difficulty', 'chainwork', 'previousblockhash']:
            assert_equal(json_obj[0][key], rpc_block_json[key])

        # See if we can get 5 headers in one response
        self.generate(self.nodes[1], 5)
        expected_filter = {
            'basic block filter index': {'synced': True, 'best_block_height': 208},
        }
        self.wait_until(lambda: self.nodes[0].getindexinfo() == expected_filter)
        json_obj = self.test_rest_request(f"/headers/{bb_hash}", query_params={"count": 5})
        assert_equal(len(json_obj), 5)  # now we should have 5 header objects
        json_obj = self.test_rest_request(f"/blockfilterheaders/basic/{bb_hash}", query_params={"count": 5})
        first_filter_header = json_obj[0]
        assert_equal(len(json_obj), 5)  # now we should have 5 filter header objects
        json_obj = self.test_rest_request(f"/blockfilter/basic/{bb_hash}")

        # Compare with normal RPC blockfilter response
        rpc_blockfilter = self.nodes[0].getblockfilter(bb_hash)
        assert_equal(first_filter_header, rpc_blockfilter['header'])
        assert_equal(json_obj['filter'], rpc_blockfilter['filter'])

        # Test blockfilterheaders with an invalid hash and filtertype
        resp = self.test_rest_request(f"/blockfilterheaders/{INVALID_PARAM}/{bb_hash}", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"Unknown filtertype {INVALID_PARAM}")
        resp = self.test_rest_request(f"/blockfilterheaders/basic/{INVALID_PARAM}", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"Invalid hash: {INVALID_PARAM}")

        # Test number parsing
        for num in ['5a', '-5', '0', '2001', '99999999999999999999999999999999999']:
            assert_equal(
                bytes(f'Header count is invalid or out of acceptable range (1-2000): {num}\r\n', 'ascii'),
                self.test_rest_request(f"/headers/{bb_hash}", ret_type=RetType.BYTES, status=400, query_params={"count": num}),
            )

        self.log.info("Test tx inclusion in the /mempool and /block URIs")

        # Make 3 chained txs and mine them on node 1
        txs = []
        input_txid = txid
        for _ in range(3):
            utxo_to_spend = self.wallet.get_utxo(txid=input_txid)
            txs.append(self.wallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_to_spend)['txid'])
            input_txid = txs[-1]
        self.sync_all()

        # Check that there are exactly 3 transactions in the TX memory pool before generating the block
        json_obj = self.test_rest_request("/mempool/info")
        assert_equal(json_obj['size'], 3)
        # the size of the memory pool should be greater than 3x ~100 bytes
        assert_greater_than(json_obj['bytes'], 300)

        mempool_info = self.nodes[0].getmempoolinfo()
        # pop unstable unbroadcastcount before check
        for obj in [json_obj, mempool_info]:
            obj.pop("unbroadcastcount")
        assert_equal(json_obj, mempool_info)

        # Check that there are our submitted transactions in the TX memory pool
        json_obj = self.test_rest_request("/mempool/contents")
        raw_mempool_verbose = self.nodes[0].getrawmempool(verbose=True)

        assert_equal(json_obj, raw_mempool_verbose)

        for i, tx in enumerate(txs):
            assert tx in json_obj
            assert_equal(json_obj[tx]['spentby'], txs[i + 1:i + 2])
            assert_equal(json_obj[tx]['depends'], txs[i - 1:i])

        # Check the mempool response for explicit parameters
        json_obj = self.test_rest_request("/mempool/contents", query_params={"verbose": "true", "mempool_sequence": "false"})
        assert_equal(json_obj, raw_mempool_verbose)

        # Check the mempool response for not verbose
        json_obj = self.test_rest_request("/mempool/contents", query_params={"verbose": "false"})
        raw_mempool = self.nodes[0].getrawmempool(verbose=False)

        assert_equal(json_obj, raw_mempool)

        # Check the mempool response for sequence
        json_obj = self.test_rest_request("/mempool/contents", query_params={"verbose": "false", "mempool_sequence": "true"})
        raw_mempool = self.nodes[0].getrawmempool(verbose=False, mempool_sequence=True)

        assert_equal(json_obj, raw_mempool)

        # Check for error response if verbose=true and mempool_sequence=true
        resp = self.test_rest_request("/mempool/contents", ret_type=RetType.OBJ, status=400, query_params={"verbose": "true", "mempool_sequence": "true"})
        assert_equal(resp.read().decode('utf-8').strip(), 'Verbose results cannot contain mempool sequence values. (hint: set "verbose=false")')

        # Check for error response if verbose is not "true" or "false"
        resp = self.test_rest_request("/mempool/contents", ret_type=RetType.OBJ, status=400, query_params={"verbose": "TRUE"})
        assert_equal(resp.read().decode('utf-8').strip(), 'The "verbose" query parameter must be either "true" or "false".')

        # Check for error response if mempool_sequence is not "true" or "false"
        resp = self.test_rest_request("/mempool/contents", ret_type=RetType.OBJ, status=400, query_params={"verbose": "false", "mempool_sequence": "TRUE"})
        assert_equal(resp.read().decode('utf-8').strip(), 'The "mempool_sequence" query parameter must be either "true" or "false".')

        # Now mine the transactions
        newblockhash = self.generate(self.nodes[1], 1)

        # Check if the 3 tx show up in the new block
        json_obj = self.test_rest_request(f"/block/{newblockhash[0]}")
        non_coinbase_txs = {tx['txid'] for tx in json_obj['tx']
                            if 'coinbase' not in tx['vin'][0]}
        assert_equal(non_coinbase_txs, set(txs))

        # Verify that the non-coinbase tx has "prevout" key set
        for tx_obj in json_obj["tx"]:
            for vin in tx_obj["vin"]:
                if "coinbase" not in vin:
                    assert "prevout" in vin
                    assert_equal(vin["prevout"]["generated"], False)
                else:
                    assert "prevout" not in vin

        # Check the same but without tx details
        json_obj = self.test_rest_request(f"/block/notxdetails/{newblockhash[0]}")
        for tx in txs:
            assert tx in json_obj['tx']

        self.log.info("Test the /chaininfo URI")

        bb_hash = self.nodes[0].getbestblockhash()

        json_obj = self.test_rest_request("/chaininfo")
        assert_equal(json_obj['bestblockhash'], bb_hash)

        # Compare with normal RPC getblockchaininfo response
        blockchain_info = self.nodes[0].getblockchaininfo()
        assert_equal(blockchain_info, json_obj)

        # Test compatibility of deprecated and newer endpoints
        self.log.info("Test compatibility of deprecated and newer endpoints")
        assert_equal(self.test_rest_request(f"/headers/{bb_hash}", query_params={"count": 1}), self.test_rest_request(f"/headers/1/{bb_hash}"))
        assert_equal(self.test_rest_request(f"/blockfilterheaders/basic/{bb_hash}", query_params={"count": 1}), self.test_rest_request(f"/blockfilterheaders/basic/5/{bb_hash}"))

        self.log.info("Test the /deploymentinfo URI")

        deployment_info = self.nodes[0].getdeploymentinfo()
        assert_equal(deployment_info, self.test_rest_request('/deploymentinfo'))

        previous_bb_hash = self.nodes[0].getblockhash(self.nodes[0].getblockcount() - 1)
        deployment_info = self.nodes[0].getdeploymentinfo(previous_bb_hash)
        assert_equal(deployment_info, self.test_rest_request(f"/deploymentinfo/{previous_bb_hash}"))

        non_existing_blockhash = '42759cde25462784395a337460bde75f58e73d3f08bd31fdc3507cbac856a2c4'
        resp = self.test_rest_request(f'/deploymentinfo/{non_existing_blockhash}', ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), "Block not found")

        resp = self.test_rest_request(f"/deploymentinfo/{INVALID_PARAM}", ret_type=RetType.OBJ, status=400)
        assert_equal(resp.read().decode('utf-8').rstrip(), f"Invalid hash: {INVALID_PARAM}")

if __name__ == '__main__':
    RESTTest(__file__).main()
