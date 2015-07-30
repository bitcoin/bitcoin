Omni Core API Documentation
===========================

## Introduction

Omni Core is a fork of Bitcoin Core, with Omni Protocol feature support added as a new layer of functionality on top. As such interacting with the API is done in the same manner (JSON-RPC) as Bitcoin Core, simply with additional RPC calls available for utilizing Omni Protocol features.

As all existing Bitcoin Core functionality is inherent to Omni Core, the RPC port by default remains as `8332` as per Bitcoin Core.  If you wish to run Omni Core in tandem with Bitcoin Core (eg. via a separate datadir) you may utilize the `-rpcport<port>` option to nominate an alternative port number.

*Please note: this document is a work in progress and is subject to change at any time. There may be errors, omissions or inaccuracies present.*


## RPC Calls for Transaction Creation

The RPC calls for transaction creation can be used to create and broadcast Omni Protocol transactions.

A hash of the broadcasted transaction is returned as result.

### omni_send

Create and broadcast a simple send transaction.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***toaddress            (string, required):*** the address of the receiver
3. ***propertyid           (number, required):*** the identifier of the tokens to send
4. ***amount               (string, required):*** the amount to send

**Example:**
 
```bash
$ omnicore-cli "omni_send" "3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "100.0"
```

### omni_senddexsell

Place, update or cancel a sell offer on the traditional distributed MSC/BTC exchange.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***propertyidforsale    (number, required):*** the identifier of the tokens to list for sale (must be MSC or TMSC)
3. ***amountforsale        (string, required):*** the amount of tokens to list for sale
4. ***amountdesired        (string, required):*** the amount of bitcoins desired
5. ***paymentwindow        (number, required):*** a time limit in blocks a buyer has to pay following a successful accept
6. ***minacceptfee         (string, required):*** a minimum mining fee a buyer has to pay to accept the offer
7. ***action               (number, required):*** the action to take: (1) new, (2) update, (3) cancel

**Example:**

```bash
$ omnicore-cli "omni_senddexsell" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "1.5" "0.75" 25 "0.0005" 1
```

### omni_senddexaccept

Create and broadcast an accept offer for the specified token and amount.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***toaddress            (string, required):*** the address of the seller
3. ***propertyid           (number, required):*** the identifier of the token to purchase
4. ***amount               (string, required):*** the amount to accept
5. ***override             (boolean, optional):*** override minimum accept fee and payment window checks (use with caution!)

**Example:**

```bash
$ omnicore-cli "omni_senddexaccept", "35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 1 "15.0"
```

### omni_sendissuancecrowdsale

Create new tokens as crowdsale.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***ecosystem            (string, required):*** the ecosystem to create the tokens in: (1) main, (2) test
3. ***type                 (number, required):*** the type of the tokens to create: (1) indivisible, (2) divisible
4. ***previousid           (number, optional):*** an identifier of a predecessor token (0 for new crowdsales)
5. ***category             (string, optional):*** a category for the new tokens (can be "")
6. ***subcategory          (string, optional):*** a subcategory for the new tokens  (can be "")
7. ***name                 (string, required):*** the name of the new tokens to create
8. ***url                  (string, optional):*** an URL for further information about the new tokens (can be "")
9. ***data                 (string, optional):*** a description for the new tokens (can be "")
10. ***propertyiddesired   (number, required):*** the identifier of a token eligible to participate in the crowdsale
11. ***tokensperunit       (string, required):*** the amount of tokens granted per unit invested in the crowdsale
12. ***deadline            (number, required):*** the deadline of the crowdsale as Unix timestamp
13. ***earlybonus          (number, required):*** an early bird bonus for participants in percent per week (default: 0)
14. ***issuerpercentage    (number, required):*** a percentage of tokens that will be granted to the issuer (default: 0)

**Example:**

```bash
$ omnicore-cli "omni_sendissuancecrowdsale", "3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo" 2 1 0 "Companies" "Bitcoin Mining" "Quantum Miner" "" "" 1 "100" 1483228800 30 2
```

### omni_sendissuancefixed

