JSON-RPC API
============

Omni Core is a fork of Bitcoin Core, with Omni Protocol feature support added as a new layer of functionality on top. As such interacting with the API is done in the same manner (JSON-RPC) as Bitcoin Core, simply with additional RPCs available for utilizing Omni Protocol features.

As all existing Bitcoin Core functionality is inherent to Omni Core, the RPC port by default remains as `8332` as per Bitcoin Core.  If you wish to run Omni Core in tandem with Bitcoin Core (eg. via a separate datadir) you may utilize the `-rpcport<port>` option to nominate an alternative port number.

All available commands can be listed with `"help"`, and information about a specific command can be retrieved with `"help <command>"`.

*Please note: this document may not always be up-to-date. There may be errors, omissions or inaccuracies present.*


## Transaction creation

The RPCs for transaction creation can be used to create and broadcast Omni Protocol transactions.

A hash of the broadcasted transaction is returned as result.

### omni_send

Create and broadcast a simple send transaction.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress`         | string  | required | the address of the receiver                                                                  |
| `propertyid`        | number  | required | the identifier of the tokens to send                                                         |
| `amount`            | string  | required | the amount to send                                                                           |
| `redeemaddress`     | string  | optional | an address that can spend the transaction dust (sender by default)                           |
| `referenceamount`   | string  | optional | a bitcoin amount that is sent to the receiver (minimal by default)                           |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_send" "3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "100.0"
```

---

### omni_senddexsell

Place, update or cancel a sell offer on the traditional distributed OMNI/BTC exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `propertyidforsale` | number  | required | the identifier of the tokens to list for sale (must be `1` for `OMNI` or `2`for `TOMNI`)     |
| `amountforsale`     | string  | required | the amount of tokens to list for sale                                                        |
| `amountdesired`     | string  | required | the amount of bitcoins desired                                                               |
| `paymentwindow`     | number  | required | a time limit in blocks a buyer has to pay following a successful accepting order             |
| `minacceptfee`      | string  | required | a minimum mining fee a buyer has to pay to accept the offer                                  |
| `action`            | number  | required | the action to take (`1` for new offers, `2` to update, `3` to cancel)                        |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_senddexsell" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "1.5" "0.75" 25 "0.0005" 1
```

---

### omni_senddexaccept

Create and broadcast an accept offer for the specified token and amount.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress`         | string  | required | the address of the seller                                                                    |
| `propertyid`        | number  | required | the identifier of the token to purchase                                                      |
| `amount`            | string  | required | the amount to accept                                                                         |
| `override`          | boolean | required | override minimum accept fee and payment window checks (use with caution!)                    |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_senddexaccept" \
    "35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "15.0"
```

---

### omni_sendissuancecrowdsale

Create new tokens as crowdsale.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `ecosystem`         | number  | required | the ecosystem to create the tokens in (`1` for main ecosystem, `2` for test ecosystem)       |
| `type`              | number  | required | the type of the tokens to create: (`1` for indivisible tokens, `2` for divisible tokens)     |
| `previousid`        | number  | optional | an identifier of a predecessor token (`0` for new crowdsales)                                |
| `category`          | string  | optional | a category for the new tokens (can be `""`)                                                  |
| `subcategory`       | string  | optional | a subcategory for the new tokens (can be `""`)                                               |
| `name`              | string  | required | the name of the new tokens to create                                                         |
| `url`               | string  | optional | an URL for further information about the new tokens (can be `""`)                            |
| `data`              | string  | optional | a description for the new tokens (can be `""`)                                               |
| `propertyiddesired` | number  | required | the identifier of a token eligible to participate in the crowdsale                           |
| `tokensperunit`     | string  | required | the amount of tokens granted per unit invested in the crowdsale                              |
| `deadline`          | number  | required | the deadline of the crowdsale as Unix timestamp                                              |
| `earlybonus`        | number  | required | an early bird bonus for participants in percent per week                                     |
| `issuerpercentage`  | number  | required | a percentage of tokens that will be granted to the issuer                                    |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendissuancecrowdsale" \
    "3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo" 2 1 0 "Companies" "Bitcoin Mining" \
    "Quantum Miner" "" "" 2 "100" 1483228800 30 2
```

---

### omni_sendissuancefixed

