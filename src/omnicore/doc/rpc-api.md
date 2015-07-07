Master Core API Documentation
=============================

##Introduction
Master Core is a fork of Bitcoin Core, with Master Protocol feature support added as a new layer of functionality on top.  As such interacting with the API is done in the same manner (JSON-RPC) as Bitcoin Core, simply with additional RPC calls available for utlizing Master Protocol features.

As all existing Bitcoin Core functionality is inherent to Master Core, the RPC port by default remains as 8332 as per Bitcoin Core.  If you wish to run Master Core in tandem with Bitcoin Core (eg via a seperate datadir) you may utilize the -rpcport<port> option to nominate an alternative port number.

*Caution: This document is a work in progress and is subject to change at any time.  There may be errors, omissions or inaccuracies present.*

*Note: As of release 0.0.7, all floating point values are taken in and output as strings. This is to preserve precision when dealing with large numbers. Integer fields typically will stay in integer amounts. The fields generally affected by this change will be the "amount" field.

In addition to this change, 0.0.6 "reserved" amounts are no longer split into two reserved fields, accept and sell, but into one complete "reserved" field, in 0.0.7. 

###Obtaining the state of the client
The **getinfo_MP** call provides information about the current state of the client.

*Note, it is important to periodically poll the getinfo_MP call to check there is no current alert.  The OmniCore alert system is seperate to the Bitcoin alert system and has been coded to also trigger the existing alertnotify script if specified at startup, however this has had very limited testing as of current so a periodical poll is suggested at this time.

**Required Parameters**
- There are no required parameters for this call.

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getinfo_MP
{
    "mastercoreversion" : "0.0.9-dev",
    "bitcoincoreversion" : "0.903",
    "commitinfo" : "2b1010f",
    "block" : 341613,
    "blocktime" : 1422872445,
    "blocktransactions" : 0,
    "totaltrades" : 0,
    "totaltransactions" : 19762,
    "alert" : {
    }
}
```
```
{"jsonrpc":"1.0","id":"1","method":"getinfo_MP","params":[]}
{"result":{"mastercoreversion":"0.0.9-dev","bitcoincoreversion":"0.903","commitinfo":"2b1010f","block":341613,"blocktime":1422872445,"blocktransactions":0,"totaltrades":0,"totaltransactions":19762,"alert":{}},"error":null,"id":"1"}
```

###Broadcasting a Simple Send transaction
Simple send allows a Master Protocol currency to be transferred from address to address in a one-to-one transaction.  Simple send transactions are exposed via the **send_MP** RPC call.

**Required Parameters**
- **_sender address (string):_** A valid bitcoin address containing a sufficient balance to support the transaction
- **_recipient address (string):_** A valid bitcoin address - the receiving party of the transaction
- **_currency/property ID (integer):_** A valid Master Protocol currency/property ID
- **_amount (string):_** The amount to transfer (note if sending individisble tokens any decimals will be truncated)
   
**Additional Optional Parameters**
- **_redeem address (string):_** The address that can redeem the bitcoin/multisig outputs. Defaults to FromAddress, required to redeem MP transactions created with P2SH.
- **_reference amount (string):_** Additional Bitcoin amount to send to the recepient address. (Max: 0.01 BTC).

**Examples**
```
$src/mastercored send_MP myN6HXmFhmMRo1bzfNXBDxTALYsh3EjXxk mvKKTXj8Z1GVwjN1Ejw8yx6n7pBujdXG2Q 1 "1.234"
d300bb52c099c664459a75908255c8ec6a58575ac8efb07080bd81d8e6c9af40
```
```
{"jsonrpc":"1.0","id":"1","method":"send_MP","params":["myN6HXmFhmMRo1bzfNXBDxTALYsh3EjXxk","mvKKTXj8Z1GVwjN1Ejw8yx6n7pBujdXG2Q",1,"1.234"]}
{"result":"d300bb52c099c664459a75908255c8ec6a58575ac8efb07080bd81d8e6c9af40","error":null,"id":"1"}
```
*Please note, the private key for the requested sender address must be available in the wallet.*

###Obtaining a Master Protocol balance
The **getbalance_MP** call allows for retrieval of a Master Protocol balance for a given address and currency/property ID.

**Required Parameters**
- **_address (string):_** A valid bitcoin address 
- **_property ID (integer):_** A valid Master Protocol property ID

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getbalance_MP mvKKTXj8Z1GVwjN1Ejw8yx6n7pBujdXG2Q 1
300000000.00000000
```
```
{"jsonrpc":"1.0","id":"1","method":"getbalance_MP","params":["mvKKTXj8Z1GVwjN1Ejw8yx6n7pBujdXG2Q",1]}
{"result":300000000.00000000,"error":null,"id":"1"}
```

###Obtaining all Master Protocol balances for an address
The **getallbalancesforaddress_MP** call allows for retrieval of all Master Protocol balances for a given address.

