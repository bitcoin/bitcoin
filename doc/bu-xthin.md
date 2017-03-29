How to setup and get the best results with Xthins
==========================================================


1. Turning Thinblocks ON/OFF
---------------------------------

When you run a Bitcoin Unimited node the XTHIN service is turned on by default.  If you wish to turn it off for some reason
you can use the bitcoin.conf setting (0 = off, 1 = on) as follows:

	use-thinblocks=0


2. Using addnode
----------------

Due to the low numbers of nodes that support XTHIN blocks, initially it is a good idea to find a node which is XTHIN capable and
force a connection using either -addnode or -connect-thinblock.

Using `addnode` is the recommended way of connecting and can be made by a simple config entry.

	addnode=<ip:port>

You can have multiple addnode entries up to a maximum of 8 connections (or the number specified via maxoutconnections parameter), so you
might have a config file looking as follows:

	addnode=10.233.34.33:8333
	addnode=11.222.34.55:10500
	addnode=45.33.223.34
	addnode=45.33.233.45:8333

When you use `addnode` your peer upon receiving a new block announcement by an INV message will begin a 10 second timer.  If an INV
message is not received within that 10 seconds from an XTHIN capable node then a request for a full block will be sent out. If it
does receive an INV within the 10 second period then an XTHIN block will be requested.  This process is used as a way to bring a
full block into the BU XTHIN capable network of nodes such that once a peer does get a full block it will then very quickly propagate
a XTHIN to every other peer in the XTHIN capable network.  In such a way we avoid potentially splitting the network while at
the same time maximizing our use of XTHIN technology.


3. When to use connect-thinblock
--------------------------------

the `connect-thinblock` feature is mainly used for the purpose of testing, however in some cases it can be useful for very bandwidth
constrained nodes that *always* want every downloaded block to be an XTHIN.  In such a case you would substitute the addnode entry
above with a connect-thinblockentry.  As with addnode you can have up to 8 `connect-thinblock` entries and your entry would be as
follows:

	connect-thinblock=<ip:port>

One thing to keep in mind is that with `connect-thinblock`, if the nodes that you are connecting to are down or can not service you then
you have the possibility of not receiving any new blocks until those nodes come back on line.  Therefore use `connect-thinblock` with
caution and always use the full 8 connections if you are unsure whether the peers will be online or not.


4. The thinblock mempool limiter
---------------------------------

In order to keep the size of the XTHIN bloom filters from getting too large, a way to limit the size of mempools was implemented in v0.12.1
using a simple rate limiting technique.  If the size of the mempool grows to 3X the size of the largest block seen then two things happen,
the -minrelaytxfee is gradually increased until it reaches the -maxlimitertxfee, and at the same time the -limitfreerelay is gradually
reduced from 150KB per 10 minute interval down to 15KB per 10 minute.  What this system allow us to do is generally allow more zero fee
transactions through the system, yet as the mempool becomes overfull we start to choke off the free transactions while at the same time
still allowing all high priority and coinbase spends through. This mempool limiting technique has shown itself to be a great way of reducing
outbound bandwidth and preventing the propagation of transactions that will never be mined within the default 72 hour window.

During the release of v0.12.1, Bloom Filter targeting was introduced which supersedes the need for the mempool limiter in keeping bloom
filters smaller however it is still a valuable tool in reducing the total amount of outbound bandwidth.

There are two setting one can change as follows:

	maxlimitertxfee : default is 3 satoshi/byte
	minlimitertxfee : default is 0 satoshi/byte

Generally you can just use the defaults, but if you know for instance that an attacker is sending many transaction that are 1.014 sat/byte then you
could set your maxlimitertxfee=1.1 which would effectively choke off the attackers transactions while having the least impact to other transactions
that are likely to be mineable.

	maxlimitertxfee=1.1
	minlimitertxfee=0
