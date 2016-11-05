Reduce Traffic
==============

Some node operators need to deal with bandwidth caps imposed by their ISPs.

By default, Bitcoin Unlimited allows up to 125 connections to different peers,
8 of which are outbound. You can therefore, have at most 117 inbound connections.

The default settings can result in relatively significant traffic consumption.

Ways to reduce traffic:

## 1. Use the traffic shaping options

Bitcoin Unlimited adds traffic shaping which is controlled by the following
parameters (these can also be regulated in the GUI):

  -receiveburst: The maximum rate that data can be received in kB/s.
       If there has been a period of lower than average data rates,
       the client may receive extra data to bring the average back to
       '-receiveavg' but the data rate will not exceed this parameter.

  -sendburst: The maximum rate that data can be sent in kB/s.
       If there has been a period of lower than average data rates,
       the client may send extra data to bring the average back to
       '-sendavg' but the data rate will not exceed this parameter.

  -receiveavg: The average rate that data can be received in kB/s

  -sendavg: The maximum rate that data can be sent in kB/s

## 2. Use `-maxuploadtarget=<MiB per day>`

A major component of the traffic is caused by serving historic blocks to other nodes
during the initial blocks download phase (syncing up a new node).
This option can be specified in MiB per day and is turned off by default.
This is *not* a hard limit; only a threshold to minimize the outbound
traffic. When the limit is about to be reached, the uploaded data is cut by no
longer serving historic blocks (blocks older than one week).
Keep in mind that new nodes require other nodes that are willing to serve
historic blocks. **The recommended minimum is 144 * <excessive block size> per day**

Whitelisted peers will never be disconnected, although their traffic counts for
calculating the target.

## 3. Disable "listening" (`-listen=0`)

Disabling listening will result in fewer nodes connected (remember the maximum of 8
outbound peers). Fewer nodes will result in less traffic usage as you are relaying
blocks and transactions to fewer nodes.

## 4. Reduce maximum connections (`-maxconnections=<num>`)

Reducing the maximum connected nodes to a minimum could be desirable if traffic
limits are tiny. Keep in mind that bitcoin's trustless model works best if you are
connected to a handful of nodes.
