{% comment %}
This file is licensed under the MIT License (MIT) available on
http://opensource.org/licenses/MIT.
{% endcomment %}
{% assign filename="_data/devdocs/en/bitcoin-core/rest/requests/get_chaininfo.md" %}

##### GET ChainInfo
{% include helpers/subhead-links.md %}

{% assign summary_restGetChainInfo="returns information about the current state of the block chain." %}

{% autocrossref %}

The `GET chaininfo` operation {{summary_restGetChainInfo}}  Supports only `json` as output format.

*Request*

{% highlight text %}
GET /chaininfo.json
{% endhighlight %}

*Parameters: none*

*Response as JSON*

{% itemplate ntpd1 %}
- n: "`result`"
  t: "object"
  p: "Required<br>(exactly 1)"
  d: "Information about the current state of the local block chain"

- n: "→<br>`chain`"
  t: "string"
  p: "Required<br>(exactly 1)"
  d: "The name of the block chain.  One of `main` for mainnet, `test` for testnet, or `regtest`<!--noref--> for regtest"

- n: "→<br>`blocks`"
  t: "number (int)"
  p: "Required<br>(exactly 1)"
  d: "The number of validated blocks in the local best block chain.  For a new node with just the hardcoded genesis block, this will be 0"

- n: "→<br>`headers`"
  t: "number (int)"
  p: "Required<br>(exactly 1)"
  d: "The number of validated headers in the local best headers chain.  For a new node with just the hardcoded genesis block, this will be zero.  This number may be higher than the number of *blocks*"

- n: "→<br>`bestblockhash`"
  t: "string (hex)"
  p: "Required<br>(exactly 1)"
  d: "The hash of the header of the highest validated block in the best block chain, encoded as hex in RPC byte order.  This is identical to the string returned by the `getbestblockhash` RPC"

- n: "→<br>`difficulty`"
  t: "number (real)"
  p: "Required<br>(exactly 1)"
  d: "The difficulty of the highest-height block in the best block chain"

- n: "→<br>`mediantime`"
  t: "number (int)"
  p: "Required<br>(exactly 1)"
  d: "*Added in Bitcoin Core 0.12.0*<br><br>The median time of the 11 blocks before the most recent block on the blockchain.  Used for validating transaction locktime under BIP113"
  
- n: "→<br>`verificationprogress`"
  t: "number (real)"
  p: "Required (exactly 1)"
  d: "Estimate of what percentage of the block chain transactions have been verified so far, starting at 0.0 and increasing to 1.0 for fully verified.  May slightly exceed 1.0 when fully synced to account for transactions in the memory pool which have been verified before being included in a block"

- n: "→<br>`chainwork`"
  t: "string (hex)"
  p: "Required<br>(exactly 1)"
  d: "The estimated number of block header hashes checked from the genesis block to this block, encoded as big-endian hex"

- n: "→<br>`pruned`"
  t: "bool"
  p: "Required<br>(exactly 1)"
  d: "Indicates if the blocks are subject to pruning"
  
- n: "→<br>`pruneheight`"
  t: "number (int)"
  p: "Optional<br>(0 or 1)"
  d: "The lowest-height complete block stored if prunning is activated"
  
- n: "→<br>`softforks`"
  t: "array"
  p: "Required<br>(exactly 1)"
  d: "*Added in Bitcoin Core 0.12.0*<br><br>An array of objects each describing a current or previous soft fork"

- n: "→ →<br>Softfork"
  t: "object"
  p: "Required<br>(3 or more)"
  d: "A specific softfork"
  
- n: "→ → →<br>`id`"
  t: "string"
  p: "Required<br>(exactly 1)"
  d: "The name of the softfork"
  
- n: "→ → →<br>`version`"
  t: "numeric<br>(int)"
  p: "Required<br>(exactly 1)"
  d: "The block version used for the softfork"
  
- n: "→ → →<br>`enforce`"
  t: "string : object"
  p: "Optional<br>(0 or 1)"
  d: "The progress toward enforcing the softfork rules for new-version blocks"

