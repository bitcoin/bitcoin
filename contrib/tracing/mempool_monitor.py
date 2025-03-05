#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Example logging Bitcoin Core mempool events using the mempool:added,
    mempool:removed, mempool:replaced, and mempool:rejected tracepoints. """

import curses
import sys
from datetime import datetime, timezone

from bcc import BPF, USDT

# BCC: The C program to be compiled to an eBPF program (by BCC) and loaded into
# a sandboxed Linux kernel VM.
PROGRAM = """
# include <uapi/linux/ptrace.h>

// The longest rejection reason is 118 chars and is generated in case of SCRIPT_ERR_EVAL_FALSE by
// strprintf("mandatory-script-verify-flag-failed (%s)", ScriptErrorString(check.GetScriptError()))
#define MAX_REJECT_REASON_LENGTH        118
// The longest string returned by RemovalReasonToString() is 'sizelimit'
#define MAX_REMOVAL_REASON_LENGTH       9
#define HASH_LENGTH                     32

struct added_event
{
  u8    hash[HASH_LENGTH];
  s32   vsize;
  s64   fee;
};

struct removed_event
{
  u8    hash[HASH_LENGTH];
  char  reason[MAX_REMOVAL_REASON_LENGTH];
  s32   vsize;
  s64   fee;
  u64   entry_time;
};

struct rejected_event
{
  u8    hash[HASH_LENGTH];
  char  reason[MAX_REJECT_REASON_LENGTH];
};

struct replaced_event
{
  u8    replaced_hash[HASH_LENGTH];
  s32   replaced_vsize;
  s64   replaced_fee;
  u64   replaced_entry_time;
  u8    replacement_hash[HASH_LENGTH];
  s32   replacement_vsize;
  s64   replacement_fee;
};

// BPF perf buffer to push the data to user space.
BPF_PERF_OUTPUT(added_events);
BPF_PERF_OUTPUT(removed_events);
BPF_PERF_OUTPUT(rejected_events);
BPF_PERF_OUTPUT(replaced_events);

int trace_added(struct pt_regs *ctx) {
  struct added_event added = {};
  void *phash = NULL;
  bpf_usdt_readarg(1, ctx, phash);
  bpf_probe_read_user(&added.hash, sizeof(added.hash), phash);
  bpf_usdt_readarg(2, ctx, &added.vsize);
  bpf_usdt_readarg(3, ctx, &added.fee);

  added_events.perf_submit(ctx, &added, sizeof(added));
  return 0;
}

int trace_removed(struct pt_regs *ctx) {
  struct removed_event removed = {};
  void *phash = NULL, *preason = NULL;
  bpf_usdt_readarg(1, ctx, phash);
  bpf_probe_read_user(&removed.hash, sizeof(removed.hash), phash);
  bpf_usdt_readarg(1, ctx, preason);
  bpf_probe_read_user_str(&removed.reason, sizeof(removed.reason), preason);
  bpf_usdt_readarg(3, ctx, &removed.vsize);
  bpf_usdt_readarg(4, ctx, &removed.fee);
  bpf_usdt_readarg(5, ctx, &removed.entry_time);

  removed_events.perf_submit(ctx, &removed, sizeof(removed));
  return 0;
}

int trace_rejected(struct pt_regs *ctx) {
  struct rejected_event rejected = {};
  void *phash = NULL, *preason = NULL;
  bpf_usdt_readarg(1, ctx, phash);
  bpf_probe_read_user(&rejected.hash, sizeof(rejected.hash), phash);
  bpf_usdt_readarg(1, ctx, preason);
  bpf_probe_read_user_str(&rejected.reason, sizeof(rejected.reason), preason);
  rejected_events.perf_submit(ctx, &rejected, sizeof(rejected));
  return 0;
}

