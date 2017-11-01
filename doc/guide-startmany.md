# start-many Setup Guide

## Setting up your Wallet

### Create New Wallet Addresses

1. Open the QT Wallet.
2. Click the Receive tab.
3. Fill in the form to request a payment.
    * Label: mn01
    * Amount: 1000 (optional)
    * Click *Request payment* button
5. Click the *Copy Address* button

Create a new wallet address for each Masternode.

Close your QT Wallet.

### Send 1000 DASH to New Addresses

Send exactly 1000 DASH to each new address created above.

### Create New Masternode Private Keys

Open your QT Wallet and go to console (from the menu select `Tools` => `Debug Console`)

Issue the following:

```masternode genkey```

*Note: A masternode private key will need to be created for each Masternode you run. You should not use the same masternode private key for multiple Masternodes.*

Close your QT Wallet.

## <a name="masternodeconf"></a>Create masternode.conf file

Remember... this is local. Make sure your QT is not running.

Create the `masternode.conf` file in the same directory as your `wallet.dat`.

Copy the masternode private key and correspondig collateral output transaction that holds the 1000 DASH.

*Note: The masternode priviate key is **not** the same as a wallet private key. **Never** put your wallet private key in the masternode.conf file. That is almost equivalent to putting your 1000 DASH on the remote server and defeats the purpose of a hot/cold setup.*

### Get the collateral output

Open your QT Wallet and go to console (from the menu select `Tools` => `Debug Console`)

Issue the following:

```masternode outputs```

Make note of the hash (which is your collateral_output) and index.

### Enter your Masternode details into your masternode.conf file
[From the dash github repo](https://github.com/dashpay/dash/blob/master/doc/masternode_conf.md)

`masternode.conf` format is a space seperated text file. Each line consisting of an alias, IP address followed by port, masternode private key, collateral output transaction id and collateral output index.

```
alias ipaddress:port masternode_private_key collateral_output collateral_output_index
```

Example:

```
mn01 127.0.0.1:9999 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0
mn02 127.0.0.2:9999 93WaAb3htPJEV8E9aQcN23Jt97bPex7YvWfgMDTUdWJvzmrMqey aa9f1034d973377a5e733272c3d0eced1de22555ad45d6b24abadff8087948d4 0
```

## Update dash.conf on server

If you generated a new masternode private key, you will need to update the remote `dash.conf` files.

Shut down the daemon and then edit the file.

```nano .dashcore/dash.conf```

### Edit the masternodeprivkey
If you generated a new masternode private key, you will need to update the `masternodeprivkey` value in your remote `dash.conf` file.

## Start your Masternodes

### Remote

If your remote server is not running, start your remote daemon as you normally would. 

You can confirm that remote server is on the correct block by issuing

```dash-cli getinfo```

and comparing with the official explorer at https://explorer.dash.org/chain/Dash

### Local

Finally... time to start from local.

#### Open up your QT Wallet

From the menu select `Tools` => `Debug Console`

If you want to review your `masternode.conf` setting before starting Masternodes, issue the following in the Debug Console:

```masternode list-conf```

Give it the eye-ball test. If satisfied, you can start your Masternodes one of two ways.

1. `masternode start-alias [alias_from_masternode.conf]`  
Example ```masternode start-alias mn01```
2. `masternode start-many`

## Verify that Masternodes actually started

### Remote

Issue command `masternode status`
It should return you something like that:
```
dash-cli masternode status
{
    "outpoint" : "<collateral_output>-<collateral_output_index>",
    "service" : "<ipaddress>:<port>",
    "pubkey" : "<1000 DASH address>",
    "status" : "Masternode successfully started"
}
```
Command output should have "_Masternode successfully started_" in its `status` field now. If it says "_not capable_" instead, you should check your config again.

### Local

Search your Masternodes on https://dashninja.pl/masternodes.html

_Hint: Bookmark it, you definitely will be using this site a lot._