**Required Parameters**
- **_address (string):_** A valid bitcoin address

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getallbalancesforaddress_MP 1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8
[
    {
        "propertyid" : 1,
        "balance" : "0.05721789",
        "reserved" : "0.00000000"
    },
    {
        "propertyid" : 2147483651,
        "balance" : "31279045",
        "reserved" : "0"
    }
]
```
```
{"jsonrpc":"1.0","id":"1","method":"getallbalancesforaddress_MP","params":["1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8"]}
{"result":[{"propertyid":1,"balance":"0.05721789","reserved":"0.00000000"},{"propertyid":2147483651,"balance":"31279045","reserved":"0"}],"error":null,"id":"1"}
```

###Obtaining all Master Protocol balances for a property ID
The **getallbalancesforid_MP** call allows for retrieval of all Master Protocol balances for a given property ID.

**Required Parameters**
- **_property ID (integer):_** A valid Master Protocol property ID

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getallbalancesforid_MP 2147483652
[
    {
        "address" : "1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV",
        "balance" : "3214",
        "reserved" : "0"
    },
    {
        "address" : "1MCHESTbJhJK27Ygqj4qKkx4Z4ZxhnP826",
        "balance" : "11",
        "reserved" : "0"
    },
    {
        "address" : "1MCHESTxYkPSLoJ57WBQot7vz3xkNahkcb",
        "balance" : "149",
        "reserved" : "0"
    }
]
```
```
{"jsonrpc":"1.0","id":"1","method":"getallbalancesforid_MP","params":[2147483652]}
{"result":[{"address":"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV","balance":"3214","reserved":"0"},{"address":"1MCHESTbJhJK27Ygqj4qKkx4Z4ZxhnP826","balance":"11","reserved":"0"},{"address":"1MCHESTxYkPSLoJ57WBQot7vz3xkNahkcb","balance":"149","reserved":"0"}],"error":null,"id":"1"}
```

###Retrieving a Master Protocol Transaction
The **gettransaction_MP** call allows for retrieval of a Master Protocol transaction and its associated details and validity.  

**Required Parameters**
- **_transaction ID (string):_** A valid Master Protocol transaction ID

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored gettransaction_MP d2907fe2c716fc6d510d63b52557907445c784cb2e8ae6ea9ef61e909c978cd7
{
    "txid" : "d2907fe2c716fc6d510d63b52557907445c784cb2e8ae6ea9ef61e909c978cd7",
    "sendingaddress" : "myN6HXmFhmMRo1bzfNXBDxTALYsh3EjXxk",
    "referenceaddress" : "mhgrKJ3WyX1RMYiUpgA3M3iF48zSeSRkri",
    "direction" : "out",
    "confirmations" : 884,
    "fee" : "0.00010000",
    "blocktime" : 1403298479,
    "blockindex" : 49,
    "type" : "Simple Send",
    "currency" : 1,
    "divisible" : true,
    "amount" : "50.00000000",
    "valid" : true
}
```
```
{"jsonrpc":"1.0","id":"1","method":"gettransaction_MP","params":["d2907fe2c716fc6d510d63b52557907445c784cb2e8ae6ea9ef61e909c978cd7"]}
{"result":{"txid":"d2907fe2c716fc6d510d63b52557907445c784cb2e8ae6ea9ef61e909c978cd7","sendingaddress":"myN6HXmFhmMRo1bzfNXBDxTALYsh3EjXxk","referenceaddress":"mhgrKJ3WyX1RMYiUpgA3M3iF48zSeSRkri","direction":"out","confirmations":884,"fee":"0.00010000","blocktime":1403298479,"blockindex":49,"type":"Simple Send","currency":1,"divisible":true,"amount":"50.00000000","valid":true},"error":null,"id":"1"}
```

###Listing Historical Transactions
The **listtransactions_MP** call allows for retrieval of the last n Master Protocol transactions from the wallet, if desired filtered on address.

**Required Parameters**
- There are no required parameters for this call.  Calling with no parameters will default to all addresses in the wallet and the last 10 transactions.

**Additional Optional Parameters**
- **_address (string):_** A valid bitcoin address to filter on or * for all addresses in the wallet
- **_count (integer):_** The number of recent transactions to return
- **_skip (integer):_** The number of recent transactions to skip
- **_startblock (integer):_** Only show transactions at or after this block height
- **_endblock (integer):_** Only show transactions at or before this block height

Optional parameters can be combined as follows ```listtransactions_MP "*" 50 100``` to list the 50 most recent transactions across all addresses in the wallet, skipping the first 100.

Optional parameters can be combined as follows ```listtransactions_MP "*" 99999 0 301000 302000``` to list the 99999 most recent transactions across all addresses in the wallet between blocks 301000 and 302000.

**Examples**

```
$src/bitcoind listtransactions_MP 1MCHESTbJhJK27Ygqj4qKkx4Z4ZxhnP826 2
[
    {
        "txid" : "0964e197c9140e4b5a2720d3f6c1345d01ca5813652da88b6b41fe7331691778",
        "sendingaddress" : "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8",
        "referenceaddress" : "1MCHESTbJhJK27Ygqj4qKkx4Z4ZxhnP826",
        "confirmations" : 32,
        "blocktime" : 1407411602,
        "blockindex" : 780,
        "type" : "DEx Accept Offer",
        "propertyid" : 2,
        "divisible" : true,
        "amount" : "1.00000000",
        "valid" : true
    },
    {
        "txid" : "4bb77bfcfd9be37a330fab9270a395f9bb871eb303be069ad58a7d74545427b0",
        "sendingaddress" : "1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8",
        "referenceaddress" : "1MCHESTbJhJK27Ygqj4qKkx4Z4ZxhnP826",
        "confirmations" : 23,
        "blocktime" : 1407418741,
        "blockindex" : 365,
        "type" : "DEx Accept Offer",
        "propertyid" : 2,
        "divisible" : true,
        "amount" : "4.00000000",
        "valid" : true
    }
]
```
*Please note, listtransactions_MP currently supports transactions available in the wallet only.*

###Retrieving information about a Master Protocol property
The **getproperty_MP** call allows for retrieval of information about a Master Protocol property.

**Required Parameters**
- **_property ID (integer):_** A valid Master Protocol property ID

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getproperty_MP 3
{
    "name" : "MaidSafeCoin",
    "category" : "Crowdsale",
    "subcategory" : "MaidSafe",
    "data" : "SAFE Network Crowdsale (MSAFE)",
    "url" : "www.buysafecoins.com",
    "divisible" : false,
    "issuer" : "1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu",
    "creationtxid" : "86f214055a7f4f5057922fd1647e00ef31ab0a3ff15217f8b90e295f051873a7",
    "fixedissuance" : false,
    "totaltokens" : "452552412"
}
```
```
{"jsonrpc":"1.0","id":"1","method":"getproperty_MP","params":[3]}
{"result":{"name":"MaidSafeCoin","category":"Crowdsale","subcategory":"MaidSafe","data":"SAFE Network Crowdsale (MSAFE)","url":"www.buysafecoins.com","divisible":false,"issuer":"1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu","creationtxid":"86f214055a7f4f5057922fd1647e00ef31ab0a3ff15217f8b90e295f051873a7","fixedissuance":false,"totaltokens":"452552412"},"error":null,"id":"1"}
```