int trace_replaced(struct pt_regs *ctx) {
  struct replaced_event replaced = {};
  void *phash_replaced = NULL, *phash_replacement = NULL;
  bpf_usdt_readarg(1, ctx, phash_replaced);
  bpf_probe_read_user(&replaced.replaced_hash, sizeof(replaced.replaced_hash), phash_replaced);
  bpf_usdt_readarg(2, ctx, &replaced.replaced_vsize);
  bpf_usdt_readarg(3, ctx, &replaced.replaced_fee);
  bpf_usdt_readarg(4, ctx, &replaced.replaced_entry_time);
  bpf_usdt_readarg(5, ctx, phash_replacement);
  bpf_probe_read_user(&replaced.replacement_hash, sizeof(replaced.replacement_hash), phash_replacement);
  bpf_usdt_readarg(6, ctx, &replaced.replacement_vsize);
  bpf_usdt_readarg(7, ctx, &replaced.replacement_fee);

  replaced_events.perf_submit(ctx, &replaced, sizeof(replaced));
  return 0;
}
"""


def main(pid):
    print(f"Hooking into bitcoind with pid {pid}")
    bitcoind_with_usdts = USDT(pid=int(pid))

    # attaching the trace functions defined in the BPF program
    # to the tracepoints
    bitcoind_with_usdts.enable_probe(probe="mempool:added", fn_name="trace_added")
    bitcoind_with_usdts.enable_probe(probe="mempool:removed", fn_name="trace_removed")
    bitcoind_with_usdts.enable_probe(probe="mempool:replaced", fn_name="trace_replaced")
    bitcoind_with_usdts.enable_probe(probe="mempool:rejected", fn_name="trace_rejected")
    bpf = BPF(text=PROGRAM, usdt_contexts=[bitcoind_with_usdts])

    events = []

    def get_timestamp():
        return datetime.now(timezone.utc)

    def handle_added(_, data, size):
        event = bpf["added_events"].event(data)
        events.append((get_timestamp(), "added", event))

    def handle_removed(_, data, size):
        event = bpf["removed_events"].event(data)
        events.append((get_timestamp(), "removed", event))

    def handle_rejected(_, data, size):
        event = bpf["rejected_events"].event(data)
        events.append((get_timestamp(), "rejected", event))

    def handle_replaced(_, data, size):
        event = bpf["replaced_events"].event(data)
        events.append((get_timestamp(), "replaced", event))

    bpf["added_events"].open_perf_buffer(handle_added)
    # By default, open_perf_buffer uses eight pages for a buffer, making for a total
    # buffer size of 32k on most machines. In practice, this size is insufficient:
    # Each `mempool:removed` event takes up 57 bytes in the buffer (32 bytes for txid,
    # 9 bytes for removal reason, and 8 bytes each for vsize and fee). Full blocks
    # contain around 2k transactions, requiring a buffer size of around 114kB. To cover
    # this amount, 32 4k pages are required.
    bpf["removed_events"].open_perf_buffer(handle_removed, page_cnt=32)
    bpf["rejected_events"].open_perf_buffer(handle_rejected)
    bpf["replaced_events"].open_perf_buffer(handle_replaced)

    curses.wrapper(loop, bpf, events)


def loop(screen, bpf, events):
    dashboard = Dashboard(screen)
    while True:
        try:
            bpf.perf_buffer_poll(timeout=50)
            dashboard.render(events)
        except KeyboardInterrupt:
            exit()


class Dashboard:
    """Visualization of mempool state using ncurses."""

    INFO_WIN_HEIGHT = 2
    EVENT_WIN_HEIGHT = 7

    def __init__(self, screen):
        screen.nodelay(True)
        curses.curs_set(False)
        self._screen = screen
        self._time_started = datetime.now(timezone.utc)
        self._timestamps = {"added": [], "removed": [], "rejected": [], "replaced": []}
        self._event_history = {"added": 0, "removed": 0, "rejected": 0, "replaced": 0}
        self._init_windows()

    def _init_windows(self):
        """Initialize all windows."""
        self._init_info_win()
        self._init_event_count_win()
        self._init_event_rate_win()
        self._init_event_log_win()

    @staticmethod
    def create_win(x, y, height, width, title=None):
        """Helper function to create generic windows and decorate them with box and title if requested."""
        win = curses.newwin(height, width, x, y)
        if title:
            win.box()
            win.addstr(0, 2, title, curses.A_BOLD)
        return win

    def _init_info_win(self):
        """Create and populate the info window."""
        self._info_win = Dashboard.create_win(
            x=0, y=1, height=Dashboard.INFO_WIN_HEIGHT, width=22
        )
        self._info_win.addstr(0, 0, "Mempool Monitor", curses.A_REVERSE)
        self._info_win.addstr(1, 0, "Press CTRL-C to stop.", curses.A_NORMAL)
        self._info_win.refresh()

    def _init_event_count_win(self):
        """Create and populate the event count window."""
        self._event_count_win = Dashboard.create_win(
            x=3, y=1, height=Dashboard.EVENT_WIN_HEIGHT, width=37, title="Event count"
        )
        header = " {:<8} {:>8} {:>7} {:>7} "
        self._event_count_win.addstr(
            1, 1, header.format("Event", "total", "1 min", "10 min"), curses.A_UNDERLINE
        )
        self._event_count_win.refresh()

    def _init_event_rate_win(self):
        """Create and populate the event rate window."""
        self._event_rate_win = Dashboard.create_win(
            x=3, y=40, height=Dashboard.EVENT_WIN_HEIGHT, width=42, title="Event rate"
        )
        header = " {:<8} {:>9} {:>9} {:>9} "
        self._event_rate_win.addstr(
            1, 1, header.format("Event", "total", "1 min", "10 min"), curses.A_UNDERLINE
        )
        self._event_rate_win.refresh()

    def _init_event_log_win(self):
        """Create windows showing event log. This comprises a dummy boxed window and an
        inset window so line breaks don't overwrite box."""
        # dummy boxed window
        num_rows, num_cols = self._screen.getmaxyx()
        space_above = Dashboard.INFO_WIN_HEIGHT + 1 + Dashboard.EVENT_WIN_HEIGHT + 1
        box_win_height = num_rows - space_above
        box_win_width = num_cols - 2
        win_box = Dashboard.create_win(
            x=space_above,
            y=1,
            height=box_win_height,
            width=box_win_width,
            title="Event log",
        )
        # actual logging window
        log_lines = box_win_height - 2  # top and bottom box lines
        log_line_len = box_win_width - 2 - 1  # box lines and left padding
        win = win_box.derwin(log_lines, log_line_len, 1, 2)
        win.idlok(True)
        win.scrollok(True)
        win_box.refresh()
        win.refresh()
        self._event_log_win_box = win_box
        self._event_log_win = win

    def calculate_metrics(self, events):
        """Calculate count and rate metrics."""
        count, rate = {}, {}
        for event_ts, event_type, event_data in events:
            self._timestamps[event_type].append(event_ts)
        for event_type, ts in self._timestamps.items():
            # remove timestamps older than ten minutes but keep track of their
            # count for the 'total' metric
            #
            self._event_history[event_type] += len(
                [t for t in ts if Dashboard.timestamp_age(t) >= 600]
            )
            ts = [t for t in ts if Dashboard.timestamp_age(t) < 600]
            self._timestamps[event_type] = ts
            # count metric
            count_1m = len([t for t in ts if Dashboard.timestamp_age(t) < 60])
            count_10m = len(ts)
            count_total = self._event_history[event_type] + len(ts)
            count[event_type] = (count_total, count_1m, count_10m)
            # rate metric
            runtime = Dashboard.timestamp_age(self._time_started)
            rate_1m = count_1m / min(60, runtime)
            rate_10m = count_10m / min(600, runtime)
            rate_total = count_total / runtime
            rate[event_type] = (rate_total, rate_1m, rate_10m)
        return count, rate

    def _update_event_count(self, count):
        """Update the event count window."""
        w = self._event_count_win
        row_format = " {:<8} {:>6}tx {:>5}tx {:>5}tx "
        for line, metric in enumerate(["added", "removed", "replaced", "rejected"]):
            w.addstr(2 + line, 1, row_format.format(metric, *count[metric]))
        w.refresh()

    def _update_event_rate(self, rate):
        """Update the event rate window."""
        w = self._event_rate_win
        row_format = " {:<8} {:>5.1f}tx/s {:>5.1f}tx/s {:>5.1f}tx/s "
        for line, metric in enumerate(["added", "removed", "replaced", "rejected"]):
            w.addstr(2 + line, 1, row_format.format(metric, *rate[metric]))
        w.refresh()

    def _update_event_log(self, events):
        """Update the event log window."""
        w = self._event_log_win
        for event in events:
            w.addstr(Dashboard.parse_event(event) + "\n")
        w.refresh()

    def render(self, events):
        """Render the dashboard."""
        count, rate = self.calculate_metrics(events)
        self._update_event_count(count)
        self._update_event_rate(rate)
        self._update_event_log(events)
        events.clear()

    @staticmethod
    def parse_event(event):
        """Converts events into human-readable messages"""

        ts_dt, type_, data = event
        ts = ts_dt.strftime("%H:%M:%SZ")
        if type_ == "added":
            return (
                f"{ts} added {bytes(data.hash)[::-1].hex()}"
                f" with feerate {data.fee/data.vsize:.2f} sat/vB"
                f" ({data.fee} sat, {data.vsize} vbytes)"
            )

        if type_ == "removed":
            return (
                f"{ts} removed {bytes(data.hash)[::-1].hex()}"
                f" with feerate {data.fee/data.vsize:.2f} sat/vB"
                f" ({data.fee} sat, {data.vsize} vbytes)"
                f" received {ts_dt.timestamp()-data.entry_time:.1f} seconds ago"
                f": {data.reason.decode('UTF-8')}"
            )

        if type_ == "rejected":
            return (
                f"{ts} rejected {bytes(data.hash)[::-1].hex()}"
                f": {data.reason.decode('UTF-8')}"
            )

        if type_ == "replaced":
            return (
                f"{ts} replaced {bytes(data.replaced_hash)[::-1].hex()}"
                f" with feerate {data.replaced_fee/data.replaced_vsize:.2f} sat/vB"
                f" received {ts_dt.timestamp()-data.replaced_entry_time:.1f} seconds ago"
                f" ({data.replaced_fee} sat, {data.replaced_vsize} vbytes)"
                f" with {bytes(data.replacement_hash)[::-1].hex()}"
                f" with feerate {data.replacement_fee/data.replacement_vsize:.2f} sat/vB"
                f" ({data.replacement_fee} sat, {data.replacement_vsize} vbytes)"
            )

        raise NotImplementedError("Unsupported event type: {type_}")

    @staticmethod
    def timestamp_age(timestamp):
        """Return age of timestamp in seconds."""
        return (datetime.now(timezone.utc) - timestamp).total_seconds()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("USAGE: ", sys.argv[0], "<pid of bitcoind>")
        exit(1)

    pid = sys.argv[1]
    main(pid)
