#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" python/bcc tool to log mempool additions and evictions. """

import sys
import queue
from datetime import datetime
from bcc import BPF, USDT

# BCC: The C program to be compiled to an eBPF program (by BCC) and loaded into
# a sandboxed Linux kernel VM.
program = """
#include <uapi/linux/ptrace.h>

#define TXID_BYTES 32

struct mempool_event
{
    u32     reason; //Eviction only
    u8      txid[TXID_BYTES];
    u64     value;
    u64     fee;
    u64     size;
    u64     weight;
};

//bpffs map queue objects for storing event data
BPF_QUEUE(evict_q, struct mempool_event, 10240);
BPF_QUEUE(add_q, struct mempool_event, 10240);

int trace_mempool_add(struct pt_regs *ctx) {
    struct mempool_event event = {};

    bpf_usdt_readarg_p(1, ctx, &event.txid, TXID_BYTES);
    bpf_usdt_readarg(2, ctx, &event.value);
    bpf_usdt_readarg(3, ctx, &event.size);
    bpf_usdt_readarg(4, ctx, &event.weight);
    bpf_usdt_readarg(5, ctx, &event.fee);

    add_q.push(&event,BPF_EXIST);

    return 0;
};

int trace_mempool_evict(struct pt_regs *ctx) {
    struct mempool_event event = {};

    bpf_usdt_readarg_p(1, ctx, &event.txid, TXID_BYTES);
    bpf_usdt_readarg(2, ctx, &event.value);
    bpf_usdt_readarg(3, ctx, &event.size);
    bpf_usdt_readarg(4, ctx, &event.weight);
    bpf_usdt_readarg(5, ctx, &event.fee);
    bpf_usdt_readarg(6, ctx, &event.reason);

    evict_q.push(&event,BPF_EXIST);

    return 0;
};
"""

#Thread safe queue object
event_q = queue.Queue()

def mempool_evict_queue(event):
    """ Mempool Eviction Event Queue Adapter.

    Converts a mempool eviction event into an object ready to be placed into the event logging queue for output"""

    evict_reasons = ['expiry','sizelimit','reorg','block','conflict','replaced']
    event_struct = {
        'bfp_data' : event,
        'type' : 'evict',
        'reason_string' : evict_reasons[event.reason],
        'sys_time' : str(datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
    }
    event_q.put(event_struct)

def mempool_add_queue(event):
    """ Mempool Eviction Event Queue Adapter.

    Converts a mempool addition event into an object ready to be placed into the event logging queue for output"""

    event_struct = {
        'bfp_data' : event,
        'type' : 'added',
        'reason_string' : 'broadcast',
        'sys_time' : str(datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3])
    }
    event_q.put(event_struct)

def csv_line_out(q_event):
    """ CSV formated single line mempool event log output

    Converts and prints q_event formatted object into a single line of csv data"""

    txid = str(hex(int.from_bytes(bytes(q_event['bfp_data'].txid[:32]), "big"))).replace('0x','').zfill(64)
    value = q_event['bfp_data'].value
    fee = q_event['bfp_data'].fee
    size = q_event['bfp_data'].size
    weight = q_event['bfp_data'].weight
    timestamp = q_event['sys_time']
    event_type = q_event['type']
    event_reason = q_event['reason_string']
    print(f'{timestamp},{event_type},{event_reason},{txid},{value},{fee},{size},{weight}')


def main(bitcoind_path):
    bitcoind_with_usdts = USDT(path=str(bitcoind_path))

    # attaching the trace functions defined in the BPF program to the tracepoints
    bitcoind_with_usdts.enable_probe(
        probe="added", fn_name="trace_mempool_add")
    bitcoind_with_usdts.enable_probe(
        probe="evicted", fn_name="trace_mempool_evict")
    bpf = BPF(text=program, usdt_contexts=[bitcoind_with_usdts])

    # BCC: perf buffer handle function for mempool evictions
    def handle_mempool_add(_, data, size):
        """ Inbound message handler.

        Called each time a message is submitted to the inbound_messages BPF table."""

        event = bpf["mempool_add_event"].event(data)
        mempool_add_queue(event)

    def handle_mempool_evict(_, data, size):
        """ Inbound message handler.

        Called each time a message is submitted to the inbound_messages BPF table."""

        event = bpf["mempool_evict_event"].event(data)
        mempool_evict_queue(event)

    while True:
        try:
            #Get the values stored in the bpffs queues
            for item in bpf['add_q'].values():
                mempool_add_queue(item)
            for item in bpf['evict_q'].values():
                mempool_evict_queue(item)
            #Check the python event queue and log the stored results
            while not event_q.empty():
                mempool_event = event_q.get_nowait()
                csv_line_out(mempool_event)
                event_q.task_done()
        except KeyboardInterrupt:
            print()
            exit()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("USAGE:", sys.argv[0], "path/to/bitcoind")
        exit()
    path = sys.argv[1]
    main(path)