###Listing Master Protocol properties
The **listproperties_MP** call allows for listing of Master Protocol smart properties.

**Required Parameters**
- There are no required parameters for this call.

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored listproperties_MP
[
    {
        "propertyid" : 1,
        "name" : "MasterCoin",
        "category" : "N/A",
        "subcategory" : "N/A",
        "data" : "***data***",
        "url" : "www.mastercoin.org",
        "divisible" : true
    },
    {
        "propertyid" : 2,
        "name" : "Test MasterCoin",
        "category" : "N/A",
        "subcategory" : "N/A",
        "data" : "***data***",
        "url" : "www.mastercoin.org",
        "divisible" : true
    },
    {
        "propertyid" : 3,
        "name" : "MaidSafeCoin",
        "category" : "Crowdsale",
        "subcategory" : "MaidSafe",
        "data" : "SAFE Network Crowdsale (MSAFE)",
        "url" : "www.buysafecoins.com",
        "divisible" : false
    },
    {
        "propertyid" : 4,
        "name" : "Craig",
        "category" : "Personal",
        "subcategory" : "Name",
        "data" : "Be.",
        "url" : "udecker.com",
        "divisible" : true
    },
    ..........
]
```
```
{"jsonrpc":"1.0","id":"1","method":"listproperties_MP","params":[]}
{"result":[{"propertyid":1,"name":"MasterCoin","category":"N/A","subcategory":"N/A","data":"***data***","url":"www.mastercoin.org","divisible":true},{"propertyid":2,"name":"Test MasterCoin","category":"N/A","subcategory":"N/A","data":"***data***","url":"www.mastercoin.org","divisible":true},{"propertyid":3,"name":"MaidSafeCoin","category":"Crowdsale","subcategory":"MaidSafe","data":"SAFE Network Crowdsale (MSAFE)","url":"www.buysafecoins.com","divisible":false},{"propertyid":4,"name":"Craig","category":"Personal","subcategory":"Name","data":"Be.","url":"udecker.com","divisible":true},{"propertyid":5,"name":"UpbeatOctoProp","category":"Goodwill","subcategory":"Thank You","data":"The Upbeat Octopus Appreciates You!","url":"http://upbeatoctopus.com","divisible":false},{"propertyid":6,"name":"GENERcoin","category":"Tangible Property","subcategory":"Renewable Biofuel","data":"Arterran Renewables Biofuel (10kbtu)","url":"www.arterranrenewables.com","divisible":false},{"propertyid":7,"name":"GENERcoin","category":"Tangible Property","subcategory":"Renewable Biofuel","data":"Arterran Renewables Biofuel (10kbtu)","url":"www.genercoin.org","divisible":false},{"propertyid":8,"name":"Vend1000","category":"Vend1000","subcategory":"Vend1000","data":"Vend1000","url":"Vend1000","divisible":true},{"propertyid":9,"name":"Zend1","category":"test","subcategory":"test","data":"test","url":"test","divisible":true},{"propertyid":10,"name":"Zend3","category":"test3","subcategory":"test3","data":"test3","url":"test3","divisible":true},{"propertyid":11,"name":"Zend2","category":"test2","subcategory":"test2","data":"test2","url":"test2","divisible":true},{"propertyid":12,"name":"Zend4","category":"Zend4","subcategory":"Zend4","data":"Zend4","url":"Zend4","divisible":true},{"propertyid":13,"name":"Zend5","category":"Zend5","subcategory":"Zend5","data":"Zend5","url":"Zend5","divisible":true},{"propertyid":14,"name":"Zend9","category":"Zend9","subcategory":"Zend9","data":"Zend9","url":"Zend9","divisible":true},{"propertyid":15,"name":"Zend8","category":"Zend8","subcategory":"Zend8","data":"Zend8","url":"Zend8","divisible":true},{"propertyid":16,"name":"Zend10","category":"Zend10","subcategory":"Zend10","data":"Zend10","url":"Zend10","divisible":true},{"propertyid":17,"name":"Zend7","category":"Zend7","subcategory":"Zend7","data":"Zend7","url":"Zend7","divisible":true},{"propertyid":18,"name":"Zend6","category":"Zend6","subcategory":"Zend6","data":"Zend6","url":"Zend6","divisible":true},{"propertyid":19,"name":"Zend7","category":"Zend7","subcategory":"Zend7","data":"Zend7","url":"Zend7","divisible":true},{"propertyid":20,"name":"Zend3","category":"test3","subcategory":"test3","data":"test3","url":"test3","divisible":true},{"propertyid":21,"name":"Zend7","category":"Zend7","subcategory":"Zend7","data":"Zend7","url":"Zend7","divisible":true},{"propertyid":22,"name":"Zend5","category":"Zend5","subcategory":"Zend5","data":"Zend5","url":"Zend5","divisible":true},{"propertyid":23,"name":"Zend1000","category":"Zend1000","subcategory":"Zend1000","data":"Zend1000","url":"Zend1000","divisible":true},{"propertyid":24,"name":"BitStrapAccessToken","category":"Other","subcategory":"Login Credentials","data":"The holder of this token shall be granted access to the BitStrap.co account services with which this token is associated.","url":"http://BitStrap.co","divisible":false},{"propertyid":25,"name":"CRYPTO NEXT COIN (CXC)","category":"Information and communication","subcategory":"Information service activities","data":"CXC are prepaid vouchers to pay for fees and commissions charged by Crypto Next plc. See website and videos for full explanation.","url":"http://www.cryptonext.net","divisible":false},{"propertyid":26,"name":"coinpowers","category":"crowdfund","subcategory":"crowdfunding","data":"nakamoto","url":"coinpowers.com","divisible":true},{"propertyid":2147483651,"name":"Test Property 1","category":"Testing","subcategory":"Testing Smart Property","data":"Test Da","url":"mastercoinfoundation.org","divisible":false},{"propertyid":2147483652,"name":"MCHEST Test Property","category":"MCHEST Testing","subcategory":"Fundraiser Testing","data":"","url":"masterchest.info/spwd.htm","divisible":false},{"propertyid":2147483653,"name":"Mastercoin Faucet Promo Coupon","category":"Coupons, Gifts","subcategory":"Web","data":"Each promo coupon allows one free redemption on the Mastercoin faucet.","url":"mastercoin-faucet.com/redeem-coupon","divisible":false},{"propertyid":2147483654,"name":"Test Property 2","category":"Testing#2","subcategory":"Testing Smart Property#2","data":"Test Data","url":"mastercoinfoundation.org","divisible":false},{"propertyid":2147483655,"name":"Test Property Fundraiser 2","category":"Testing Fundraiser 2","subcategory":"Testing Smart Property Fundraiser 2","data":"Test Data","url":"mastercoinfoundation.org","divisible":false},{"propertyid":2147483656,"name":"Fundraiser 3 Indiv","category":"Test","subcategory":"Test","data":"Test","url":"abc.com","divisible":false},{"propertyid":2147483657,"name":"Fundraiser 3 Div","category":"Test","subcategory":"Test","data":"n/a","url":"n/a","divisible":true},{"propertyid":2147483658,"name":"Doubloons","category":"Testing","subcategory":"Smart Property Test Sequence 1","data":"Test Issuing a new Currency","url":"https://docs.google.com/a/engine.co/spreadsheet/ccc?key=0Al4FhV693WqWdGNFRDhEMTFtaWNmcVNObFlVQmNOa1E&usp=drive_web#gid=0","divisible":true},{"propertyid":2147483659,"name":"New Doubloons","category":"Testing","subcategory":"Smart Property Test Sequence 1","data":"Test Issuing a new Currency","url":"https://docs.google.com/a/engine.co/spreadsheet/ccc?key=0Al4FhV693WqWdGNFRDhEMTFtaWNmcVNObFlVQmNOa1E&usp=drive_web#gid=0","divisible":true},{"propertyid":2147483660,"name":"Ducats","category":"Testing","subcategory":"Smart Property Test Sequence 1","data":"Test issuing a new crowdsale","url":"https://docs.google.com/a/engine.co/spreadsheet/ccc?key=0Al4FhV693WqWdDZ2R2tfakJKOGF4VThNSmNOTjR5YlE&usp=drive_web#gid=0","divisible":true},{"propertyid":2147483661,"name":"AdamsCoin","category":"Testing","subcategory":"SubTesting Smart Property","data":"The Answer is 42","url":"http://snipurl.com/adamscoin","divisible":false},{"propertyid":2147483662,"name":"Test\u00B8","category":"Test\u00B8","subcategory":"Test\u00B8","data":"n/a","url":"n/a","divisible":false},{"propertyid":2147483663,"name":"Test\u00B8\u00B8\u00B8\u00B8\u00B8\u00B8","category":"Test\u00B8\u00B8\u00B8\u00B8\u00B8\u00B8","subcategory":"Test\u00B8\u00B8\u00B8\u00B8\u00B8\u00B8","data":"n/a","url":"n/a","divisible":false},{"propertyid":2147483664,"name":"DemoCoin1","category":"Testing","subcategory":"Crowdsale Demo","data":"Test issuing a new crowdsale","url":"http://mastercoin.org","divisible":true},{"propertyid":2147483665,"name":"TMSAFE","category":"Test","subcategory":"Test MaidSafeCoin","data":"Issuing Test MaidSafeCoin","url":"www.buysafecoin.io","divisible":true},{"propertyid":2147483666,"name":"TMSAFE3","category":"Test","subcategory":"Test MaidSafeCoin 3","data":"Issuing Test MaidSafeCoin","url":"www.buysafecoin.io","divisible":false},{"propertyid":2147483667,"name":"MSAFE-Test","category":"Testing","subcategory":"DryRun","data":"TestDistribution","url":"","divisible":false},{"propertyid":2147483668,"name":"MCHEST Div Test Property","category":"MCHEST Div Testing","subcategory":"Testing","data":"","url":"masterchest.info/spwd.htm","divisible":true},{"propertyid":2147483669,"name":"Test ANCoin","category":"Tangible Property","subcategory":"Renewable Biofuel","data":"Test Coin Issuance","url":"http://www.arterranrenewables.com","divisible":false},{"propertyid":2147483670,"name":"testZOOZ_v1","category":"Testing","subcategory":"Testing ZOOZ Smart Property","data":"Test Data","url":"www.lazooz.org","divisible":false},{"propertyid":2147483671,"name":"1","category":"1","subcategory":"1","data":"","url":"http://1.com","divisible":false},{"propertyid":2147483672,"name":"WhiteHats","category":"Hacking","subcategory":"Ethical Hacking","data":"","url":"http://www.whitehats.com","divisible":false},{"propertyid":2147483673,"name":"BlackHats","category":"Hacking","subcategory":"Ethical Hacking","data":"","url":"http://www.blackhats.com","divisible":false},{"propertyid":2147483674,"name":"RepRapParts","category":"Crafting","subcategory":"3D Printing","data":"","url":"http://www.reprap.org","divisible":true},{"propertyid":2147483675,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483676,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483677,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483678,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483679,"name":"OmpaLoompa","category":"Testing","subcategory":"Crowdsale Demo","data":"This awesome crowdsale supports multiple currencies!","url":"http://www.themulticurrencycrowdsale.com","divisible":true},{"propertyid":2147483680,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483681,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483682,"name":"Elirans","category":"Testing","subcategory":"Eliran test number1","data":"Testing stuff","url":"https://www.bitgo.co.il","divisible":true},{"propertyid":2147483683,"name":"fds","category":"fsdf","subcategory":"fds","data":"fs","url":"fsd","divisible":true},{"propertyid":2147483684,"name":"AllHands","category":"Test","subcategory":"Test","data":"test all hands","url":"http://www.allhands.com","divisible":true},{"propertyid":2147483685,"name":"Vend1000","category":"Vend1000","subcategory":"Vend1000","data":"Vend1000","url":"Vend1000","divisible":true},{"propertyid":2147483686,"name":"Malkavians","category":"Activities of extraterritorial organizations and bodies","subcategory":"Other","data":"Nothing is truth, everything is permitted.","url":"http://www.malkav.com","divisible":true},{"propertyid":2147483687,"name":"ExploitCoin","category":"Hax0rs","subcategory":"Crackz0rs","data":"';alert(String.fromCharCode(88,83,83))//';alert(String.fromCharCode(88,83,83))//\";alert(String.fromCharCode(88,83,83))//\";alert(String.fromCharCode(88,83,83))//--></SCRIPT>\">'><SCRIPT>alert(String.fromCharCode(88,83,83))</SCRIPT>","url":"http://hax.prototypic.net","divisible":true},{"propertyid":2147483688,"name":"XSSCoin","category":"Hacks","subcategory":"Slashes","data":"Simple XSS Test","url":"http://YouHave.com","divisible":true},{"propertyid":2147483689,"name":"1","category":"1","subcategory":"2","data":"asd","url":"http://www.go.com","divisible":true},{"propertyid":2147483690,"name":"1","category":"1","subcategory":"2","data":"asd","url":"http://www.go.com","divisible":true},{"propertyid":2147483691,"name":"JSUrlInject","category":"a","subcategory":"b","data":"123","url":"http://YouHave\" onclick=alert(String.fromCharCode(88,83,83)) ignoreme=BeenHacked\"","divisible":true},{"propertyid":2147483692,"name":"testcoms","category":"a","subcategory":"a","data":"a","url":"http://a.com","divisible":true},{"propertyid":2147483693,"name":"a","category":"a","subcategory":"a","data":"a","url":"http://a.com","divisible":true},{"propertyid":2147483694,"name":"FooCoin","category":"Accommodation and food service activities","subcategory":"Accommodation","data":"foo","url":"http://omniwallet.org","divisible":true},{"propertyid":2147483695,"name":"31337Coin","category":"Original","subcategory":"All New","data":"123","url":"http://giftcoin.coin.prototypic.net","divisible":true},{"propertyid":2147483696,"name":"CrowdCoin","category":"CoinCoin","subcategory":"CoinCoin","data":"idk","url":"http://allthecoin.us","divisible":true},{"propertyid":2147483697,"name":"BomComs","category":"1","subcategory":"1","data":"1","url":"http://.com","divisible":true},{"propertyid":2147483698,"name":"BomComs","category":"1","subcategory":"1","data":"1","url":"http://.com","divisible":true},{"propertyid":2147483699,"name":"1","category":"1","subcategory":"1","data":"1","url":"http://com","divisible":true},{"propertyid":2147483700,"name":"suprcoms","category":"1","subcategory":"1","data":"1","url":"http://com","divisible":true},{"propertyid":2147483701,"name":"SunCoin","category":"Celestial Bodies","subcategory":"Stars","data":"The sun is mine, all mine!","url":"http://en.wikipedia.org/wiki/Sun","divisible":true},{"propertyid":2147483702,"name":"Globulus","category":"Human health and social work activities","subcategory":"Human health activities","data":"","url":"","divisible":false},{"propertyid":2147483703,"name":"WeeCoin","category":"Information and communication","subcategory":"Computer programming, consultancy and related activities","data":"woo wee","url":"http://wee.woo.com","divisible":true},{"propertyid":2147483704,"name":"ShoePhone","category":"Information and communication","subcategory":"Telecommunications","data":"Would you believe ...","url":"http://en.wikipedia.org/wiki/Shoe_phone","divisible":false},{"propertyid":2147483705,"name":"OneCoin","category":"Other","subcategory":"Other","data":"One is the loneliest number.","url":"","divisible":false},{"propertyid":2147483706,"name":"MaxDivs","category":"Other","subcategory":"Other","data":"92,233,720,368.54775807 coins","url":"","divisible":true},{"propertyid":2147483707,"name":"MaxInDivs","category":"Other","subcategory":"Other","data":"9,223,372,036,854,775,807 tokens","url":"","divisible":false},{"propertyid":2147483708,"name":"TMD coin","category":"Education","subcategory":"","data":"","url":"","divisible":true},{"propertyid":2147483709,"name":"Bitcoin","category":"Administrative and support service activities","subcategory":"Dupe","data":"Coin to trick people","url":"http://allthecoin.us","divisible":true},{"propertyid":2147483710,"name":"mithradites","category":"Information and communication","subcategory":"Information service activities","data":"A coin to reward social media content generators","url":"http://www.bitcoin.com","divisible":true},{"propertyid":2147483711,"name":"Mithradites","category":"Information and communication","subcategory":"Information service activities","data":"Coin for rewarding/tipping people who create content for Social Media","url":"http://bitcoin.org","divisible":true},{"propertyid":2147483712,"name":"SpaceLite Life Sciences R&D","category":"Professional, scientific and technical activities","subcategory":"Scientific research and development","data":"Each SLC is backed by 100 grams of SpaceLite, a highly medicinal potassium-oxygen based alkaline electrolyte mineral for human consumption. SpaceLite is a Life Sciences company that continues to R&D natural life extension nutrients & technologies.","url":"http://spacelite.info","divisible":true},{"propertyid":2147483713,"name":"hawk-a-loogey-coin","category":"~","subcategory":"1","data":"T","url":"g","divisible":true},{"propertyid":2147483714,"name":"hawk-a-loogey-coin-indiv","category":"~","subcategory":"1","data":"T","url":"g","divisible":false}],"error":null,"id":"1"}
```

###Retrieving information for a Master Protocol crowdsale
The **getcrowdsale_MP** call allows for retrieving information for a crowdsale on the Master Protocol.

**Required Parameters**
- **_property ID (integer):_** A valid Master Protocol property ID

**Additional Optional Parameters**
- **_verbose (boolean):_** Bool to determine whether to display crowdsale participant transactions

**Examples**
```
$src/mastercored getcrowdsale_MP 2147483657 true 
{
    "name" : "Fundraiser 3 Div",
    "active" : true,
    "issuer" : "1DYb5Njvcgovt9gUMdMgYkpaQjAEdUooon",
    "propertyiddesired" : 2,
    "tokensperunit" : "0.00001000",
    "earlybonus" : 0,
    "percenttoissuer" : 0,
    "starttime" : 1397243096,
    "deadline" : 1649030400,
    "amountraised" : "32.29930502",
    "tokensissued" : "0.00032299",
    "closedearly"  : false,
    "participanttransactions" : [
        {
            "txid" : "139e476fb34921f7cad834a2868dc4a734cd03a1e6b0a5eb03164e228e28e30d",
            "amountsent" : "1.29900000",
            "participanttokens" : "0.00001299",
            "issuertokens" : "0.00000000"
        },
        {
            "txid" : "3995f9ff3400ef7274b2224e2f584261a70763ed0d03a5e125438cb46771f80d",
            "amountsent" : "0.00030000",
            "participanttokens" : "0.00000000",
            "issuertokens" : "0.00000000"
        },
        {
            "txid" : "5136b5a2036d66beb0f0f260aabba09b8e0d64a8913bc49aad79f8fe43c04ba5",
            "amountsent" : "0.00000001",
            "participanttokens" : "0.00000000",
            "issuertokens" : "0.00000000"
        },
        {
            "txid" : "72532216c9f43ce11a410b82f431fbf0613abef819f7ac58054396db81c8b38d",
            "amountsent" : "1.00000000",
            "participanttokens" : "0.00001000",
            "issuertokens" : "0.00000000"
        },
        {
            "txid" : "85b1d028aee21f68f65f778341b4aca7e2921858acb14c2b2299e69a0307dab2",
            "amountsent" : "30.00000000",
            "participanttokens" : "0.00030000",
            "issuertokens" : "0.00000000"
        },
        {
            "txid" : "d90e89d1a5487d0abaebb70d3baeec6eafb8b62380f639d999bd3f3d6a4433f8",
            "amountsent" : "0.00000501",
            "participanttokens" : "0.00000000",
            "issuertokens" : "0.00000000"
        }
    ]
}
```
```
{"jsonrpc":"1.0","id":"1","method":"getcrowdsale_MP","params":[2147483657,true]}
{"result":{"name":"Fundraiser 3 Div","active":true,"issuer":"1DYb5Njvcgovt9gUMdMgYkpaQjAEdUooon","propertyiddesired":2,"tokensperunit":"0.00001000","earlybonus":0,"percenttoissuer":0,"starttime":1397243096,"deadline":1649030400,"amountraised":"32.29930502","tokensissued":"0.00032299","participanttransactions":[{"txid":"139e476fb34921f7cad834a2868dc4a734cd03a1e6b0a5eb03164e228e28e30d","amountsent":"1.29900000","participanttokens":"0.00001299","issuertokens":"0.00000000"},{"txid":"3995f9ff3400ef7274b2224e2f584261a70763ed0d03a5e125438cb46771f80d","amountsent":"0.00030000","participanttokens":"0.00000000","issuertokens":"0.00000000"},{"txid":"5136b5a2036d66beb0f0f260aabba09b8e0d64a8913bc49aad79f8fe43c04ba5","amountsent":"0.00000001","participanttokens":"0.00000000","issuertokens":"0.00000000"},{"txid":"72532216c9f43ce11a410b82f431fbf0613abef819f7ac58054396db81c8b38d","amountsent":"1.00000000","participanttokens":"0.00001000","issuertokens":"0.00000000"},{"txid":"85b1d028aee21f68f65f778341b4aca7e2921858acb14c2b2299e69a0307dab2","amountsent":"30.00000000","participanttokens":"0.00030000","issuertokens":"0.00000000"},{"txid":"d90e89d1a5487d0abaebb70d3baeec6eafb8b62380f639d999bd3f3d6a4433f8","amountsent":"0.00000501","participanttokens":"0.00000000","issuertokens":"0.00000000"}]},"error":null,"id":"1"}
```

###Listing currently active crowdsales
The **getactivecrowdsales_MP** call allows for listing all currently active Master Protocol crowdsales.

**Required Parameters**
- There are no required parameters for this call.

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored getactivecrowdsales_MP
[
    {
        "propertyid" : 2147483704,
        "name" : "ShoePhone",
        "issuer" : "12vPM3chMgXBoyi5tWSjCcUBQvdPxd4QxH",
        "propertyiddesired" : 2,
        "tokensperunit" : "8686",
        "earlybonus" : 28,
        "percenttoissuer" : 68,
        "starttime" : 1405022172,
        "deadline" : 1406925000000
    },
    {
        "propertyid" : 2147483679,
        "name" : "OmpaLoompa",
        "issuer" : "18eVnuXEixXuVt248a5i18eLnkKbfmpUWk",
        "propertyiddesired" : 2,
        "tokensperunit" : "0.00000100",
        "earlybonus" : 25,
        "percenttoissuer" : 10,
        "starttime" : 1403303662,
        "deadline" : 22453142409904
    },
    {
        "propertyid" : 2147483699,
        "name" : "1",
        "issuer" : "1CTmPpXF89LMNHwdizPXxSLd34uRok133V",
        "propertyiddesired" : 2147483692,
        "tokensperunit" : "3.00000000",
        "earlybonus" : 1,
        "percenttoissuer" : 1,
        "starttime" : 1404417063,
        "deadline" : 1407076800000
    },
    ........
]
```
```
{"jsonrpc":"1.0","id":"1","method":"getactivecrowdsales_MP","params":[]}
{"result":[{"propertyid":2147483704,"name":"ShoePhone","issuer":"12vPM3chMgXBoyi5tWSjCcUBQvdPxd4QxH","propertyiddesired":2,"tokensperunit":"8686","earlybonus":28,"percenttoissuer":68,"starttime":1405022172,"deadline":1406925000000},{"propertyid":2147483679,"name":"OmpaLoompa","issuer":"18eVnuXEixXuVt248a5i18eLnkKbfmpUWk","propertyiddesired":2,"tokensperunit":"0.00000100","earlybonus":25,"percenttoissuer":10,"starttime":1403303662,"deadline":22453142409904},{"propertyid":2147483699,"name":"1","issuer":"1CTmPpXF89LMNHwdizPXxSLd34uRok133V","propertyiddesired":2147483692,"tokensperunit":"3.00000000","earlybonus":1,"percenttoissuer":1,"starttime":1404417063,"deadline":1407076800000},{"propertyid":2147483696,"name":"CrowdCoin","issuer":"1CXnRrBm3NezYeAcu555Ubu7swwKsn26dT","propertyiddesired":2,"tokensperunit":"31337.00000000","earlybonus":6,"percenttoissuer":10,"starttime":1404401559,"deadline":1407064860000},{"propertyid":2147483657,"name":"Fundraiser 3 Div","issuer":"1DYb5Njvcgovt9gUMdMgYkpaQjAEdUooon","propertyiddesired":2,"tokensperunit":"0.00001000","earlybonus":0,"percenttoissuer":0,"starttime":1397243096,"deadline":1649030400},{"propertyid":2147483703,"name":"WeeCoin","issuer":"1KVhsbJ4pBgZVSUYUVZUJ2xAMMbt8eY8vU","propertyiddesired":2,"tokensperunit":"25.00000000","earlybonus":2,"percenttoissuer":10,"starttime":1405016346,"deadline":1406987700000},{"propertyid":2147483712,"name":"SpaceLite Life Sciences R&D","issuer":"1MatrixoUT68b6mWwyRSpL6uVPpfwWhB9R","propertyiddesired":2147483707,"tokensperunit":"7.00000000","earlybonus":5,"percenttoissuer":100,"starttime":1405500631,"deadline":1409940240000},{"propertyid":2147483693,"name":"a","issuer":"1PVWtK1ATnvbRaRceLRH5xj8XV1LxUBu7n","propertyiddesired":2147483671,"tokensperunit":"5.00000000","earlybonus":1,"percenttoissuer":1,"starttime":1404169502,"deadline":1406696400000},{"propertyid":2147483710,"name":"mithradites","issuer":"1PfREWL44zJun1MLXkH64s88DSkPZXVxot","propertyiddesired":2,"tokensperunit":"21000000.00000000","earlybonus":6,"percenttoissuer":20,"starttime":1405228233,"deadline":1407888180000}],"error":null,"id":"1"}
```

