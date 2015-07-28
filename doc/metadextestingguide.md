# Omni Core Testing Guide

Please note this guide utilizes the testnet network.  All tokens such as MSC will be testnet tokens and carry no value.

### Step 1

Download the Omni Core preview build for your operating system from the below links and either extract or install it.

http://omnichest.info/files/omnicore-0.0.9.99-dev-win64.zip

http://omnichest.info/files/omnicore-0.0.9.99-dev-win64-setup.exe

http://omnichest.info/files/omnicore-0.0.9.99-dev-osx64.tar.gz

http://omnichest.info/files/omnicore-0.0.9.99-dev-osx-unsigned.dmg

http://omnichest.info/files/omnicore-0.0.9.99-dev-linux64.tar.gz

### Step 2

Run Omni Core QT on testnet.  To do so, execute with the --testnet startup parameter.

**Windows:**

Create a shortcut to omnicore-qt.exe and edit the properties to append ```--testnet``` to the target field

![alt tag](http://i.imgur.com/NJ5TuzN.png)

Alternatively, open the command prompt, change to the directory containing the Omni Core binaries and execute:

```omnicore-qt.exe --testnet```

**Linux:**

Open a terminal, change to the directory containing the Omni core binaries and execute:

```omnicore-qt --testnet```

**OSX:** 

Open the terminal, change to the directory containing the Omni Core binaries and execute:

```open -a "omnicore-qt.app" --args -testnet```

### Step 3

Wait for the Omni Core client to sync the testnet blockchain.

![alt tag](http://i.imgur.com/RLtEfCU.png)

### Step 4

Obtain a testnet bitcoin address for your wallet by selecting File > Receiving Addresses

![alt tag](http://i.imgur.com/vZxf0pL.png)

### Step 5

Use a testnet faucet to obtain some testnet BTC.  Any of the following should be suitable:

http://kuttler.eu/bitcoin/btc/faucet/

https://testnet.coinfaucet.eu/en/

https://accounts.blockcypher.com/testnet-faucet

If you have difficulty using these faucets you can find a list of additional faucets at the Bitcoin wiki (https://en.bitcoin.it/wiki/Testnet#Faucets).

### Step 6

Once you have received your testnet bitcoins, you can convert some of them into Testnet MSC by sending them to the "moneyman" address.  This address runs a perpetual Exodus crowdsale for testing purposes.

1. Click the 'Send' page, followed by the 'Bitcoin' tab.
2. Enter the following address in the 'Pay To' field:  ```moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP```
3. Enter the amount of testnet BTC you would like to convert to testnet MSC
4. Use the 'Send' button to broadcast the transaction to the testnet network.

![alt tag](http://i.imgur.com/PnLyAYe.png)

Wait for the transaction to confirm (you can use the 'Transactions' page, 'Bitcoin' tab to check the confirmation status of the transaction.)

![alt tag](http://i.imgur.com/j62Cnig.png)

### Step 7

There is a known bug in this preview build, where Exodus crowdsale purchases do not trigger an update to the UI.  Close down and reopen the client to refresh your testnet MSC balance after sending funds to the 'moneyman' address.

Note this bug will be fixed in a subsequent build.

### Step 8

To test trading on the MetaDEx, switch to the Exchange tab.  

It is recommended that you test trading via properties #12 (divisible) and #13 (indivisible) as there should be existing sell offers to trade against, though you may of course trade against any property on the testnet network or create your own token via the RPC layer if you so desire.

To trade, simply select the address you wish to use and either click an existing open trade to automatically populate the amount & price fields or enter your chosen values manually.  Finally click the buy or sell buttons accordingly.

![alt tag](http://i.imgur.com/G5dQS4E.png)