- n: "→ → → →<br>`status`"
  t: "bool"
  p: "Required<br>(exactly 1)"
  d: "Indicates if the threshold was reached"
  
- n: "→ → → →<br>`found`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "Number of blocks that support the softfork"
  
- n: "→ → → →<br>`required`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "Number of blocks that are required to reach the threshold"

- n: "→ → → →<br>`window`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "The maximum size of examined window of recent blocks"

- n: "→ → →<br>`reject`"
  t: "object"
  p: "Optional<br>(0 or 1)"
  d: "The progress toward enforcing the softfork rules for new-version blocks"

- n: "→ → → →<br>`status`"
  t: "bool"
  p: "Optional<br>(0 or 1)"
  d: "Indicates if the threshold was reached"
  
- n: "→ → → →<br>`found`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "Number of blocks that support the softfork"
  
- n: "→ → → →<br>`required`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "Number of blocks that are required to reach the threshold"

- n: "→ → → →<br>`window`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "The maximum size of examined window of recent blocks"
  
- n: "→<br>`bip9_softforks`"
  t: "object"
  p: "Required<br>(exactly 1)"
  d: "*Added in Bitcoin Core 0.12.1*<br><br>The status of BIP9 softforks in progress"
  
- n: "→ →<br>Name"
  t: "string : object"
  p: "Required<br>(2 or more)"
  d: "A specific BIP9 softfork"
  
- n: "→ → →<br>`status`"
  t: "string"
  p: "Required<br>(exactly 1)"
  d: "Set to one of the following reasons:<br>• `defined` if voting hasn't started yet<br>• `started` if the voting has started <br>• `locked_in` if the voting was successful but the softfort hasn't been activated yet<br>• `active` if the softfork was activated<br>• `failed` if the softfork has not receieved enough votes"
  
- n: "→ → →<br>`bit`"
  t: "numeric<br>(int)"
  p: "Optional<br>(0 or 1)"
  d: "The bit (0-28) in the block version field used to signal this softfork.  Field is only shown when status is `started`"
  
- n: "→ → →<br>`startTime`"
  t: "numeric<br>(int)"
  p: "Required<br>(exactly 1)"
  d: "The Unix epoch time when the softfork voting begins"
  
- n: "→ → →<br>`timeout`"
  t: "numeric<br>(int)"
  p: "Required<br>(exactly 1)"
  d: "The Unix epoch time at which the deployment is considered failed if not yet locked in"
  
{% enditemplate %}

*Examples from Bitcoin Core 0.13.1*

Get blockchain info in JSON:

{% highlight bash %}
curl http://localhost:8332/rest/chaininfo.json
{% endhighlight %}

Result (whitespaced added):

{% highlight json %}
{
  "chain": "main",
  "blocks": 443372,
  "headers": 443372,
  "bestblockhash": "0000000000000000029a7ee8eb90c47cfcb3e3d877428ed85a3251719bf65ad7",
  "difficulty": 286765766820.5504,
  "mediantime": 1481671547,
  "verificationprogress": 0.9999951668985226,
  "chainwork": "000000000000000000000000000000000000000000330d0672c7d7f4f705b65c",
  "pruned": false,
  "softforks": [
    {
      "id": "bip34",
      "version": 2,
      "reject": {
        "status": true
      }
    },
    {
      "id": "bip66",
      "version": 3,
      "reject": {
        "status": true
      }
    },
    {
      "id": "bip65",
      "version": 4,
      "reject": {
        "status": true
      }
    }
  ],
  "bip9_softforks": {
    "csv": {
      "status": "active",
      "startTime": 1462060800,
      "timeout": 1493596800,
      "since": 419328
    },
    "segwit": {
      "status": "started",
      "bit": 1,
      "startTime": 1479168000,
      "timeout": 1510704000,
      "since": 439488
    }
  }
}
{% endhighlight %}

*See also*

* [GetBlockChainInfo][rpc getblockchaininfo] RPC: {{summary_getBlockChainInfo}}

{% endautocrossref %}