Create new tokens with fixed supply.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `ecosystem`         | number  | required | the ecosystem to create the tokens in (`1` for main ecosystem, `2` for test ecosystem)       |
| `type`              | number  | required | the type of the tokens to create: (`1` for indivisible tokens, `2` for divisible tokens)     |
| `previousid`        | number  | optional | an identifier of a predecessor token (`0` for new tokens)                                    |
| `category`          | string  | optional | a category for the new tokens (can be `""`)                                                  |
| `subcategory`       | string  | optional | a subcategory for the new tokens (can be `""`)                                               |
| `name`              | string  | required | the name of the new tokens to create                                                         |
| `url`               | string  | optional | an URL for further information about the new tokens (can be `""`)                            |
| `data`              | string  | optional | a description for the new tokens (can be `""`)                                               |
| `amount`            | string  | required | the number of tokens to create                                                               |
| `tokensperunit`     | string  | required | the amount of tokens granted per unit invested in the crowdsale                              |
| `deadline`          | number  | required | the deadline of the crowdsale as Unix timestamp                                              |
| `earlybonus`        | number  | required | an early bird bonus for participants in percent per week                                     |
| `issuerpercentage`  | number  | required | a percentage of tokens that will be granted to the issuer                                    |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendissuancefixed" \
    "3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3" 2 1 0 "Companies" "Bitcoin Mining" \
    "Quantum Miner" "" "" "1000000"
```

---

### omni_sendissuancemanaged

Create new tokens with manageable supply.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `ecosystem`         | number  | required | the ecosystem to create the tokens in (`1` for main ecosystem, `2` for test ecosystem)       |
| `type`              | number  | required | the type of the tokens to create: (`1` for indivisible tokens, `2` for divisible tokens)     |
| `previousid`        | number  | optional | an identifier of a predecessor token (`0` for new tokens)                                    |
| `category`          | string  | optional | a category for the new tokens (can be `""`)                                                  |
| `subcategory`       | string  | optional | a subcategory for the new tokens (can be `""`)                                               |
| `name`              | string  | required | the name of the new tokens to create                                                         |
| `url`               | string  | optional | an URL for further information about the new tokens (can be `""`)                            |
| `data`              | string  | optional | a description for the new tokens (can be `""`)                                               |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendissuancemanaged" \
    "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" 2 1 0 "Companies" "Bitcoin Mining" "Quantum Miner" "" ""
```

---

### omni_sendsto

Create and broadcast a send-to-owners transaction.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `propertyid`        | number  | required | the identifier of the tokens to distribute                                                   |
| `amount`            | string  | required | the amount to distribute                                                                     |
| `redeemaddress`     | string  | optional | an address that can spend the transaction dust (sender by default)                           |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendsto" \
    "32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 3 "5000"
```

---

### omni_sendgrant

Issue or grant new units of managed tokens.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress`         | string  | optional | the receiver of the tokens (sender by default)                                               |
| `propertyid`        | number  | required | the identifier of the tokens to grant                                                        |
| `amount`            | string  | required | the amount of tokens to create                                                               |
| `memo`              | string  | optional | a text note attached to this transaction (none by default)                                   |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendgrant" "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" "" 51 "7000"
```

---

### omni_sendrevoke

Revoke units of managed tokens.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `propertyid`        | number  | required | the identifier of the tokens to revoke                                                       |
| `amount`            | string  | required | the amount of tokens to revoke                                                               |
| `memo`              | string  | optional | a text note attached to this transaction (none by default)                                   |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendrevoke" "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" "" 51 "100"
```

---

### omni_sendclosecrowdsale

Manually close a crowdsale.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address associated with the crowdsale to close                                           |
| `propertyid`        | number  | required | the identifier of the crowdsale to close                                                     |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendclosecrowdsale" "3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo" 70
```

---

### omni_sendtrade

Place a trade offer on the distributed token exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to trade with                                                                    |
| `propertyidforsale` | number  | required | the identifier of the tokens to list for sale                                                |
| `amountforsale`     | string  | required | the amount of tokens to list for sale                                                        |
| `propertiddesired`  | number  | required | the identifier of the tokens desired in exchange                                             |
| `amountdesired`     | string  | required | the amount of tokens desired in exchange                                                     |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendtrade" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 31 "250.0" 1 "10.0"
```

---

### omni_sendcanceltradesbyprice

Cancel offers on the distributed token exchange with the specified price.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to trade with                                                                    |
| `propertyidforsale` | number  | required | the identifier of the tokens listed for sale                                                 |
| `amountforsale`     | string  | required | the amount of tokens to listed for sale                                                      |
| `propertiddesired`  | number  | required | the identifier of the tokens desired in exchange                                             |
| `amountdesired`     | string  | required | the amount of tokens desired in exchange                                                     |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendcanceltradesbyprice" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 31 "100.0" 1 "5.0"
```

---

### omni_sendcanceltradesbypair

Cancel all offers on the distributed token exchange with the given currency pair.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to trade with                                                                    |
| `propertyidforsale` | number  | required | the identifier of the tokens listed for sale                                                 |
| `propertiddesired`  | number  | required | the identifier of the tokens desired in exchange                                             |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendcanceltradesbypair" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 1 31
```

