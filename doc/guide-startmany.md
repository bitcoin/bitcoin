#start-many Setup Guide

## Two Options for Setting up your Wallet
There are many ways to setup a wallet to support start-many. This guide will walk through two of them.

1. [Importing an existing wallet (recommended if you are consolidating wallets).](#option1)
2. [Sending 1,000 CRW to new wallet addresses.](#option2)

## <a name="option1"></a>Option 1. Importing an existing wallet

This is the way to go if you are consolidating multiple wallets into one that supports start-many. 

### From your single-instance ThroNe Wallet

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Dump the private key from your ThroNe's pulic key.

```
walletpassphrase [your_wallet_passphrase] 600
dumpprivkey [mn_public_key]
```

Copy the resulting priviate key. You'll use it in the next step.

### From your multi-instance ThroNe Wallet

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Import the private key from the step above.

```
walletpassphrase [your_wallet_passphrase] 600
importprivkey [single_instance_private_key]
```

The wallet will re-scan and you will see your available balance increase by the amount that was in the imported wallet.

[Skip Option 2. and go to Create throne.conf file](#throneconf)

## <a name="option2"></a>Option 2. Starting with a new wallet

[If you used Option 1 above, then you can skip down to Create throne.conf file.](#throneconf)

### Create New Wallet Addresses

1. Open the QT Wallet.
2. Click the Receive tab.
3. Fill in the form to request a payment.
    * Label: mn01
    * Amount: 1000 (optional)
    * Click *Request payment*
5. Click the *Copy Address* button

Create a new wallet address for each ThroNe.

Close your QT Wallet.

### Send 1,000 CRW to New Addresses

Just like setting up a standard MN. Send exactly 1,000 CRW to each new address created above.

### Create New Throne Private Keys

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Issue the following:

```throne genkey```

*Note: A throne private key will need to be created for each ThroNe you run. You should not use the same throne private key for multiple ThroNes.*

Close your QT Wallet.

## <a name="throneconf"></a>Create throne.conf file

Remember... this is local. Make sure your QT is not running.

Create the throne.conf file in the same directory as your wallet.dat.

Copy the throne private key and correspondig collateral output transaction that holds the 1K CRW.

The throne private key may be an existing key from [Option 1](#option1), or a newly generated key from [Option 2](#option2). 

*Please note, the throne priviate key is not the same as a wallet private key. Never put your wallet private key in the throne.conf file. That is equivalent to putting your 1,000 CRW on the remote server and defeats the purpose of a hot/cold setup.*

### Get the collateral output

Open your QT Wallet and go to console (from the menu select Tools => Debug Console)

Issue the following:

```throne outputs```

Make note of the hash (which is your collaterla_output) and index.

### Enter your ThroNe details into your throne.conf file
[From the dash github repo](https://github.com/darkcoin/darkcoin/blob/master/doc/throne_conf.md)

The new throne.conf format consists of a space seperated text file. Each line consisting of an alias, IP address followed by port, throne private key, collateral output transaction id and collateral output index, donation address and donation percentage (the latter two are optional and should be in format "address:percentage").

```
alias ipaddress:port throne_private_key collateral_output collateral_output_index donationin_address:donation_percentage
```



Example:

```
mn01 127.0.0.1:9340 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn02 127.0.0.2:9340 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0 7gnwGHt17heGpG9Crfeh4KGpYNFugPhJdh:25
```

## What about the dash.conf file?

If you are using a throne.conf file you no longer need the dash.conf file. The exception is if you need custom settings (thanks oblox). 

## Update dash.conf on server

If you generated a new throne private key, you will need to update the remote dash.conf files.

Shut down the daemon and then edit the file.

```sudo nano .dash/dash.conf```

### Edit the throneprivkey
If you generated a new throne private key, you will need to update the throneprivkey value in your remote dash.conf file.

## Start your ThroNes

### Remote

If your remote server is not running, start your remote daemon as you normally would. 

I usually confirm that remote is on the correct block by issuing:

```crownd getinfo```

And compare with the official explorer at http://explorer.crown.tech/chain/Crown

### Local

Finally... time to start from local.

#### Open up your QT Wallet

From the menu select Tools => Debug Console

If you want to review your throne.conf setting before starting the ThroNes, issue the following in the Debug Console:

```throne list-conf```

Give it the eye-ball test. If satisfied, you can start your nodes one of two ways.

1. throne start-alias [alias_from_throne.conf]. Example ```throne start-alias mn01```
2. throne start-many
