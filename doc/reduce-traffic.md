Reduce Traffic
==============

Some node operators need to deal with bandwidth caps imposed by their ISPs.

By default, Dash Core allows up to 125 connections to different peers, 8 of
which are outbound. You can therefore, have at most 117 inbound connections.

The default settings can result in relatively significant traffic consumption.

Ways to reduce traffic:

## 1. Use `-maxuploadtarget=<MiB per day>`

A major component of the traffic is caused by serving historic blocks to other nodes
during the initial blocks download phase (syncing up a new node).
This option can be specified in MiB per day and is turned off by default.
This is *not* a hard limit; only a threshold to minimize the outbound
traffic. When the limit is about to be reached, the uploaded data is cut by no
longer serving historic blocks (blocks older than one week).
Keep in mind that new nodes require other nodes that are willing to serve
historic blocks. **The recommended minimum is 144 blocks per day (max. 144MB
per day)**

Whitelisted peers will never be disconnected, although their traffic counts for
calculating the target.

## 2. Disable "listening" (`-listen=0`)

Disabling listening will result in fewer nodes connected (remember the maximum of 8
outbound peers). Fewer nodes will result in less traffic usage as you are relaying
blocks and transactions to fewer nodes.

## 3. Reduce maximum connections (`-maxconnections=<num>`)

Reducing the maximum connected nodes to a minimum could be desirable if traffic
limits are tiny. Keep in mind that dash's trustless model works best if you are
connected to a handful of nodes.
