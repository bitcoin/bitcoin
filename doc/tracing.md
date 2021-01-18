Tracing
=======

To make it possible to have more detailed logging for troubleshooting, or for keeping track of detailed statistics, Bitcoin Core makes use of so-called eBPF tracepoints.

The linux kernel can hook into these tracepoints at runtime, but when disabled they have little to no performance impact. Traces can pass data which can be printed externally via tools such as `bpftrace`.

Viewing all tracepoints
-----------------------

To view all available probes in a binary, use `info probes` in `gdb`:

```
$ gdb src/bitcoind
…
(gdb) info probes
Type Provider   Name            Where              Semaphore Object
stap net        process_message 0x00000000000d48f1           /…/bitcoin/src/bitcoind
stap net        push_message    0x000000000009db8b           /…/bitcoin/bitcoin/src/bitcoind
stap validation connect_block   0x00000000002adbe6           /…/bitcoin/bitcoin/src/bitcoind
…
```

Tracepoint documentation
------------------------

The current tracepoints are listed here.

### net

- `net:process_message(char* addr_str, int node_id, char* msgtype_str, uint8_t* rx_data, size_t rx_size)`

Called for every received P2P message.

- `net:push_message(char* msgtype_str, uint8_t* tx_data, size_t tx_size, int node_id)`

Called for every sent P2P message.

### validation

- `validation:connect_block(int height)`

Called when a new block is connected to the longest chain.

Examples
--------

## Network Messages

Here's a `bpftrace` script that prints incoming and outgoing network messages, and demonstrates generation of histograms:

```
#!/usr/bin/env bpftrace

BEGIN
{
  printf("bitcoin net msgs\n");
  @start = nsecs;
}

usdt:./src/bitcoind:net:push_message
{
  $ip = str(arg0);
  $peer_id = (int64)arg1;
  $command = str(arg2);
  $data_len = arg3;
  $data = buf(arg3,arg4);
  $t = (nsecs - @start) / 100000;

  printf("%zu outbound %s %s %zu %d %r\n", $t, $command, $ip, $peer_id, $data_len, $data);

  @outbound[$command]++;
}

usdt:./src/bitcoind:net:process_message
{
  $ip = str(arg0);
  $peer_id = (int64)arg1;
  $command = str(arg2);
  $data_len = arg3;
  $data = buf(arg3,arg4);
  $t = (nsecs - @start) / 100000;

  printf("%zu inbound %s %s %zu %d %r\n", $t, $command, $ip, $peer_id, $data_len, $data);

  @inbound[$ip, $command]++;
}

```

    $ sudo bpftrace netmsg.bt

Example output:

```
Attaching 3 probes...
bitcoin net msgs
26058 outbound version X.X.X.X:8333 0 -402038944 \x80\x11\x01\x00\x09\x04\x00\x00\x00\x00\x00\x00\xb4\xc5S_\x00\x00\x00\x00\x0d\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xc1FN\x94 \x8d\x09\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
…
@inbound[85.195.X.X:8333, sendheaders]: 1
@inbound[193.70.X.X:8333, sendheaders]: 1
@inbound[193.70.X.X:8333, pong]: 1
@inbound[193.70.X.X:8333, verack]: 1
@inbound[85.195.X.X:8333, version]: 1
@inbound[85.195.X.X:8333, verack]: 1
@inbound[85.195.X.X:8333, getheaders]: 1
@inbound[193.70.X.X:8333, version]: 1
@inbound[85.195.X.X:8333, feefilter]: 1
@inbound[193.70.X.X:8333, feefilter]: 1
@inbound[193.70.X.X:8333, getheaders]: 1
@inbound[193.70.X.X:8333, ping]: 1
@inbound[85.195.X.X:8333, pong]: 1
@inbound[193.70.X.X:8333, addr]: 1
@inbound[85.195.X.X:8333, ping]: 1
@inbound[85.195.X.X:8333, addr]: 1
@inbound[85.195.X.X:8333, sendcmpct]: 2
@inbound[193.70.X.X:8333, sendcmpct]: 2
@inbound[193.70.X.X:8333, headers]: 16

@outbound[version]: 2
@outbound[feefilter]: 2
@outbound[verack]: 2
@outbound[pong]: 2
@outbound[ping]: 2
@outbound[sendheaders]: 2
@outbound[getaddr]: 2
@outbound[sendcmpct]: 4
@outbound[getheaders]: 17

@start: 2329712756047960
```

At the end of the output there is a histogram of all the messages grouped by message type and IP.

## IBD Benchmarking

```
#!/usr/bin/env bpftrace
BEGIN
{
  printf("IBD to 500,000 bench\n");
}

usdt:./src/bitcoind:validation:connect_block
{
  $height = (uint32)arg0;

  if ($height == 1) {
    printf("block 1 found, starting benchmark\n");
    @start = nsecs;
  }

  if ($height >= 500000) {
    @end = nsecs;
    @duration = @end - @start;
    exit();
  }
}

END {
  printf("duration %d ms\n", @duration / 1000000)
}
```

This one hooks into ConnectBlock and prints the IBD time to height 500,000 starting from the first call to ConnectBlock.

Adding tracepoints
------------------

To add a new tracepoint, `#include <util/trace.h>` in the compilation unit where it is to be inserted, then call one of the `TRACEx` macros depending on the number of arguments at the relevant position in the code. Up to 12 arguments can be provided:

```c
#define TRACE(context, event)
#define TRACE1(context, event, a)
#define TRACE2(context, event, a, b)
#define TRACE3(context, event, a, b, c)
#define TRACE4(context, event, a, b, c, d)
#define TRACE5(context, event, a, b, c, d, e)
#define TRACE6(context, event, a, b, c, d, e, f)
#define TRACE7(context, event, a, b, c, d, e, f, g)
#define TRACE8(context, event, a, b, c, d, e, f, g, h)
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i)
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j)
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k)
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l)
```

The `context` and `event` specify the names by which the tracepoint is to be referred to. Please use `snake_case` and try to make sure that the tracepoint names make sense even without detailed knowledge of the implementation details. Do not forget to update the list in this document.

For example:

```
    TRACE5(net, process_message,
           pfrom->addr.ToString().c_str(),
           pfrom->GetId(),
           msg.m_command.c_str(),
           msg.m_recv.data(),
           msg.m_recv.size());
```

Make sure that what is passed to the tracepoint is inexpensive to compute. Although the tracepoint itself only has overhead when enabled, the code to compute arguments is always run—even if the tracepoint is disabled.
