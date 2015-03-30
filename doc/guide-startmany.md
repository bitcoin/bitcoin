#start-many Setup Guide

## Two Options for Setting up your Wallet
There are many ways to setup a wallet to support start-many. This guide will walk through two of them.

1. [Importing an existing wallet (recommended if you are consolidating wallets).](#option1)
2. [Sending 1,000 DASH to new wallet addresses.](#option2)

## <a name="option1"></a>Option 1. Importing an existing wallet

This is the way to go if you are consolidating multiple wallets into one that supports start-many. 

### From your single-instance MasterNode Wallet

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Dump the private key from your MasterNode's pulic key.

```
walletpassphrase [your_wallet_passphrase] 600
dumpprivkey [mn_public_key]
```

Copy the resulting priviate key. You'll use it in the next step.

### From your multi-instance MasterNode Wallet

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Import the private key from the step above.

```
walletpassphrase [your_wallet_passphrase] 600
importprivkey [single_instance_private_key]
```

The wallet will re-scan and you will see your available balance increase by the amount that was in the imported wallet.

[Skip Option 2. and go to Create masternode.conf file](#masternodeconf)

## <a name="option2"></a>Option 2. Starting with a new wallet

[If you used Option 1 above, then you can skip down to Create masternode.conf file.](#masternodeconf)

### Create New Wallet Addresses

1. Open the QT Wallet.
2. Click the Receive tab.
3. Fill in the form to request a payment.
    * Label: mn01
    * Amount: 1000 (optional)
    * Click *Request payment*
5. Click the *Copy Address* button

Create a new wallet address for each MasterNode.

Close your QT Wallet.

### Send 1,000 DASH to New Addresses

Just like setting up a standard MN. Send exactly 1,000 DASH to each new address created above.

### Create New Masternode Private Keys

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Issue the following:

```masternode genkey```

*Note: A masternode private key will need to be created for each MasterNode you run. You should not use the same masternode private key for multiple MasterNodes.*

Close your QT Wallet.

## <a name="masternodeconf"></a>Create masternode.conf file

Remember... this is local. Make sure your QT is not running.

Create the masternode.conf file in the same directory as your wallet.dat.

Copy the masternode private key and correspondig collateral output transaction that holds the 1K DASH.

The masternode private key may be an existing key from [Option 1](#option1), or a newly generated key from [Option 2](#option2). 

*Please note, the masternode priviate key is not the same as a wallet private key. Never put your wallet private key in the masternode.conf file. That is equivalent to putting your 1,000 DASH on the remote server and defeats the purpose of a hot/cold setup.*

### Get the collateral output

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Issue the following:

```masternode outputs```

Make note of the hash (which is your collaterla_output) and index.

### Enter your MasterNode details into your masternode.conf file
[From the dash github repo](https://github.com/darkcoin/darkcoin/blob/master/doc/masternode_conf.md)

The new masternode.conf format consists of a space seperated text file. Each line consisting of an alias, IP address followed by port, masternode private key, collateral output transaction id and collateral output index, donation address and donation percentage (the latter two are optional and should be in format "address:percentage").

```
alias ipaddress:port masternode_private_key collateral_output collateral_output_index donationin_address:donation_percentage
```



Example:

```
mn01 127.0.0.1:9999 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn02 127.0.0.2:9999 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh:25
```

## What about the dash.conf file?

If you are using a masternode.conf file you no longer need the dash.conf file. The exception is if you need custom settings (thanks oblox). 

## Update dash.conf on server

If you generated a new masternode private key, you will need to update the remote dash.conf files.

Shut down the daemon and then edit the file.

```sudo nano .dash/dash.conf```

### Edit the masternodeprivkey
If you generated a new masternode private key, you will need to update the masternodeprivkey value in your remote dash.conf file.

## Start your MasterNodes

### Remote

If your remote server is not running, start your remote daemon as you normally would. 

I usually confirm that remote is on the correct block by issuing:

```dashd getinfo```

And compare with the official explorer at http://explorer.dashpay.io/chain/Dash

### Local

Finally... time to start from local.

#### Open up your QT Wallet

From the menu select Tools => Debug Console

If you want to review your masternode.conf setting before starting the MasterNodes, issue the following in the Debug Console:

```masternode list-conf```

Give it the eye-ball test. If satisfied, you can start your nodes one of two ways.

1. masternode start-alias [alias_from_masternode.conf]. Example ```masternode start-alias mn01```
2. masternode start-many
