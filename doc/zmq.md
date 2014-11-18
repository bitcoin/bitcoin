# Block and Transaction Broadcasting With ZeroMQ

[ZeroMQ](http://zeromq.org/) is a lightweight wrapper around TCP
connections, inter-process communications, and shared-memory,
providing various message-oriented semantics such as publish/subcribe,
request/reply, and push/pull.

The Bitcoin Core daemon can be configured to act as a trusted "border
router", implementing the bitcoin wire protocol and relay, making
consensus decisions, maintaining the local blockchain database,
broadcasting locally generated transactions into the network, and
providing a queryable RPC interface to interact on a polled basis for
requesting blockchain related data.  However, there exists only a
limited service to notify external software of events like the arrival
of new blocks or transactions.

The ZeroMQ facility binds a "publish" port that broadcasts all newly
arrived *and validated* blocks and transactions to one or more
connected subscribers.  This read-only facility requires only the
connection of a corresponding ZeroMQ subscriber port in receiving
software; it is not authenticated nor is there any two-way protocol
involvement.

In this fashion, external software can use a trusted Bitcoin Core
daemon to do the heavy lifting of communicating with the P2P network,
while still receiving network information asynchronously in real
time. As transactions and blocks arrive via the P2P network, Bitcoin
Core will apply all the validation and standardization rules to this
data, only passing along through ZeroMQ those items that pass.

ZeroMQ sockets are self-connecting and self-healing; that is, connects
made between two endpoints will be automatically restored after an
outage, and either end may be freely started or stopped in any order.

Because ZeroMQ is message oriented, subscribers receive transactions
and blocks all-at-once and do not need to implement any sort of
buffering or reassembly.

## Prerequisites

The ZeroMQ feature in Bitcoin Core uses only a very small part of the
ZeroMQ C API, and is thus compatible with any version of ZeroMQ
from 2.1 onward, including all versions in the 3.x and 4.x release
series.  Typically, it is packaged by distributions as something like
*libzmq-dev*.

The C++ wrapper for ZeroMQ is *not* needed.

## Enabling

By default, the ZeroMQ port functionality is disabled.  Two steps are
required to enable--compiling in the ZeroMQ code, and configuring
runtime operation on the command-line or configuration file.

    $ ./configure --enable-zmq (other options)

This will produce a binary that is capable of providing the ZeroMQ
facility, but will not do so until also configured properly.

## Configuration

Currently, the ZeroMQ facility only needs to have the ZeroMQ endpoint
specified:

    $ bitcoind -zmqpub=tcp://127.0.0.1/28332

This will cause bitcoind to establish a PUB listening socket at the
specified host address and port number.  The endpoint specifier may
also be provided as the equivalent line item in bitcoin.conf.

ZeroMQ endpoint specifiers for TCP (and others) are documented in the
[ZeroMQ API](http://api.zeromq.org).

## Operation

ZeroMQ publish sockets prepend each data item with an arbitrary topic
prefix that allows subscriber clients to request only those items with
a matching prefix.  When publishing, bitcoind will prepend the topic
"TXN" (no quotes, no null terminator) to the binary, serialized form
of a published transaction, and "BLK" to the binary, serialized form
of a published block.

Client side, then, the ZeroMQ subscriber socket must have the
ZMQ_SUBSCRIBE option set to one or either of these prefixes; without
doing so will result in no messages arriving.

Here is a small example, in the Python language, using the
*python-zmq* wrapper:

    import zmq
    import binascii

    port = 28332
    topic1 = "BLK"
    topic2 = "TXN"
    topic_len = len(topic1)

    zmqContext = zmq.Context()
    zmqSubSocket = zmqContext.socket(zmq.SUB)
    zmqSubSocket.setsockopt(zmq.SUBSCRIBE, topic1)
    zmqSubSocket.setsockopt(zmq.SUBSCRIBE, topic2)
    zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

    def handleBLK(blk):
        print "-BLKHDR-"
        print binascii.hexlify(blk[:80])

    def handleTX(tx):
        print "-TX-"
        print binascii.hexlify(tx)

    try:
        while True:
            msg = zmqSubSocket.recv()
            msg_topic = msg[:topic_len]
            msg_data  = msg[topic_len:]

            if msg_topic == "TXN":
                handleTX(msg_data)
            elif msg_topic == "BLK":
                handleBLK(msg_data)

    except KeyboardInterrupt:
        zmqContext.destroy()


This example is provided in the contrib/zmq directory of the source
code.

## Security Considerations

From the perspective of bitcoind, the ZeroMQ socket is write-only; PUB
sockets don't even have a read function.  Thus, there is no state
introduced into bitcoind directly.  Furthermore, no information is
broadcast that wasn't already received from the public P2P network.

No authentication or authorization is done on connecting clients; it
is assumed that the ZeroMQ port is exposed only to trusted entities,
using other means such as firewalling.

Transactions and blocks are broadcast in their serialized form
directly as received and validated by bitcoind.  External software may
assume that these have passed all validity/consensus/standard checks,
but of course is free to perform these functions again in part or in
whole.