###Broadcasting a Send to Owners transaction
The **sendtoowners_MP** call allows for broadcast of a Send to Owners transaction.

**Required Parameters**
- **_sender address (string):_** A valid bitcoin address containing a sufficient balance to support the transaction
- **_currency/property ID (integer):_** A valid Master Protocol currency/property ID
- **_amount (string):_** The amount to issue to owners (note if sending individisble tokens any decimals will be truncated)

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

**Examples**
```
$src/mastercored sendtoowners_MP "1MCHESTxYkPSLoJ57WBQot7vz3xkNahkcb" 2147483668 "5"
3fba3995f679f764efbdfc800331a8dc333211cf50291072c3c2dfcb1569c3d8
```
```
{"jsonrpc":"1.0","id":"1","method":"sendtoowners_MP","params":[""1MCHESTxYkPSLoJ57WBQot7vz3xkNahkcb",2147483668,"5"]}
{"result":"3fba3995f679f764efbdfc800331a8dc333211cf50291072c3c2dfcb1569c3d8","error":null,"id":"1"}
```
*Please note, the private key for the requested sender address must be available in the wallet.*

###Retrieving details for a Send To Owners transaction
The **getsto_MP** call allows for retrieval of details for a Send To Owners transaction including the recipients and their respective amounts.  Please note, by default the call will only list recipients in the wallet.  To list all recipients you may use the filter ```"*"```.

