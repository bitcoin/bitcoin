REDUCE TRAFFIC
==============

Some node operators need to deal with bandwith cap given by their ISPs.

By default, bitcoin-core allows up to 125 connections to different peers, 8 of
them outbound (and therefore 117 max inbound connections).

The default settings can result in relatively significant traffic consumption.


Ways to reduce traffic:

1. Use `-maxuploadtarget=<MiB per day>`

A major part of the traffic is caused by serving historic blocks to other nodes
in initial blocks download state (syncing up a new node).
This option can be specified in MiB per day and is turned off by default.
This is *not* a hard limit but a threshold to minimize the outbound
traffic. When the limit is about to be reached, the uploaded data is cut by not
serving historic blocks (blocks older than one week).
Keep in mind that new nodes require other nodes that are willing to serve
historic blocks. **The recommended minimum is 144 blocks per day (max. 144MB
per day)**

2. Disable "listening" (`-listen=0`)

Disable listening will result in fewer nodes connected (remind the max of 8
outbound peers). Fewer nodes will result in less traffic usage because relaying
blocks and transaction needs to be passed to fewer nodes.

3. Reduce maximal connections (`-maxconnections=<num>`)

Reducing the connected nodes to a miniumum can be desired in case traffic
limits are tiny. Keep in mind that bitcoin trustless model works best if you are
connected to a handfull of nodes.
