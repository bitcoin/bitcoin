Example scripts for User-space, Statically Defined Tracing (USDT)
=================================================================

This directory contains scripts showcasing User-space, Statically Defined
Tracing (USDT) support for Bitcoin Core on Linux using. For more information on
USDT support in Bitcoin Core see the [USDT documentation].

[USDT documentation]: ../../doc/tracing.md


Examples for the two main eBPF front-ends, [bpftrace] and
[BPF Compiler Collection (BCC)], with support for USDT, are listed. BCC is used
for complex tools and daemons and `bpftrace` is preferred for one-liners and
shorter scripts.

[bpftrace]: https://github.com/iovisor/bpftrace
[BPF Compiler Collection (BCC)]: https://github.com/iovisor/bcc


To develop and run bpftrace and BCC scripts you need to install the
corresponding packages. See [installing bpftrace] and [installing BCC] for more
information. For development there exist a [bpftrace Reference Guide], a
[BCC Reference Guide], and a [bcc Python Developer Tutorial].

[installing bpftrace]: https://github.com/iovisor/bpftrace/blob/master/INSTALL.md
[installing BCC]: https://github.com/iovisor/bcc/blob/master/INSTALL.md
[bpftrace Reference Guide]: https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md
[BCC Reference Guide]: https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md
[bcc Python Developer Tutorial]: https://github.com/iovisor/bcc/blob/master/docs/tutorial_bcc_python_developer.md

## Examples

The bpftrace examples contain a relative path to the `bitcoind` binary. By
default, the scripts should be run from the repository-root and assume a
self-compiled `bitcoind` binary. The paths in the examples can be changed, for
example, to point to release builds if needed. See the
[Bitcoin Core USDT documentation] on how to list available tracepoints in your
`bitcoind` binary.

[Bitcoin Core USDT documentation]: ../../doc/tracing.md#listing-available-tracepoints

**WARNING: eBPF programs require root privileges to be loaded into a Linux
kernel VM. This means the bpftrace and BCC examples must be executed with root
privileges. Make sure to carefully review any scripts that you run with root
privileges first!**

### log_p2p_traffic.bt

A bpftrace script logging information about inbound and outbound P2P network
messages. Based on the `net:inbound_message` and `net:outbound_message`
tracepoints.

By default, `bpftrace` limits strings to 64 bytes due to the limited stack size
in the eBPF VM. For example, Tor v3 addresses exceed the string size limit which
results in the port being cut off during logging. The string size limit can be
increased with the `BPFTRACE_STRLEN` environment variable (`BPFTRACE_STRLEN=70`
works fine).

```
$ bpftrace contrib/tracing/log_p2p_traffic.bt
```

Output
```
outbound 'ping' msg to peer 11 (outbound-full-relay, [2a02:b10c:f747:1:ef:fake:ipv6:addr]:8333) with 8 bytes
inbound 'pong' msg from peer 11 (outbound-full-relay, [2a02:b10c:f747:1:ef:fake:ipv6:addr]:8333) with 8 bytes
inbound 'inv' msg from peer 16 (outbound-full-relay, XX.XX.XXX.121:8333) with 37 bytes
outbound 'getdata' msg to peer 16 (outbound-full-relay, XX.XX.XXX.121:8333) with 37 bytes
inbound 'tx' msg from peer 16 (outbound-full-relay, XX.XX.XXX.121:8333) with 222 bytes
outbound 'inv' msg to peer 9 (outbound-full-relay, faketorv3addressa2ufa6odvoi3s77j4uegey0xb10csyfyve2t33curbyd.onion:8333) with 37 bytes
outbound 'inv' msg to peer 7 (outbound-full-relay, XX.XX.XXX.242:8333) with 37 bytes
…
```

### p2p_monitor.py

A BCC Python script using curses for an interactive P2P message monitor. Based
on the `net:inbound_message` and `net:outbound_message` tracepoints.

Inbound and outbound traffic is listed for each peer together with information
about the connection. Peers can be selected individually to view recent P2P
messages.

```
$ python3 contrib/tracing/p2p_monitor.py ./src/bitcoind
```

