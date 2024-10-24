#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Interactive bitcoind P2P network traffic monitor utilizing USDT and the
    net:inbound_message and net:outbound_message tracepoints. """

# This script demonstrates what USDT for Bitcoin Core can enable. It uses BCC
# (https://github.com/iovisor/bcc) to load a sandboxed eBPF program into the
# Linux kernel (root privileges are required). The eBPF program attaches to two
# statically defined tracepoints. The tracepoint 'net:inbound_message' is called
# when a new P2P message is received, and 'net:outbound_message' is called on
# outbound P2P messages. The eBPF program submits the P2P messages to
# this script via a BPF ring buffer.

import curses
import os
import sys
from curses import wrapper, panel
from bcc import BPF, USDT

# BCC: The C program to be compiled to an eBPF program (by BCC) and loaded into
# a sandboxed Linux kernel VM.
program = """
#include <uapi/linux/ptrace.h>

// Tor v3 addresses are 62 chars + 6 chars for the port (':12345').
// I2P addresses are 60 chars + 6 chars for the port (':12345').
#define MAX_PEER_ADDR_LENGTH 62 + 6
#define MAX_PEER_CONN_TYPE_LENGTH 20
#define MAX_MSG_TYPE_LENGTH 20

struct p2p_message
{
    u64     peer_id;
    char    peer_addr[MAX_PEER_ADDR_LENGTH];
    char    peer_conn_type[MAX_PEER_CONN_TYPE_LENGTH];
    char    msg_type[MAX_MSG_TYPE_LENGTH];
    u64     msg_size;
};


// Two BPF perf buffers for pushing data (here P2P messages) to user space.
BPF_PERF_OUTPUT(inbound_messages);
BPF_PERF_OUTPUT(outbound_messages);

int trace_inbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};

    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg.peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg.peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg.msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);

    inbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
};

