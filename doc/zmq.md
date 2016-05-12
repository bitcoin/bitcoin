# Block and Transaction Broadcasting With ZeroMQ

[ZeroMQ](http://zeromq.org/) is a lightweight wrapper around TCP
connections, inter-process communication, and shared-memory,
providing various message-oriented semantics such as publish/subscribe,
request/reply, and push/pull.

The Bitcoin Core daemon can be configured to act as a trusted "border
router", implementing the bitcoin wire protocol and relay, making
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

The ZeroMQ feature in Bitcoin Core requires ZeroMQ API version 4.x or
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
    -zmqpubhashblock=address
    -zmqpubrawblock=address
    -zmqpubrawtx=address
    -zmqpubmempool=address

The socket type is PUB and the address must be a valid ZeroMQ socket
address. The same address can be used in more than one notification.

For instance:

    $ bitcoind -zmqpubhashtx=tcp://127.0.0.1:28332 \
               -zmqpubrawtx=ipc:///tmp/bitcoind.tx.raw

Each PUB notification has a topic and body, where the header
corresponds to the notification type. For instance, for the
notification `-zmqpubhashtx` the topic is `hashtx` (no null
terminator) and the body is the hexadecimal transaction hash (32
bytes).

These options can also be provided in bitcoin.conf.

ZeroMQ endpoint specifiers for TCP (and others) are documented in the
[ZeroMQ API](http://api.zeromq.org/4-0:_start).

Client side, then, the ZeroMQ subscriber socket must have the
ZMQ_SUBSCRIBE option set to one or either of these prefixes (for
instance, just `hash`); without doing so will result in no messages
arriving. Please see `contrib/zmq/zmq_sub.py` for a working example.

## Notification types

### hashtx

`hashtx` (enabled by `-zmqpubhashtx`) is broadcasted on every incoming,
accepted transaction.

The body is 32 bytes, and contains the binary hash of the transaction.

### hashblock

`hashblock` (enabled by `-zmqpubhashblock`) is broadcasted on every incoming,
accepted block.

The body is 32 bytes, and contains the binary hash of the block.

### rawtx

`rawtx` (enabled by `-zmqpubrawtx`) is broadcasted on every incoming, accepted
transaction.

The body has a variable length, and contains the raw binary data of the transaction.

### rawblock

`rawblock` (enabled by `-zmqpubrawblock`) is broadcasted on every incoming,
accepted block.

The body has a variable length, and contains the raw binary data of the block.

### mempooladd

`mempooladd` (enabled by `-zmqpubmempool`) is broadcasted on every transaction
that is added to the mempool.

The body is 44 bytes.

| Offset  | Type (size)   | Description  |
| -------:|:------------- |:------------ |
| 0       | hash (32)     | Hash of the transaction |
| 32      | uint64 (8)    | Fee posted with transaction (absolute, not per kB) |
| 40      | uint32 (4)    | Size of transaction |

Values are in little-endian, independent of the platform.

### mempoolremove

`mempoolremove` (enabled by `-zmqpubmempool`) is broadcasted on every
transaction that is removed from the mempool.

The body is 33 bytes.

| Offset  | Type (size)   | Description  |
| -------:|:------------- |:------------ |
| 0       | hash (32)     | Hash of the transaction |
| 32      | enum (1)      | Reason for removal (see below) |

The reason can have the following values:

| Enum    | Id            | Description  |
| -------:|:------------- |:------------ |
| 0       | `UNKNOWN`     | Manually removed or unknown reason |
| 1       | `EXPIRY`      | Expired from mempool |
| 2       | `SIZELIMIT`   | Removed in size limiting |
| 3       | `REORG`       | Removed for reorganization |
| 4       | `BLOCK`       | Removed for block |
| 5       | `REPLACED`    | Removed for conflict (replaced) |

Note that future versions may add further removal reasons, so it is
recommended to treat any unknown value as `UNKNOWN`.

## Note about transaction and block hashes

The 32-byte transaction and block hashes in notifications are byte-by-byte
reversed compared to their in-memory representation.

They match the order of the hexadecimal representation as used in the debug log
as well as RPC.

## Remarks

From the perspective of bitcoind, the ZeroMQ socket is write-only; PUB
sockets don't even have a read function. Thus, there is no state
introduced into bitcoind directly. Furthermore, no information is
broadcast that wasn't already received from the public P2P network.

No authentication or authorization is done on connecting clients; it
is assumed that the ZeroMQ port is exposed only to trusted entities,
using other means such as firewalling.

Note that when the block chain tip changes, a reorganisation may occur
and just the tip will be notified. It is up to the subscriber to
retrieve the chain from the last known block to the new tip.

There are several possibilities that ZMQ notification can get lost
during transmission depending on the communication type your are
using. Bitcoind appends an up-counting sequence number to each
notification which allows listeners to detect lost notifications.
