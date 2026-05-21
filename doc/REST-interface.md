Unauthenticated REST Interface
==============================

The REST API can be enabled with the `-rest` option.

The interface runs on the same port as the JSON-RPC interface, by default port 8332 for mainnet, port 18332 for testnet,
port 48332 for testnet4, port 38332 for signet, and port 18443 for regtest.

REST Interface consistency guarantees
-------------------------------------

The [same guarantees as for the RPC Interface](/doc/JSON-RPC-interface.md#rpc-consistency-guarantees)
apply.

Limitations
-----------

There is a known issue in the REST interface that can cause a node to crash if
too many http connections are being opened at the same time because the system runs
out of available file descriptors. To prevent this from happening you might
want to increase the number of maximum allowed file descriptors in your system
and try to prevent opening too many connections to your rest interface at the
same time if this is under your control. It is hard to give general advice
since this depends on your system but if you make several hundred requests at
once you are definitely at risk of encountering this issue.

Supported API
-------------

#### Transactions
`GET /rest/tx/<TX-HASH>.<bin|hex|json>`

Given a transaction hash: returns a transaction in binary, hex-encoded binary, or JSON formats.
Responds with 404 if the transaction doesn't exist.

By default, this endpoint will only search the mempool.
To query for a confirmed transaction, enable the transaction index via "txindex=1" command line / configuration option.

#### Blocks
- `GET /rest/block/<BLOCK-HASH>.<bin|hex|json>`
- `GET /rest/block/notxdetails/<BLOCK-HASH>.<bin|hex|json>`

Given a block hash: returns a block, in binary, hex-encoded binary or JSON formats.
Responds with 404 if the block doesn't exist.

The HTTP request and response are both handled entirely in-memory.

With the /notxdetails/ option JSON response will only contain the transaction hash instead of the complete transaction details. The option only affects the JSON response.

- `GET /rest/blockpart/<BLOCK-HASH>.<bin|hex>?offset=<OFFSET>&size=<SIZE>`

Given a block hash: returns a block part, in binary or hex-encoded binary formats.
Responds with 404 if the block or the byte range doesn't exist.

#### Blockheaders
`GET /rest/headers/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`

Given a block hash: returns <COUNT> amount of blockheaders in upward direction.
Returns empty if the block doesn't exist or it isn't in the active chain.

*Deprecated (but not removed) since 24.0:*
`GET /rest/headers/<COUNT>/<BLOCK-HASH>.<bin|hex|json>`

#### Blockfilter Headers
`GET /rest/blockfilterheaders/<FILTERTYPE>/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`

Given a block hash: returns <COUNT> amount of blockfilter headers in upward
direction for the filter type <FILTERTYPE>.
Returns empty if the block doesn't exist or it isn't in the active chain.

*Deprecated (but not removed) since 24.0:*
`GET /rest/blockfilterheaders/<FILTERTYPE>/<COUNT>/<BLOCK-HASH>.<bin|hex|json>`

#### Blockfilters
`GET /rest/blockfilter/<FILTERTYPE>/<BLOCK-HASH>.<bin|hex|json>`

Given a block hash: returns the block filter of the given block of type
<FILTERTYPE>.
Responds with 404 if the block doesn't exist.

#### Blockhash by height
`GET /rest/blockhashbyheight/<HEIGHT>.<bin|hex|json>`

Given a height: returns hash of block in best-block-chain at height provided.
Responds with 404 if block not found.

#### Spent transaction outputs
`GET /rest/spenttxouts/<BLOCK-HASH>.<bin|hex|json>`

Given a block hash: returns a collection of spent transaction output lists,
one per transaction in the block.
Responds with 404 if the block doesn't exist or its undo data is not available.

#### Chaininfos
`GET /rest/chaininfo.json`

Returns various state info regarding block chain processing.
Only supports JSON as output format.
Refer to the `getblockchaininfo` RPC help for details.

#### Deployment info
`GET /rest/deploymentinfo.json`
`GET /rest/deploymentinfo/<BLOCKHASH>.json`

Returns an object containing various state info regarding deployments of
consensus changes at the current chain tip, or at <BLOCKHASH> if provided.
Only supports JSON as output format.
Refer to the `getdeploymentinfo` RPC help for details.

#### Query UTXO set
- `GET /rest/getutxos/<TXID>-<N>/<TXID>-<N>/.../<TXID>-<N>.<bin|hex|json>`
- `GET /rest/getutxos/checkmempool/<TXID>-<N>/<TXID>-<N>/.../<TXID>-<N>.<bin|hex|json>`

The getutxos endpoint allows querying the UTXO set, given a set of outpoints.
With the `/checkmempool/` option, the mempool is also taken into account.
See [BIP64](https://github.com/bitcoin/bips/blob/master/bip-0064.mediawiki) for
input and output serialization (relevant for `bin` and `hex` output formats).

Example:
```
$ curl localhost:18332/rest/getutxos/checkmempool/b2cdfd7b89def827ff8af7cd9bff7627ff72e5e8b0f71210f92ea7a4000c5d75-0.json 2>/dev/null | json_pp
{
   "chainHeight" : 325347,
   "chaintipHash" : "00000000fb01a7f3745a717f8caebee056c484e6e0bfe4a9591c235bb70506fb",
   "bitmap": "1",
   "utxos" : [
      {
         "height" : 2147483647,
         "value" : 8.8687,
         "scriptPubKey" : {
            "asm" : "OP_DUP OP_HASH160 1c7cebb529b86a04c683dfa87be49de35bcf589e OP_EQUALVERIFY OP_CHECKSIG",
            "desc" : "addr(mi7as51dvLJsizWnTMurtRmrP8hG2m1XvD)#gj9tznmy",
            "hex" : "76a9141c7cebb529b86a04c683dfa87be49de35bcf589e88ac",
            "type" : "pubkeyhash",
            "address" : "mi7as51dvLJsizWnTMurtRmrP8hG2m1XvD"
         }
      }
   ]
}
```

#### Memory pool
`GET /rest/mempool/info.json`

Returns various information about the transaction mempool.
Only supports JSON as output format.
Refer to the `getmempoolinfo` RPC help for details.

`GET /rest/mempool/contents.json?verbose=<true|false>&mempool_sequence=<false|true>`

Returns the transactions in the mempool.
Only supports JSON as output format.
Refer to the `getrawmempool` RPC help for details. Defaults to setting
`verbose=true` and `mempool_sequence=false`.

*Query parameters for `verbose` and `mempool_sequence` available in 25.0 and up.*


Risks
-------------
Running a web browser on the same node with a REST enabled bitcoind can be a risk. Accessing prepared XSS websites could read out tx/block data of your node by placing links like `<script src="http://127.0.0.1:8332/rest/tx/1234567890.json">` which might break the nodes privacy.