Create new tokens with fixed supply.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***ecosystem            (string, required):*** the ecosystem to create the tokens in: (1) main, (2) test
3. ***type                 (number, required):*** the type of the tokens to create: (1) indivisible, (2) divisible
4. ***previousid           (number, optional):*** an identifier of a predecessor token (0 for new tokens)
5. ***category             (string, optional):*** a category for the new tokens (can be "")
6. ***subcategory          (string, optional):*** a subcategory for the new tokens  (can be "")
7. ***name                 (string, required):*** the name of the new tokens to create
8. ***url                  (string, optional):*** an URL for further information about the new tokens (can be "")
9. ***data                 (string, optional):*** a description for the new tokens (can be "")
10. ***amount              (string, required):*** the number of tokens to create

**Example:**

```bash
$ omnicore-cli "omni_sendissuancefixed", "3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3" 2 1 0 "Companies" "Bitcoin Mining" "Quantum Miner" "" "" "1000000"
```

### omni_sendissuancemanaged

Create new tokens with manageable supply.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***ecosystem            (string, required):*** the ecosystem to create the tokens in: (1) main, (2) test
3. ***type                 (number, required):*** the type of the tokens to create: (1) indivisible, (2) divisible
4. ***previousid           (number, optional):*** an identifier of a predecessor token (0 for new tokens)
5. ***category             (string, optional):*** a category for the new tokens (can be "")
6. ***subcategory          (string, optional):*** a subcategory for the new tokens  (can be "")
7. ***name                 (string, required):*** the name of the new tokens to create
8. ***url                  (string, optional):*** an URL for further information about the new tokens (can be "")
9. ***data                 (string, optional):*** a description for the new tokens (can be "")

**Example:**

```bash
$ omnicore-cli "omni_sendissuancemanaged" "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" 2 1 0 "Companies" "Bitcoin Mining" "Quantum Miner" "" ""
```

### omni_sendsto

Create new tokens with manageable supply.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***propertyid           (number, required):*** the identifier of the tokens to distribute
3. ***amount               (string, required):*** the amount to distribute
4. ***redeemaddress        (string, optional):*** an address that can spend the transaction dust (sender by default)

**Example:**

```bash
$ omnicore-cli "omni_sendsto" "32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa" 3 "5000"
```

### omni_sendgrant

Issue or grant new units of managed tokens.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***toaddress            (string, optional):*** the receiver of the tokens (sender by default)
3. ***propertyid           (number, required):*** the identifier of the tokens to grant
4. ***amount               (string, required):*** the amount of tokens to create
5. ***memo                 (string, optional):*** a text note attached to this transaction (none by default)

**Example:**

```bash
$ omnicore-cli "omni_sendgrant" "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" "" 51 "7000"
```

### omni_sendrevoke

Revoke units of managed tokens.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to revoke the tokens from
2. ***propertyid           (number, required):*** the identifier of the tokens to revoke
3. ***amount               (string, required):*** the amount of tokens to revoke
4. ***memo                 (string, optional):*** a text note attached to this transaction (none by default)

**Example:**

```bash
$ omnicore-cli "omni_sendrevoke" "3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH" "" 51 "100"
```

### omni_sendclosecrowdsale

Manually close a crowdsale.

**Arguments:**

1. ***fromaddress          (string, required):*** the address associated with the crowdsale to close
2. ***propertyid           (number, required):*** the identifier of the crowdsale to close

**Example:**

```bash
$ omnicore-cli "omni_sendclosecrowdsale" "3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo" 70
```

### omni_sendtrade

Place a trade offer on the distributed token exchange.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to trade with
2. ***propertyidforsale    (number, required):*** the identifier of the tokens to list for sale
3. ***amountforsale        (string, required):*** the amount of tokens to list for sale
4. ***propertiddesired     (number, required):*** the identifier of the tokens desired in exchange
5. ***amountdesired        (string, required):*** the amount of tokens desired in exchange

**Example:**

```bash
$ omnicore-cli "omni_sendtrade" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 31 "250.0" 1 "10.0"
```

### omni_sendcanceltradesbyprice