**Required Parameters**
- **_transaction ID (string):_** A valid Master Protocol Send To Owners transaction ID

**Additional Optional Parameters**
- **_address (string):_** A valid bitcoin address to filter returned recipients on or * for all recipients

**Examples**
```
$src/mastercored getsto_MP 61f161023f11dae651d739bded05805356ed2f6dc136398dc3878b425ef73c62
{
    "txid" : "61f161023f11dae651d739bded05805356ed2f6dc136398dc3878b425ef73c62",
    "sendingaddress" : "1PVWtK1ATnvbRaRceLRH5xj8XV1LxUBu7n",
    "ismine" : false,
    "confirmations" : 29534,
    "fee" : 0.00010000,
    "blocktime" : 1406079768,
    "version" : 0,
    "type_int" : 3,
    "type" : "Send To Owners",
    "propertyid" : 2,
    "divisible" : true,
    "amount" : "1.00000000",
    "valid" : true,
    "totalstofee" : "0.00001291",
    "recipients" : [
    ]
}
```
```
{"jsonrpc":"1.0","id":"1","method":"getsto_MP","params":["61f161023f11dae651d739bded05805356ed2f6dc136398dc3878b425ef73c62"]}
{"result":{"txid":"61f161023f11dae651d739bded05805356ed2f6dc136398dc3878b425ef73c62","sendingaddress":"1PVWtK1ATnvbRaRceLRH5xj8XV1LxUBu7n","ismine":false,"confirmations":29535,"fee":0.00010000,"blocktime":1406079768,"version":0,"type_int":3,"type":"Send To Owners","propertyid":2,"divisible":true,"amount":"1.00000000","valid":true,"totalstofee":"0.00001291","recipients":[]},"error":null,"id":"1"}
```

