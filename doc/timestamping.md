Timestamping with Crown
===========================

Crown Platform allows to store arbitrary 32-byte hashes into the blockchain,
so that existence of the hashed data at a certain point in time can be proven.
This is done in two (or three, depending on the point of view) steps:

1) Hash your data:
------------------

As the first step, you have to obtain the actual hash you want to store.  On
a GNU/Linux system, this can be done by the 'sha256sum' utility.  Assuming
that your data is in 'myfile.pdf', this works as follows:

  $ sha256sum myfile.pdf
  cde94e3ef4915554596bed1fe043fb20ed2278d18dbbab6b131851062d6ea7e1  myfile.pdf

The long hex string is the 32-byte SHA-256 hash that you can store into the
blockchain in the next step.

2) Timestamp the hash:
----------------------

Given your hash, you can use Crown's 'timestamp' RPC command to store
it into the blockchain in a special transaction.  This costs the usual
transaction fee required for the Crown network, but has no extra cost.

Open the RPC console via "Help -> Debug window -> Console".  If your wallet
is encrypted, you have to unlock it first:

  > walletpassphrase "Your long passphrase" 60

(60 means that the wallet will be unlocked for 60 seconds, you can
use any number you like.)

Then, you can issue the actual timestamping command together with your
hex string.  For instance:

  > timestamp cde94e3ef4915554596bed1fe043fb20ed2278d18dbbab6b131851062d6ea7e1
  663e70d27124be861106255c41f2c40850499a2c1deef38d202e920d0b725103

The resulting hex string is the transaction ID of the timestamping tx.  Your
data is now sent to the network, and will be recorded permanently in the
blockchain as soon as that transaction is confirmed.  As soon as the next
Crown block is merge-mined with a valid Crown block, there will even
be a "hash link" between your data and the Crown blockchain.  See also [1].

  [1] https://www.domob.eu/blog/2015/0225-NameStamp.php

3) Verifying the timestamp:
---------------------------

Finally, let us verify that your hash is indeed stored into the blockchain.
For this, you can query for the timestamping transaction ID:

  > getrawtransaction 663e70d27124be861106255c41f2c40850499a2c1deef38d202e920d0b725103
  010000000117eaa6ab8c5d526b35afa9c6028596f28ed2aa62ac6a907f3e8cb073481631d00100
  00006a47304402201330ce81cfa69edc234e1b2333498700931e795b0b28adca8a62ad75fe4b0c
  8a022049fe0ac3c76eea895c7f1d2237e3a21e4e653c18c5965a329243c408a7068d7e0121028b
  27353568e497c5bf1e29a1fdb766bc40f8d6967aa4b86b20d3ca7f1da8a775ffffffff02f06e1b
  55860400001976a914a329bfb1ac5972736feb8316587e40263adb612e88ac0000000000000000
  226a20cde94e3ef4915554596bed1fe043fb20ed2278d18dbbab6b131851062d6ea7e100000000

As you can see, the hash is contained literally in the transaction (last line)
and thus is also part of the blockchain.  Note that you may need to run your
Crown client with the '-txindex' option to look up by txid, or you may
need to query the transaction via the block containing it or an online
service.  In any case, your hash data is permanently recorded in the
blockchain and such a query can be used to prove this fact to the world.