---

### omni_sendcancelalltrades

Cancel all offers on the distributed token exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to trade with                                                                    |
| `ecosystem`         | number  | required | the ecosystem of the offers to cancel (`1` for main ecosystem, `2` for test ecosystem)       |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendcancelalltrades" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 1
```

---

### omni_sendchangeissuer

Change the issuer on record of the given tokens.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address associated with the tokens                                                       |
| `toaddress  `       | string  | required | the address to transfer administrative control to                                            |
| `propertyid`        | number  | required | the identifier of the tokens                                                                 |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendchangeissuer" \
    "1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu" "3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs" 3
```

---

### omni_sendall

Transfers all available tokens in the given ecosystem to the recipient.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `toaddress  `       | string  | required | the address of the receiver                                                                  |
| `ecosystem`         | number  | required | the ecosystem of the tokens to send (`1` for main ecosystem, `2` for test ecosystem)         |
| `redeemaddress`     | string  | optional | an address that can spend the transaction dust (sender by default)                           |
| `referenceamount`   | string  | optional | a bitcoin amount that is sent to the receiver (minimal by default)                           |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendall" "3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 2
```

---

### omni_sendrawtx

Broadcasts a raw Omni Layer transaction.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `fromaddress`       | string  | required | the address to send from                                                                     |
| `rawtransaction`    | string  | required | the hex-encoded raw transaction                                                              |
| `referenceaddress`  | string  | optional | a reference address (none by default)                                                        |
| `redeemaddress`     | string  | optional | an address that can spend the transaction dust (sender by default)                           |
| `referenceamount`   | string  | optional | a bitcoin amount that is sent to the receiver (minimal by default)                           |

**Result:**
```js
"hash"  // (string) the hex-encoded transaction hash
```

**Example:**

```bash
$ omnicore-cli "omni_sendrawtx" \
    "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8" "000000000000000100000000017d7840" \
    "1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV"
