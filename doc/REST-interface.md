Unauthenticated REST Interface
==============================

The REST API can be enabled with the `-rest` option.

Supported API
-------------
`GET /rest/tx/TX-HASH.{bin|hex|json}`

Given a transaction hash,
Returns a transaction, in binary, hex-encoded binary or JSON formats.

`GET /rest/block/BLOCK-HASH.{bin|hex|json}`
`GET /rest/block/notxdetails/BLOCK-HASH.{bin|hex|json}`

Given a block hash,
Returns a block, in binary, hex-encoded binary or JSON formats.

The HTTP request and response are both handled entirely in-memory, thus making maximum memory usage at least 2.66MB (1 MB max block, plus hex encoding) per request.

With the /notxdetails/ option JSON response will only contain the transaction hash instead of the complete transaction details. The option only affects the JSON response.

For full TX query capability, one must enable the transaction index via "txindex=1" command line / configuration option.

Risks
-------------
Running a webbrowser on the same node with a REST enabled bitcoind can be a risk. Accessing prepared XSS websites could read out tx/block data of your node by placing links like `<script src="http://127.0.0.1:1234/tx/json/1234567890">` which might break the nodes privacy.