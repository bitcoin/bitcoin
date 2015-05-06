Unauthenticated REST Interface
==============================

The REST API can be enabled with the `-rest` option.

Supported API
-------------

####Transactions
`GET /rest/tx/TX-HASH.{bin|hex|json}`

Given a transaction hash,
Returns a transaction, in binary, hex-encoded binary or JSON formats.

For full TX query capability, one must enable the transaction index via "txindex=1" command line / configuration option.

####Blocks
`GET /rest/block/BLOCK-HASH.{bin|hex|json}`
`GET /rest/block/notxdetails/BLOCK-HASH.{bin|hex|json}`

Given a block hash,
Returns a block, in binary, hex-encoded binary or JSON formats.

The HTTP request and response are both handled entirely in-memory, thus making maximum memory usage at least 2.66MB (1 MB max block, plus hex encoding) per request.

With the /notxdetails/ option JSON response will only contain the transaction hash instead of the complete transaction details. The option only affects the JSON response.

####Blockheaders
`GET /rest/headers/<COUNT>/<BLOCK-HASH>.<bin|hex>`

Given a block hash,
Returns <COUNT> amount of blockheaders in upward direction.

JSON is not supported.

####Chaininfos
`GET /rest/chaininfo.json`

Returns various state info regarding block chain processing.
Only supports JSON as output format.
* chain : (string) current network name as defined in BIP70 (main, test, regtest)
* blocks : (numeric) the current number of blocks processed in the server
* headers : (numeric) the current number of headers we have validated
* bestblockhash : (string) the hash of the currently best block
* difficulty : (numeric) the current difficulty
* verificationprogress : (numeric) estimate of verification progress [0..1]
* chainwork : (string) total amount of work in active chain, in hexadecimal

`GET /rest/getutxos`

The getutxo command allows querying of the UTXO set given a set of of outpoints.
See BIP64 for input and output serialisation:
https://github.com/bitcoin/bips/blob/master/bip-0064.mediawiki


Risks
-------------
Running a webbrowser on the same node with a REST enabled bitcoind can be a risk. Accessing prepared XSS websites could read out tx/block data of your node by placing links like `<script src="http://127.0.0.1:8332/rest/tx/1234567890.json">` which might break the nodes privacy.