```

---


## Data retrieval

The RPCs for data retrieval can be used to get information about the state of the Omni ecosystem.

### omni_getinfo

Returns various state information of the client and protocol.

**Arguments:**

*None*

**Result:**
```js
Result:
{
  "mastercoreversion" : "x.x.x.x-xxx",  // (string) client version
  "bitcoincoreversion" : "x.x.x",       // (string) Bitcoin Core version
  "commitinfo" : "xxxxxxx",             // (string) build commit identifier
  "block" : nnnnnn,                     // (number) index of the last processed block
  "blocktime" : nnnnnnnnnn,             // (number) timestamp of the last processed block
  "blocktransactions" : nnnn,           // (number) Omni transactions found in the last processed block
  "totaltransactions" : nnnnnnnn,       // (number) Omni transactions processed in total
  "alerts" : [                          // (array of JSON objects) active protocol alert (if any)
    {
      "alerttype" : n                       // (number) alert type as integer
      "alerttype" : "xxx"                   // (string) alert type (can be "alertexpiringbyblock", "alertexpiringbyblocktime", "alertexpiringbyclientversion" or "error")
      "alertexpiry" : nnnnnnnnnn            // (number) expiration criteria (can refer to block height, timestamp or client verion)
      "alertmessage" : "xxx"                // (string) information about the alert
    },
    ...
  ]
}
```

**Example:**

```bash
$ omnicore-cli "omni_getinfo"
```

---

### omni_getbalance

Returns the token balance for a given address and property.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `address`           | string  | required | the address                                                                                  |
| `propertyid`        | number  | required | the property identifier                                                                      |

**Result:**
```js
{
  "balance" : "n.nnnnnnnn",  // (string) the available balance of the address
  "reserved" : "n.nnnnnnnn"  // (string) the amount reserved by sell offers and accepts
}
```

**Example:**

```bash
$ omnicore-cli "omni_getbalance", "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P" 1
```

---

### omni_getallbalancesforid

Returns a list of token balances for a given currency or property identifier.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | the property identifier                                                                      |

**Result:**
```js
[                          // (array of JSON objects)
  {
    "address" : "address",     // (string) the address
    "balance" : "n.nnnnnnnn",  // (string) the available balance of the address
    "reserved" : "n.nnnnnnnn"  // (string) the amount reserved by sell offers and accepts
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_getallbalancesforid" 1
```

---

### omni_getallbalancesforaddress

Returns a list of all token balances for a given address.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `address`           | string  | required | the address                                                                                  |

**Result:**
```js
[                          // (array of JSON objects)
  {
    "propertyid" : n,          // (number) the property identifier
    "balance" : "n.nnnnnnnn",  // (string) the available balance of the address
    "reserved" : "n.nnnnnnnn"  // (string) the amount reserved by sell offers and accepts
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_getallbalancesforaddress" "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"
```

---

### omni_gettransaction

Get detailed information about an Omni transaction.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `txid`              | string  | required | the hash of the transaction to lookup                                                        |

**Result:**
```js
{
  "txid" : "hash",                 // (string) the hex-encoded hash of the transaction
  "sendingaddress" : "address",    // (string) the Bitcoin address of the sender
  "referenceaddress" : "address",  // (string) a Bitcoin address used as reference (if any)
  "ismine" : true|false,           // (boolean) whether the transaction involes an address in the wallet
  "confirmations" : nnnnnnnnnn,    // (number) the number of transaction confirmations
  "fee" : "n.nnnnnnnn",            // (string) the transaction fee in bitcoins
  "blocktime" : nnnnnnnnnn,        // (number) the timestamp of the block that contains the transaction
  "valid" : true|false,            // (boolean) whether the transaction is valid
  "version" : n,                   // (number) the transaction version
  "type_int" : n,                  // (number) the transaction type as number
  "type" : "type",                 // (string) the transaction type as string
  [...]                            // (mixed) other transaction type specific properties
}
```

**Example:**

```bash
$ omnicore-cli "omni_gettransaction" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"
```

---

### omni_listtransactions

List wallet transactions, optionally filtered by an address and block boundaries.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `txid`              | string  | optional | address filter (default: `"*"`)                                                             |
| `count`             | number  | optional | show at most n transactions (default: `10`)                                                  |
| `skip`              | number  | optional | skip the first n transactions (default: `0`)                                                 |
| `startblock`        | number  | optional | first block to begin the search (default: `0`)                                               |
| `endblock`          | number  | optional | last block to include in the search (default: `999999`)                                      |

**Result:**
```js
[                                // (array of JSON objects)
  {
    "txid" : "hash",                 // (string) the hex-encoded hash of the transaction
    "sendingaddress" : "address",    // (string) the Bitcoin address of the sender
    "referenceaddress" : "address",  // (string) a Bitcoin address used as reference (if any)
    "ismine" : true|false,           // (boolean) whether the transaction involves an address in the wallet
    "confirmations" : nnnnnnnnnn,    // (number) the number of transaction confirmations
    "fee" : "n.nnnnnnnn",            // (string) the transaction fee in bitcoins
    "blocktime" : nnnnnnnnnn,        // (number) the timestamp of the block that contains the transaction
    "valid" : true|false,            // (boolean) whether the transaction is valid
    "version" : n,                   // (number) the transaction version
    "type_int" : n,                  // (number) the transaction type as number
    "type" : "type",                 // (string) the transaction type as string
    [...]                            // (mixed) other transaction type specific properties
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_listtransactions"
```

---

### omni_listblocktransactions

Lists all Omni transactions in a block.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `index`             | number  | required | the block height or block index                                                              |

**Result:**
```js
[      // (array of string)
  "hash",  // (string) the hash of the transaction
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_listblocktransactions" 279007
```

---

### omni_listpendingtransactions

Returns a list of unconfirmed Omni transactions, pending in the memory pool.

Note: the validity of pending transactions is uncertain, and the state of the memory pool may change at any moment. It is recommended to check transactions after confirmation, and pending transactions should be considered as invalid.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `address`           | string  | optional | filter results by address (default: `""` for no filter)                                      |

**Result:**
```js
[                                // (array of JSON objects)
  {
    "txid" : "hash",                 // (string) the hex-encoded hash of the transaction
    "sendingaddress" : "address",    // (string) the Bitcoin address of the sender
    "referenceaddress" : "address",  // (string) a Bitcoin address used as reference (if any)
    "ismine" : true|false,           // (boolean) whether the transaction involes an address in the wallet
    "fee" : "n.nnnnnnnn",            // (string) the transaction fee in bitcoins
    "version" : n,                   // (number) the transaction version
    "type_int" : n,                  // (number) the transaction type as number
    "type" : "type",                 // (string) the transaction type as string
    [...]                            // (mixed) other transaction type specific properties
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_listpendingtransactions"
```

---

### omni_getactivedexsells

Returns currently active offers on the distributed exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `address`           | string  | optional | address filter (default: include any)                                                        |

**Result:**
```js
[                                  // (array of JSON objects)
  {
    "txid" : "hash",                   // (string) the hash of the transaction of this offer
    "propertyid" : n,                  // (number) the identifier of the tokens for sale
    "seller" : "address",              // (string) the Bitcoin address of the seller
    "amountavailable" : "n.nnnnnnnn",  // (string) the number of tokens still listed for sale and currently available
    "bitcoindesired" : "n.nnnnnnnn",   // (string) the number of bitcoins desired in exchange
    "unitprice" : "n.nnnnnnnn" ,       // (string) the unit price (BTC/token)
    "timelimit" : nn,                  // (number) the time limit in blocks a buyer has to pay following a successful accept
    "minimumfee" : "n.nnnnnnnn",       // (string) the minimum mining fee a buyer has to pay to accept this offer
    "amountaccepted" : "n.nnnnnnnn",   // (string) the number of tokens currently reserved for pending "accept" orders
    "accepts": [                       // (array of JSON objects) a list of pending "accept" orders
      {
        "buyer" : "address",               // (string) the Bitcoin address of the buyer
        "block" : nnnnnn,                  // (number) the index of the block that contains the "accept" order
        "blocksleft" : nn,                 // (number) the number of blocks left to pay
        "amount" : "n.nnnnnnnn"            // (string) the amount of tokens accepted and reserved
        "amounttopay" : "n.nnnnnnnn"       // (string) the amount in bitcoins needed finalize the trade
      },
      ...
    ]
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_getactivedexsells"
```

---

### omni_listproperties

Lists all tokens or smart properties.

**Arguments:**

*None*

**Result:**
```js
[                               // (array of JSON objects)
  {
    "propertyid" : n,               // (number) the identifier of the tokens
    "name" : "name",                // (string) the name of the tokens
    "category" : "category",        // (string) the category used for the tokens
    "subcategory" : "subcategory",  // (string) the subcategory used for the tokens
    "data" : "information",         // (string) additional information or a description
    "url" : "uri",                  // (string) an URI, for example pointing to a website
    "divisible" : true|false        // (boolean) whether the tokens are divisible
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_listproperties"
```

---

### omni_getproperty

Returns details for about the tokens or smart property to lookup.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | the identifier of the tokens or property                                                     |

**Result:**
```js
{
  "propertyid" : n,               // (number) the identifier
  "name" : "name",                // (string) the name of the tokens
  "category" : "category",        // (string) the category used for the tokens
  "subcategory" : "subcategory",  // (string) the subcategory used for the tokens
  "data" : "information",         // (string) additional information or a description
  "url" : "uri",                  // (string) an URI, for example pointing to a website
  "divisible" : true|false,       // (boolean) whether the tokens are divisible
  "issuer" : "address",           // (string) the Bitcoin address of the issuer on record
  "creationtxid" : "hash",        // (string) the hex-encoded creation transaction hash
  "fixedissuance" : true|false,   // (boolean) whether the token supply is fixed
  "totaltokens" : "n.nnnnnnnn"    // (string) the total number of tokens in existence
}
```

**Example:**

```bash
$ omnicore-cli "omni_getproperty" 3
```

---

### omni_getactivecrowdsales

Lists currently active crowdsales.

**Arguments:**

*None*

**Result:**
```js
[                                // (array of JSON objects)
  {
    "propertyid" : n,                // (number) the identifier of the crowdsale
    "name" : "name",                 // (string) the name of the tokens issued via the crowdsale
    "issuer" : "address",            // (string) the Bitcoin address of the issuer on record
    "propertyiddesired" : n,         // (number) the identifier of the tokens eligible to participate in the crowdsale
    "tokensperunit" : "n.nnnnnnnn",  // (string) the amount of tokens granted per unit invested in the crowdsale
    "earlybonus" : n,                // (number) an early bird bonus for participants in percent per week
    "percenttoissuer" : n,           // (number) a percentage of tokens that will be granted to the issuer
    "starttime" : nnnnnnnnnn,        // (number) the start time of the of the crowdsale as Unix timestamp
    "deadline" : nnnnnnnnnn          // (number) the deadline of the crowdsale as Unix timestamp
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_getactivecrowdsales"
```

---

### omni_getcrowdsale

Returns information about a crowdsale.

**Arguments:**,

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | the identifier of the crowdsale                                                              |
| `verbose`           | boolean | optional | list crowdsale participants (default: `false`)                                               |

**Result:**
```js
{
  "propertyid" : n,                    // (number) the identifier of the crowdsale
  "name" : "name",                     // (string) the name of the tokens issued via the crowdsale
  "active" : true|false,               // (boolean) whether the crowdsale is still active
  "issuer" : "address",                // (string) the Bitcoin address of the issuer on record
  "propertyiddesired" : n,             // (number) the identifier of the tokens eligible to participate in the crowdsale
  "tokensperunit" : "n.nnnnnnnn",      // (string) the amount of tokens granted per unit invested in the crowdsale
  "earlybonus" : n,                    // (number) an early bird bonus for participants in percent per week
  "percenttoissuer" : n,               // (number) a percentage of tokens that will be granted to the issuer
  "starttime" : nnnnnnnnnn,            // (number) the start time of the of the crowdsale as Unix timestamp
  "deadline" : nnnnnnnnnn,             // (number) the deadline of the crowdsale as Unix timestamp
  "amountraised" : "n.nnnnnnnn",       // (string) the amount of tokens invested by participants
  "tokensissued" : "n.nnnnnnnn",       // (string) the total number of tokens issued via the crowdsale
  "addedissuertokens" : "n.nnnnnnnn",  // (string) the amount of tokens granted to the issuer as bonus
  "closedearly" : true|false,          // (boolean) whether the crowdsale ended early (if not active)
  "maxtokens" : true|false,            // (boolean) whether the crowdsale ended early due to reaching the limit of max. issuable tokens (if not active)
  "endedtime" : nnnnnnnnnn,            // (number) the time when the crowdsale ended (if closed early)
  "closetx" : "hash",                  // (string) the hex-encoded hash of the transaction that closed the crowdsale (if closed manually)
  "participanttransactions": [         // (array of JSON objects) a list of crowdsale participations (if verbose=true)
    {
      "txid" : "hash",                     // (string) the hex-encoded hash of participation transaction
      "amountsent" : "n.nnnnnnnn",         // (string) the amount of tokens invested by the participant
      "participanttokens" : "n.nnnnnnnn",  // (string) the tokens granted to the participant
      "issuertokens" : "n.nnnnnnnn"        // (string) the tokens granted to the issuer as bonus
    },
    ...
  ]
}
```

**Example:**

```bash
$ omnicore-cli "omni_getcrowdsale" 3 true
```

---

### omni_getgrants

Returns information about granted and revoked units of managed tokens.

**Arguments:**

*None*

**Result:**
```js
{
  "propertyid" : n,              // (number) the identifier of the managed tokens
  "name" : "name",               // (string) the name of the tokens
  "issuer" : "address",          // (string) the Bitcoin address of the issuer on record
  "creationtxid" : "hash",       // (string) the hex-encoded creation transaction hash
  "totaltokens" : "n.nnnnnnnn",  // (string) the total number of tokens in existence
  "issuances": [                 // (array of JSON objects) a list of the granted and revoked tokens
    {
      "txid" : "hash",               // (string) the hash of the transaction that granted tokens
      "grant" : "n.nnnnnnnn"         // (string) the number of tokens granted by this transaction
    },
    {
      "txid" : "hash",               // (string) the hash of the transaction that revoked tokens
      "grant" : "n.nnnnnnnn"         // (string) the number of tokens revoked by this transaction
    },
    ...
  ]
}
```

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | the identifier of the managed tokens to lookup                                               |

**Example:**

```bash
$ omnicore-cli "omni_getgrants" 31
```

---

### omni_getsto

Get information and recipients of a send-to-owners transaction.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `txid`              | string  | required | the hash of the transaction to lookup                                                        |
| `recipientfilter`   | string  | optional | a filter for recipients (wallet by default, `"*"` for all)                                   |

**Result:**
```js
{
  "txid" : "hash",               // (string) the hex-encoded hash of the transaction
  "sendingaddress" : "address",  // (string) the Bitcoin address of the sender
  "ismine" : true|false,         // (boolean) whether the transaction involes an address in the wallet
  "confirmations" : nnnnnnnnnn,  // (number) the number of transaction confirmations
  "fee" : "n.nnnnnnnn",          // (string) the transaction fee in bitcoins
  "blocktime" : nnnnnnnnnn,      // (number) the timestamp of the block that contains the transaction
  "valid" : true|false,          // (boolean) whether the transaction is valid
  "version" : n,                 // (number) the transaction version
  "type_int" : n,                // (number) the transaction type as number
  "type" : "type",               // (string) the transaction type as string
  "propertyid" : n,              // (number) the identifier of sent tokens
  "divisible" : true|false,      // (boolean) whether the sent tokens are divisible
  "amount" : "n.nnnnnnnn",       // (string) the number of tokens sent to owners
  "totalstofee" : "n.nnnnnnnn",    // (string) the fee paid by the sender, nominated in OMNI or TOMNI
  "recipients": [                  // (array of JSON objects) a list of recipients
    {
      "address" : "address",           // (string) the Bitcoin address of the recipient
      "amount" : "n.nnnnnnnn"          // (string) the number of tokens sent to this recipient
    },
    ...
  ]
}
```

**Example:**

```bash
$ omnicore-cli "omni_getsto" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d" "*"
```

---

### omni_gettrade

Get detailed information and trade matches for orders on the distributed token exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `txid`              | string  | required | the hash of the order to lookup                                                              |

**Result:**
```js
{
  "txid" : "hash",                              // (string) the hex-encoded hash of the transaction of the order
  "sendingaddress" : "address",                 // (string) the Bitcoin address of the trader
  "ismine" : true|false,                        // (boolean) whether the order involes an address in the wallet
  "confirmations" : nnnnnnnnnn,                 // (number) the number of transaction confirmations
  "fee" : "n.nnnnnnnn",                         // (string) the transaction fee in bitcoins
  "blocktime" : nnnnnnnnnn,                     // (number) the timestamp of the block that contains the transaction
  "valid" : true|false,                         // (boolean) whether the transaction is valid
  "version" : n,                                // (number) the transaction version
  "type_int" : n,                               // (number) the transaction type as number
  "type" : "type",                              // (string) the transaction type as string
  "propertyidforsale" : n,                      // (number) the identifier of the tokens put up for sale
  "propertyidforsaleisdivisible" : true|false,  // (boolean) whether the tokens for sale are divisible
  "amountforsale" : "n.nnnnnnnn",               // (string) the amount of tokens initially offered
  "propertyiddesired" : n,                      // (number) the identifier of the tokens desired in exchange
  "propertyiddesiredisdivisible" : true|false,  // (boolean) whether the desired tokens are divisible
  "amountdesired" : "n.nnnnnnnn",               // (string) the amount of tokens initially desired
  "unitprice" : "n.nnnnnnnnnnn..."              // (string) the unit price nominated in OMNI or TOMNI
  "status" : "status"                           // (string) the status of the order ("open", "cancelled", "filled", ...)
  "canceltxid" : "hash",                        // (string) the hash of the transaction that cancelled the order (if cancelled)
  "matches": [                                  // (array of JSON objects) a list of matched orders and executed trades
    {
      "txid" : "hash",                              // (string) the hash of the transaction that was matched against
      "block" : nnnnnn,                             // (number) the index of the block that contains this transaction
      "address" : "address",                        // (string) the Bitcoin address of the other trader
      "amountsold" : "n.nnnnnnnn",                  // (string) the number of tokens sold in this trade
      "amountreceived" : "n.nnnnnnnn"               // (string) the number of tokens traded in exchange
    },
    ...
  ]
}
```

**Example:**

```bash
$ omnicore-cli "omni_gettrade" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"
```

---

### omni_getorderbook

List active offers on the distributed token exchange.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | filter orders by `propertyid` for sale                                                       |
| `propertyid`        | number  | optional | filter orders by `propertyid` desired                                                        |

**Result:**
```js
[                                             // (array of JSON objects)
  {
    "address" : "address",                        // (string) the Bitcoin address of the trader
    "txid" : "hash",                              // (string) the hex-encoded hash of the transaction of the order
    "ecosystem" : "main"|"test",                  // (string) the ecosytem in which the order was made (if "cancel-ecosystem")
    "propertyidforsale" : n,                      // (number) the identifier of the tokens put up for sale
    "propertyidforsaleisdivisible" : true|false,  // (boolean) whether the tokens for sale are divisible
    "amountforsale" : "n.nnnnnnnn",               // (string) the amount of tokens initially offered
    "amountremaining" : "n.nnnnnnnn",             // (string) the amount of tokens still up for sale
    "propertyiddesired" : n,                      // (number) the identifier of the tokens desired in exchange
    "propertyiddesiredisdivisible" : true|false,  // (boolean) whether the desired tokens are divisible
    "amountdesired" : "n.nnnnnnnn",               // (string) the amount of tokens initially desired
    "amounttofill" : "n.nnnnnnnn",                // (string) the amount of tokens still needed to fill the offer completely
    "action" : n,                                 // (number) the action of the transaction: (1) "trade", (2) "cancel-price", (3) "cancel-pair", (4) "cancel-ecosystem"
    "block" : nnnnnn,                             // (number) the index of the block that contains the transaction
    "blocktime" : nnnnnnnnnn                      // (number) the timestamp of the block that contains the transaction
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_getorderbook" 2
```

---

### omni_gettradehistoryforpair

Retrieves the history of trades on the distributed token exchange for the specified market.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `propertyid`        | number  | required | the first side of the traded pair                                                            |
| `propertyid`        | number  | required | the second side of the traded pair                                                           |
| `count`             | number  | optional | number of trades to retrieve (default: `10`)                                                 |

**Result:**
```js
[                                     // (array of JSON objects)
  {
    "block" : nnnnnn,                     // (number) the index of the block that contains the trade match
    "unitprice" : "n.nnnnnnnnnnn..." ,    // (string) the unit price used to execute this trade (received/sold)
    "inverseprice" : "n.nnnnnnnnnnn...",  // (string) the inverse unit price (sold/received)
    "sellertxid" : "hash",                // (string) the hash of the transaction of the seller
    "address" : "address",                // (string) the Bitcoin address of the seller
    "amountsold" : "n.nnnnnnnn",          // (string) the number of tokens sold in this trade
    "amountreceived" : "n.nnnnnnnn",      // (string) the number of tokens traded in exchange
    "matchingtxid" : "hash",              // (string) the hash of the transaction that was matched against
    "matchingaddress" : "address"         // (string) the Bitcoin address of the other party of this trade
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_gettradehistoryforpair" 1 12 500
```

---

### omni_gettradehistoryforaddress

Retrieves the history of orders on the distributed exchange for the supplied address.

**Arguments:**

| Name                | Type    | Presence | Description                                                                                  |
|---------------------|---------|----------|----------------------------------------------------------------------------------------------|
| `address`           | string  | required | address to retrieve history for                                                              |
| `count`             | number  | optional | number of orders to retrieve (default: `10`)                                                 |
| `propertyid`        | number  | optional | filter by propertyid transacted (default: no filter)                                         |

**Result:**
```js
[                                             // (array of JSON objects)
  {
    "txid" : "hash",                              // (string) the hex-encoded hash of the transaction of the order
    "sendingaddress" : "address",                 // (string) the Bitcoin address of the trader
    "ismine" : true|false,                        // (boolean) whether the order involes an address in the wallet
    "confirmations" : nnnnnnnnnn,                 // (number) the number of transaction confirmations
    "fee" : "n.nnnnnnnn",                         // (string) the transaction fee in bitcoins
    "blocktime" : nnnnnnnnnn,                     // (number) the timestamp of the block that contains the transaction
    "valid" : true|false,                         // (boolean) whether the transaction is valid
    "version" : n,                                // (number) the transaction version
    "type_int" : n,                               // (number) the transaction type as number
    "type" : "type",                              // (string) the transaction type as string
    "propertyidforsale" : n,                      // (number) the identifier of the tokens put up for sale
    "propertyidforsaleisdivisible" : true|false,  // (boolean) whether the tokens for sale are divisible
    "amountforsale" : "n.nnnnnnnn",               // (string) the amount of tokens initially offered
    "propertyiddesired" : n,                      // (number) the identifier of the tokens desired in exchange
    "propertyiddesiredisdivisible" : true|false,  // (boolean) whether the desired tokens are divisible
    "amountdesired" : "n.nnnnnnnn",               // (string) the amount of tokens initially desired
    "unitprice" : "n.nnnnnnnnnnn..."              // (string) the unit price nominated in OMNI or TOMNI
    "status" : "status"                           // (string) the status of the order ("open", "cancelled", "filled", ...)
    "canceltxid" : "hash",                        // (string) the hash of the transaction that cancelled the order (if cancelled)
    "matches": [                                  // (array of JSON objects) a list of matched orders and executed trades
      {
        "txid" : "hash",                              // (string) the hash of the transaction that was matched against
        "block" : nnnnnn,                             // (number) the index of the block that contains this transaction
        "address" : "address",                        // (string) the Bitcoin address of the other trader
        "amountsold" : "n.nnnnnnnn",                  // (string) the number of tokens sold in this trade
        "amountreceived" : "n.nnnnnnnn"               // (string) the number of tokens traded in exchange
      },
      ...
    ]
  },
  ...
]
```

**Example:**

```bash
$ omnicore-cli "omni_gettradehistoryforaddress" "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8"
```

---

## Depreciated API calls

To ensure backwards compatibility, depreciated RPCs are kept for at least one major version.

The following calls are replaced in Omni Core 0.0.10, and queries with the old command are forwarded.

- `send_MP` by `omni_send`
- `sendtoowners_MP` by `omni_sendsto`
- `sendrawtx_MP` by `omni_sendrawtx`
- `getinfo_MP` by `omni_getinfo`
- `getbalance_MP` by `omni_getbalance`
- `getallbalancesforid_MP` by `omni_getallbalancesforid`
- `getallbalancesforaddress_MP` by `omni_getallbalancesforaddress`
- `gettransaction_MP` by `omni_gettransaction`
- `listtransactions_MP` by `omni_listtransactions`
- `listblocktransactions_MP` by `omni_listblocktransactions`
- `getactivedexsells_MP` by `omni_getactivedexsells`
- `listproperties_MP` by `omni_listproperties`
- `getproperty_MP` by `omni_getproperty`
- `getactivecrowdsales_MP` by `omni_getactivecrowdsales`
- `getcrowdsale_MP` by `omni_getcrowdsale`
- `getgrants_MP` by `omni_getgrants`
- `getsto_MP` by `omni_getsto` or `omni_gettransaction`
