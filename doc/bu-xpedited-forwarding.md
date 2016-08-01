How to setup and get the best results with Xpedited Forwarding
==============================================================


1. What is Xpedited Forwarding
---------------------------------

Xpedited Forwarding is a decentralised rapid block propagation system designed to replace the centralised Relay Network.  Anyone
can participate in or easily setup their own Xpedited Network.

The forwarding mechanism while simple to understand uses several enhancements to speed the delivery of newly mined blocks
throughout the world and most importantly to other miners.  Essentially when one participates in the network, as soon as a new block 
is found, and Xthinblock is fowarded to every node you are connected to that has requested expedited forwarding.  When they recieved 
the Xthin, the header is quickly checked to make sure it is valid and then forwarded to the next set of nodes and so on.

It's important to note that only after the Xthin is forwarded is the block verfied and accepted.  This allows for a much faster
propagation through the network.


2a. Setting up an Xpedited connection with addnode
--------------------------------------------------

There are two ways to setup connections.  You can use `addnode` or RPC, however with addnode the setup will persist when your node 
is recycled and is the recommended setup.

Two bitcoin.conf entries need to be made.  One is the addnode connection which opens the connection to the remote peer and the 
second is the `expeditedblock` entry which makes the request to begin the forwarding of xpedited blocks.

	addnode=<ip:port>
	expeditedblock=<ip:port>

You can have muliple `addnode` and expedited entries up to a maximum of 8 connections.  See the following example:

	addnode=10.233.34.33:8333
	expeditedblock=10.233.34.33:8333
	addnode=11.222.34.55:10500
	expeditedblock=11.222.34.55:10500
	addnode=45.33.223.34
	expeditedblock=45.33.223.34
	addnode=45.33.233.45:8333
	expeditedblock=45.33.233.45:8333
	addnode=42.33.233.45:8333
	expeditedblock=42.33.233.45:8333
	addnode=15.33.233.45:8333
	expeditedblock=15.33.233.45:8333
	addnode=45.33.22.45:8333
	expeditedblock=45.33.22.45:8333
	addnode=145.33.233.45:8333
	expeditedblock=145.33.233.45:8333


2b. Setting up an Xpedited connection with RPC
----------------------------------------------

You can also setup connections using the RPC interface, however once you restart your node the xpedited forwarding will no longer be in effect
and you will have to re-request forwarding unless you use a script of some kind to send the forwarding requests.

Using RPC is simple as follows:

	expedited block|tx "node IP addr" on|off

For example, to turn on expedited forwarding of blocks from a remote peer to my peer you would enter the following:

	bitcoin-cli expedited block "192.168.0.6:8333" on

and to do the same for transaction fowarding I would enter:

	bitcoin-cli expedited tx "192.168.0.6:8333" on


To get more help on how to use the RPC commands you can type in "bitcoin-cli help expedited" from the command line, or if you ar using the debug console
then simply type "help expedited".


3. Putting limits on forwarding
-------------------------------

By default the maximum number of recipients that can request forwarding from a node is set to 3.  If you wish to limit that or reduce that
you can uses the following settings.

	maxexpeditedblockrecipients=<n>
	maxexpeditedtxrecipients=<n>