Lists selectable peers and traffic and connection information.
```
 P2P Message Monitor
 Navigate with UP/DOWN or J/K and select a peer with ENTER or SPACE to see individual P2P messages

 PEER  OUTBOUND              INBOUND               TYPE                   ADDR
    0  46          398 byte  61      1407590 byte  block-relay-only       XX.XX.XXX.196:8333
   11  1156     253570 byte  3431    2394924 byte  outbound-full-relay    XXX.X.XX.179:8333
   13  3425    1809620 byte  1236     305458 byte  inbound                XXX.X.X.X:60380
   16  1046     241633 byte  1589    1199220 byte  outbound-full-relay    4faketorv2pbfu7x.onion:8333
   19  577      181679 byte  390      148951 byte  outbound-full-relay    kfake4vctorjv2o2.onion:8333
   20  11         1248 byte  13         1283 byte  block-relay-only       [2600:fake:64d9:b10c:4436:aaaa:fe:bb]:8333
   21  11         1248 byte  13         1299 byte  block-relay-only       XX.XXX.X.155:8333
   22  5           103 byte  1           102 byte  feeler                 XX.XX.XXX.173:8333
   23  11         1248 byte  12         1255 byte  block-relay-only       XX.XXX.XXX.220:8333
   24  3           103 byte  1           102 byte  feeler                 XXX.XXX.XXX.64:8333
…
```

Showing recent P2P messages between our node and a selected peer.

```
    ----------------------------------------------------------------------
    |                PEER 16 (4faketorv2pbfu7x.onion:8333)               |
    | OUR NODE                outbound-full-relay                   PEER |
    |                                           <--- sendcmpct (9 bytes) |
    | inv (37 byte) --->                                                 |
    |                                                <--- ping (8 bytes) |
    | pong (8 byte) --->                                                 |
    | inv (37 byte) --->                                                 |
    |                                               <--- addr (31 bytes) |
    | inv (37 byte) --->                                                 |
    |                                       <--- getheaders (1029 bytes) |
    | headers (1 byte) --->                                              |
    |                                           <--- feefilter (8 bytes) |
    |                                                <--- pong (8 bytes) |
    |                                            <--- headers (82 bytes) |
    |                                            <--- addr (30003 bytes) |
    | inv (1261 byte) --->                                               |
    |                                 …                                  |

```

### log_raw_p2p_msgs.py

A BCC Python script showcasing eBPF and USDT limitations when passing data
larger than about 32kb. Based on the `net:inbound_message` and
`net:outbound_message` tracepoints.

Bitcoin P2P messages can be larger than 32kb (e.g. `tx`, `block`, ...). The
eBPF VM's stack is limited to 512 bytes, and we can't allocate more than about
32kb for a P2P message in the eBPF VM. The **message data is cut off** when the
message is larger than MAX_MSG_DATA_LENGTH (see script). This can be detected
in user-space by comparing the data length to the message length variable. The
message is cut off when the data length is smaller than the message length.
A warning is included with the printed message data.

Data is submitted to user-space (i.e. to this script) via a ring buffer. The
throughput of the ring buffer is limited. Each p2p_message is about 32kb in
size. In- or outbound messages submitted to the ring buffer in rapid
succession fill the ring buffer faster than it can be read. Some messages are
lost. BCC prints: `Possibly lost 2 samples` on lost messages.


```
$ python3 contrib/tracing/log_raw_p2p_msgs.py ./src/bitcoind
```

```
Logging raw P2P messages.
Messages larger that about 32kb will be cut off!
Some messages might be lost!
 outbound msg 'inv' from peer 4 (outbound-full-relay, XX.XXX.XX.4:8333) with 253 bytes: 0705000000be2245c8f844c9f763748e1a7…
…
Warning: incomplete message (only 32568 out of 53552 bytes)! inbound msg 'tx' from peer 32 (outbound-full-relay, XX.XXX.XXX.43:8333) with 53552 bytes: 020000000001fd3c01939c85ad6756ed9fc…
…
Possibly lost 2 samples
```