Cancel offers on the distributed token exchange with the specified price.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to trade with
2. ***propertyidforsale    (number, required):*** the identifier of the tokens listed for sale
3. ***amountforsale        (string, required):*** the amount of tokens to listed for sale
4. ***propertiddesired     (number, required):*** the identifier of the tokens desired in exchange
5. ***amountdesired        (string, required):*** the amount of tokens desired in exchange

**Example:**

```bash
$ omnicore-cli "omni_sendcanceltradesbyprice" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 31 "100.0" 1 "5.0"
```

### omni_sendcanceltradesbypair

Cancel all offers on the distributed token exchange with the given currency pair.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to trade with
2. ***propertyidforsale    (number, required):*** the identifier of the tokens listed for sale
3. ***propertiddesired     (number, required):*** the identifier of the tokens desired in exchange

**Example:**

```bash
$ omnicore-cli "omni_sendcanceltradesbypair" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 1 31
```

### omni_sendcancelalltrades

Cancel all offers on the distributed token exchange.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to trade with
2. ***ecosystem            (number, required):*** the ecosystem of the offers to cancel: (0) both, (1) main, (2) test

**Example:**

```bash
$ omnicore-cli "omni_sendcancelalltrades" "3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR" 1
```

### omni_sendchangeissuer

Change the issuer on record of the given tokens.

**Arguments:**

1. ***fromaddress          (string, required):*** the address associated with the tokens
2. ***toaddress            (string, required):*** the address to transfer administrative control to
3. ***propertyid           (number, required):*** the identifier of the tokens

**Example:**

```bash
$ omnicore-cli "omni_sendchangeissuer" "1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu" "3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs" 3
```

### omni_sendall

Transfers *all* tokens owned to the recipient.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***toaddress            (string, required):*** the address of the receiver
3. ***redeemaddress        (string, optional):*** an address that can spend the transaction dust (sender by default)
4. ***referenceamount      (string, optional):*** a bitcoin amount that is sent to the receiver (minimal by default)

**Example:**

```bash
$ omnicore-cli "omni_sendall" "3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY" "37FaKponF7zqoMLUjEiko25pDiuVH5YLEa"
```

### omni_sendrawtx

Broadcasts a raw Omni Layer transaction.

**Arguments:**

1. ***fromaddress          (string, required):*** the address to send from
2. ***rawtransaction       (string, required):*** the hex-encoded raw transaction
3. ***referenceaddress     (string, optional):*** a reference address (empty by default)
4. ***redeemaddress        (string, optional):*** an address that can spend the transaction dust (sender by default)
5. ***referenceamount      (string, optional):*** a bitcoin amount that is sent to the receiver (minimal by default)

**Example:**

```bash
$ omnicore-cli "omni_sendrawtx" "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8" "000000000000000100000000017d7840" "1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV"
```


## RPC Calls for Data Retrieval

The RPC calls for data retrieval can be used to get information about the state of the Omni ecosystem.

### omni_getinfo

Returns various state information of the client and protocol.

**Example:**

```bash
$ omnicore-cli "omni_getinfo"
```

### omni_getbalance

Returns the token balance for a given address and property.

**Arguments:**

1. ***address              (string, required):*** the address
2. ***propertyid           (number, required):*** the property identifier

**Example:**

```bash
$ omnicore-cli "omni_getbalance", "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P" 1
```

### omni_getallbalancesforid

Returns a list of token balances for a given currency or property identifier.

**Arguments:**

1. ***propertyid           (number, required):*** the property identifier

**Example:**

```bash
$ omnicore-cli "omni_getallbalancesforid" 1
```

### omni_getallbalancesforaddress

Returns a list of all token balances for a given address.

**Arguments:**

1. ***address              (string, required):*** the address

**Example:**

```bash
$ omnicore-cli "omni_getallbalancesforaddress" "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"
```

### omni_gettransaction

Get detailed information about an Omni transaction.

**Arguments:**

1. txid                 (string, required):*** the hash of the transaction to lookup

**Example:**

```bash
$ omnicore-cli "omni_gettransaction" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"
```

