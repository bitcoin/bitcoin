# Block and Transaction Broadcasting With ZeroMQ

[ZeroMQ](http://zeromq.org/) is a lightweight wrapper around TCP
connections, inter-process communication, and shared-memory,
providing various message-oriented semantics such as publish/subscribe,
request/reply, and push/pull.

The Dash Core daemon can be configured to act as a trusted "border
router", implementing the dash wire protocol and relay, making
consensus decisions, maintaining the local blockchain database,
broadcasting locally generated transactions into the network, and
providing a queryable RPC interface to interact on a polled basis for
requesting blockchain related data. However, there exists only a
limited service to notify external software of events like the arrival
of new blocks or transactions.

The ZeroMQ facility implements a notification interface through a set
of specific notifiers. Currently there are notifiers that publish
blocks and transactions. This read-only facility requires only the
connection of a corresponding ZeroMQ subscriber port in receiving
software; it is not authenticated nor is there any two-way protocol
involvement. Therefore, subscribers should validate the received data
since it may be out of date, incomplete or even invalid.

ZeroMQ sockets are self-connecting and self-healing; that is,
connections made between two endpoints will be automatically restored
after an outage, and either end may be freely started or stopped in
any order.

Because ZeroMQ is message oriented, subscribers receive transactions
and blocks all-at-once and do not need to implement any sort of
buffering or reassembly.

## Prerequisites

The ZeroMQ feature in Dash Core requires ZeroMQ API version 4.x or
newer. Typically, it is packaged by distributions as something like
*libzmq3-dev*. The C++ wrapper for ZeroMQ is *not* needed.

In order to run the example Python client scripts in contrib/ one must
also install *python3-zmq*, though this is not necessary for daemon
operation.

## Enabling

By default, the ZeroMQ feature is automatically compiled in if the
necessary prerequisites are found.  To disable, use --disable-zmq
during the *configure* step of building bitcoind:

    $ ./configure --disable-zmq (other options)

To actually enable operation, one must set the appropriate options on
the commandline or in the configuration file.

## Usage

Currently, the following notifications are supported:

    -zmqpubhashtx=address
    -zmqpubhashtxlock=address
    -zmqpubhashblock=address
    -zmqpubrawblock=address
    -zmqpubrawtx=address
    -zmqpubrawtxlock=address

The socket type is PUB and the address must be a valid ZeroMQ socket
address. The same address can be used in more than one notification.

For instance:

    $ dashd -zmqpubhashtx=tcp://127.0.0.1:28332 \
               -zmqpubrawtx=ipc:///tmp/dashd.tx.raw

Each PUB notification has a topic and body, where the header
corresponds to the notification type. For instance, for the
notification `-zmqpubhashtx` the topic is `hashtx` (no null
terminator) and the body is the hexadecimal transaction hash (32
bytes).

These options can also be provided in dash.conf.

ZeroMQ endpoint specifiers for TCP (and others) are documented in the
[ZeroMQ API](http://api.zeromq.org/4-0:_start).

Client side, then, the ZeroMQ subscriber socket must have the
ZMQ_SUBSCRIBE option set to one or either of these prefixes (for
instance, just `hash`); without doing so will result in no messages
arriving. Please see `contrib/zmq/zmq_sub.py` for a working example.

## Remarks

From the perspective of dashd, the ZeroMQ socket is write-only; PUB
sockets don't even have a read function. Thus, there is no state
introduced into dashd directly. Furthermore, no information is
broadcast that wasn't already received from the public P2P network.

No authentication or authorization is done on connecting clients; it
is assumed that the ZeroMQ port is exposed only to trusted entities,
using other means such as firewalling.

Note that when the block chain tip changes, a reorganisation may occur
and just the tip will be notified. It is up to the subscriber to
retrieve the chain from the last known block to the new tip.

There are several possibilities that ZMQ notification can get lost
during transmission depending on the communication type your are
using. Dashd appends an up-counting sequence number to each
notification which allows listeners to detect lost notifications.
