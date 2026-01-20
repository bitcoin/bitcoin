P2P and network changes
-----------------------

- Normally local transactions are broadcast to all connected peers with
  which we do transaction relay. Now, for the `sendrawtransaction` RPC
  this behavior can be changed to only do the broadcast via the Tor or
  I2P networks. A new boolean option `-privatebroadcast` has been added
  to enable this behavior. This improves the privacy of the transaction
  originator in two aspects:
  1. Their IP address (and thus geolocation) is never known to the
     recipients.
  2. If the originator sends two otherwise unrelated transactions, they
     will not be linkable. This is because a separate connection is used
     for broadcasting each transaction. (#29415)