int trace_outbound_message(struct pt_regs *ctx) {
    struct p2p_message msg = {};

    bpf_usdt_readarg(1, ctx, &msg.peer_id);
    bpf_usdt_readarg_p(2, ctx, &msg.peer_addr, MAX_PEER_ADDR_LENGTH);
    bpf_usdt_readarg_p(3, ctx, &msg.peer_conn_type, MAX_PEER_CONN_TYPE_LENGTH);
    bpf_usdt_readarg_p(4, ctx, &msg.msg_type, MAX_MSG_TYPE_LENGTH);
    bpf_usdt_readarg(5, ctx, &msg.msg_size);

    outbound_messages.perf_submit(ctx, &msg, sizeof(msg));
    return 0;
};
"""


class Message:
    """ A P2P network message. """
    msg_type = ""
    size = 0
    data = bytes()
    inbound = False

    def __init__(self, msg_type, size, inbound):
        self.msg_type = msg_type
        self.size = size
        self.inbound = inbound


class Peer:
    """ A P2P network peer. """
    id = 0
    address = ""
    connection_type = ""
    last_messages = list()

    total_inbound_msgs = 0
    total_inbound_bytes = 0
    total_outbound_msgs = 0
    total_outbound_bytes = 0

    def __init__(self, id, address, connection_type):
        self.id = id
        self.address = address
        self.connection_type = connection_type
        self.last_messages = list()

    def add_message(self, message):
        self.last_messages.append(message)
        if len(self.last_messages) > 25:
            self.last_messages.pop(0)
        if message.inbound:
            self.total_inbound_bytes += message.size
            self.total_inbound_msgs += 1
        else:
            self.total_outbound_bytes += message.size
            self.total_outbound_msgs += 1


def main(pid):
    peers = dict()
    print(f"Hooking into bitcoind with pid {pid}")
    bitcoind_with_usdts = USDT(pid=int(pid))

    # attaching the trace functions defined in the BPF program to the tracepoints
    bitcoind_with_usdts.enable_probe(
        probe="inbound_message", fn_name="trace_inbound_message")
    bitcoind_with_usdts.enable_probe(
        probe="outbound_message", fn_name="trace_outbound_message")
    bpf = BPF(text=program, usdt_contexts=[bitcoind_with_usdts])

    # BCC: perf buffer handle function for inbound_messages
    def handle_inbound(_, data, size):
        """ Inbound message handler.

        Called each time a message is submitted to the inbound_messages BPF table."""
        event = bpf["inbound_messages"].event(data)
        if event.peer_id not in peers:
            peer = Peer(event.peer_id, event.peer_addr.decode(
                "utf-8"), event.peer_conn_type.decode("utf-8"))
            peers[peer.id] = peer
        peers[event.peer_id].add_message(
            Message(event.msg_type.decode("utf-8"), event.msg_size, True))

    # BCC: perf buffer handle function for outbound_messages
    def handle_outbound(_, data, size):
        """ Outbound message handler.

        Called each time a message is submitted to the outbound_messages BPF table."""
        event = bpf["outbound_messages"].event(data)
        if event.peer_id not in peers:
            peer = Peer(event.peer_id, event.peer_addr.decode(
                "utf-8"), event.peer_conn_type.decode("utf-8"))
            peers[peer.id] = peer
        peers[event.peer_id].add_message(
            Message(event.msg_type.decode("utf-8"), event.msg_size, False))

    # BCC: add handlers to the inbound and outbound perf buffers
    bpf["inbound_messages"].open_perf_buffer(handle_inbound)
    bpf["outbound_messages"].open_perf_buffer(handle_outbound)

    wrapper(loop, bpf, peers)


def loop(screen, bpf, peers):
    screen.nodelay(1)
    cur_list_pos = 0
    win = curses.newwin(30, 70, 2, 7)
    win.erase()
    win.border(ord("|"), ord("|"), ord("-"), ord("-"),
               ord("-"), ord("-"), ord("-"), ord("-"))
    info_panel = panel.new_panel(win)
    info_panel.hide()

    ROWS_AVALIABLE_FOR_LIST = curses.LINES - 5
    scroll = 0

    while True:
        try:
            # BCC: poll the perf buffers for new events or timeout after 50ms
            bpf.perf_buffer_poll(timeout=50)

            ch = screen.getch()
            if (ch == curses.KEY_DOWN or ch == ord("j")) and cur_list_pos < len(
                    peers.keys()) -1 and info_panel.hidden():
                cur_list_pos += 1
                if cur_list_pos >= ROWS_AVALIABLE_FOR_LIST:
                    scroll += 1
            if (ch == curses.KEY_UP or ch == ord("k")) and cur_list_pos > 0 and info_panel.hidden():
                cur_list_pos -= 1
                if scroll > 0:
                    scroll -= 1
            if ch == ord('\n') or ch == ord(' '):
                if info_panel.hidden():
                    info_panel.show()
                else:
                    info_panel.hide()
            screen.erase()
            render(screen, peers, cur_list_pos, scroll, ROWS_AVALIABLE_FOR_LIST, info_panel)
            curses.panel.update_panels()
            screen.refresh()
        except KeyboardInterrupt:
            exit()


def render(screen, peers, cur_list_pos, scroll, ROWS_AVALIABLE_FOR_LIST, info_panel):
    """ renders the list of peers and details panel

    This code is unrelated to USDT, BCC and BPF.
    """
    header_format = "%6s  %-20s  %-20s  %-22s  %-67s"
    row_format = "%6s  %-5d %9d byte  %-5d %9d byte  %-22s  %-67s"

    screen.addstr(0, 1, (" P2P Message Monitor "), curses.A_REVERSE)
    screen.addstr(
        1, 0, (" Navigate with UP/DOWN or J/K and select a peer with ENTER or SPACE to see individual P2P messages"), curses.A_NORMAL)
    screen.addstr(3, 0,
                  header_format % ("PEER", "OUTBOUND", "INBOUND", "TYPE", "ADDR"), curses.A_BOLD | curses.A_UNDERLINE)
    peer_list = sorted(peers.keys())[scroll:ROWS_AVALIABLE_FOR_LIST+scroll]
    for i, peer_id in enumerate(peer_list):
        peer = peers[peer_id]
        screen.addstr(i + 4, 0,
                      row_format % (peer.id, peer.total_outbound_msgs, peer.total_outbound_bytes,
                                    peer.total_inbound_msgs, peer.total_inbound_bytes,
                                    peer.connection_type, peer.address),
                      curses.A_REVERSE if i + scroll == cur_list_pos else curses.A_NORMAL)
        if i + scroll == cur_list_pos:
            info_window = info_panel.window()
            info_window.erase()
            info_window.border(
                ord("|"), ord("|"), ord("-"), ord("-"),
                ord("-"), ord("-"), ord("-"), ord("-"))

            info_window.addstr(
                1, 1, f"PEER {peer.id} ({peer.address})".center(68), curses.A_REVERSE | curses.A_BOLD)
            info_window.addstr(
                2, 1, f" OUR NODE{peer.connection_type:^54}PEER ",
                curses.A_BOLD)
            for i, msg in enumerate(peer.last_messages):
                if msg.inbound:
                    info_window.addstr(
                        i + 3, 1, "%68s" %
                        (f"<--- {msg.msg_type} ({msg.size} bytes) "), curses.A_NORMAL)
                else:
                    info_window.addstr(
                        i + 3, 1, " %s (%d byte) --->" %
                        (msg.msg_type, msg.size), curses.A_NORMAL)


def running_as_root():
    return os.getuid() == 0

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE:", sys.argv[0], "<pid of bitcoind>")
        exit()
    if not running_as_root():
        print("You might not have the privileges required to hook into the tracepoints!")
    pid = sys.argv[1]
    main(pid)