### omni_listtransactions

List wallet transactions, optionally filtered by an address and block boundaries.

**Arguments:**

1. ***address              (string, optional):*** address filter (default: "\*")
2. ***count                (number, optional):*** show at most n transactions (default: 10)
3. ***skip                 (number, optional):*** skip the first n transactions (default: 0)
4. ***startblock           (number, optional):*** first block to begin the search (default: 0)
5. ***endblock             (number, optional):*** last block to include in the search (default: 999999)

**Example:**

```bash
$ omnicore-cli "omni_listtransactions"
```

### omni_listblocktransactions

Lists all Omni transactions in a block.

**Arguments:**

1. ***index                (number, required):*** the block height or block index

**Example:**

```bash
$ omnicore-cli "omni_listblocktransactions" 279007
```

### omni_getactivedexsells

Returns currently active offers on the distributed exchange.

**Arguments:**

1. ***address              (string, optional):*** address filter (default: include any)

**Example:**

```bash
$ omnicore-cli "omni_getactivedexsells"
```

### omni_listproperties

Lists all tokens or smart properties.

**Example:**

```bash
$ omnicore-cli "omni_listproperties"
```

### omni_getproperty

Returns details for about the tokens or smart property to lookup.

**Arguments:**

1. ***propertyid           (number, required):*** the identifier of the tokens or property

**Example:**

```bash
$ omnicore-cli "omni_getproperty" 3
```

### omni_getactivecrowdsales

Lists currently active crowdsales.

**Example:**

```bash
$ omnicore-cli "omni_getactivecrowdsales"
```

### omni_getcrowdsale

Returns information about a crowdsale.

**Arguments:**

1. ***propertyid           (number, required):*** the identifier of the crowdsale
2. ***verbose              (boolean, optional):*** list crowdsale participants (default: false)

**Example:**

```bash
$ omnicore-cli "omni_getcrowdsale" 3 true
```

### omni_getgrants

Returns information about granted and revoked units of managed tokens.

**Arguments:**

1. ***propertyid           (number, required):*** the identifier of the managed tokens to lookup

**Example:**

```bash
$ omnicore-cli "omni_getgrants" 31
```

### omni_getsto

Get information and recipients of a send-to-owners transaction.

**Arguments:**

1. ***txid                 (string, required):*** the hash of the transaction to lookup
2. ***recipientfilter      (string, optional):*** a filter for recipients (wallet by default, "\*" for all)

**Example:**

```bash
$ omnicore-cli "omni_getsto" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d" "*"
```

### omni_gettrade

Get detailed information and trade matches for orders on the distributed token exchange.

**Arguments:**

1. ***txid                 (string, required):*** the hash of the order to lookup

**Example:**

```bash
$ omnicore-cli "omni_gettrade" "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"
```

### omni_getorderbook

List active offers on the distributed token exchange.

**Arguments:**

1. ***propertyid           (number, required):*** filter orders by propertyid for sale
2. ***propertyid           (number, optional):*** filter orders by propertyid desired

**Example:**

```bash
$ omnicore-cli "omni_getorderbook" 2
```

### omni_gettradehistoryforpair

Retrieves the history of trades on the distributed token exchange for the specified market.

**Arguments:**

1. ***propertyid           (number, required):*** the first side of the traded pair
2. ***propertyid           (number, required):*** the second side of the traded pair
3. ***count                (number, optional):*** number of trades to retrieve (default: 10)

**Example:**

```bash
$ omnicore-cli "omni_gettradehistoryforpair" 1 12 500
```

### omni_gettradehistoryforaddress

Retrieves the history of orders on the distributed exchange for the supplied address.

**Arguments:**

1. ***address              (string, required):*** address to retrieve history for
2. ***count                (number, optional):*** number of orders to retrieve (default: 10)
3. ***propertyid           (number, optional):*** filter by propertyid transacted (default: no filter)

**Example:**

```bash
$ omnicore-cli "omni_gettradehistoryforaddress" "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8"
```


## Depreciated RPC Calls

To ensure compability, depreciated RPC calls are kept for at least one major version.

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