###Listing currently active DeX sell offers
The **getactivedexsells_MP** call allows for listing all currently active Master Protocol DeX sell offers.

**Required Parameters**
- There are no required parameters for this call.

**Additional Optional Parameters**
- You may pass in (1) parameter, which is the address to filter by/on.

**Examples**
```
$src/mastercored getactivedexsells_MP
[
    {
        "txid" : "9133dc9f042a4edd92a8bc1f1764a9cc5e0b1f04061ccb74e5fce34981f664a1",
        "propertyid" : 1,
        "seller" : "1Ji1E3r4paY8ZqtudraKYvc7cfN1fyEfNo",
        "amountavailable" : "0.01000000",
        "bitcoindesired" : "0.00200000",
        "unitprice" : "0.20000000",
        "timelimit" : 6,
        "minimumfee" : "0.00010000",
        "amountaccepted" : "0.00000000",
        "accepts" : [
        ]
    },
    {
        "txid" : "28d951ac4b48c31706561dac62fd0ea065a7873faede5316ff04817e1057315f",
        "propertyid" : 1,
        "seller" : "1K5Tofy7UTfcrpWBnXcJhHZzvLTksDdasQ",
        "amountavailable" : "0.00000001",
        "bitcoindesired" : "0.00000000",
        "unitprice" : "0.12000000",
        "timelimit" : 100,
        "minimumfee" : "0.00010000",
        "amountaccepted" : "0.00000000",
        "accepts" : [
        ]
    }
]
```

###Get grant/revoke info for a property ID
The **getgrants_MP** call allows for grant/revoke retreival of a property ID.

**Required Parameters**
- **_currency/property ID (integer):_** A valid Master Protocol currency/property ID

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

Result: A list of revoke/grant transactions for a propertyID.


###Return all Master Protocol transactions in a block
The **listblocktransactions_MP** call allows one to view Master Protocol transactions in a block.

**Required Parameters**
- **_block height/index (integer):_** A valid blockheight or index

**Additional Optional Parameters**
- There are currently no supported optional parameters for this call.

Result: A list of Master Protocol txids
