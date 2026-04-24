#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Single source of truth for TxGraph USDT tracing probe definitions.

This module defines all TxGraph tracepoint opcodes, argument types, and
binary serialization format. It is imported by txgraph_trace_recorder.py
and analyze_trace.py to avoid duplicating opcode/format definitions.

To add a new TxGraph tracepoint:
  1. Add one entry to the PROBES list below
  2. Add the TRACEPOINT call in src/txgraph.cpp
  3. Add the replay dispatch case in txgraph_replay.cpp
The recorder and analyzer adapt automatically.
"""

import ctypes
import re
import struct

MAX_ELEMENTS = 64

# Binary format type metadata (for scalar types only)
_TYPE_SIZE = {"u8": 1, "u32": 4, "i32": 4, "u64": 8, "i64": 8}
_TYPE_FMT = {"u8": "<B", "u32": "<I", "i32": "<i", "u64": "<Q", "i64": "<q"}
_WRITE_FN = {
    "u8": lambda f, v: f.write(struct.pack("<B", v & 0xFF)),
    "u32": lambda f, v: f.write(struct.pack("<I", v & 0xFFFFFFFF)),
    "i32": lambda f, v: f.write(struct.pack("<i", ctypes.c_int32(v).value)),
    "u64": lambda f, v: f.write(struct.pack("<Q", v)),
    "i64": lambda f, v: f.write(struct.pack("<q", ctypes.c_int64(v).value)),
}

# Mask for extracting unsigned values from u64 BCC event fields
_UINT_MASK = {"u8": 0xFF, "u32": 0xFFFFFFFF, "u64": 0xFFFFFFFFFFFFFFFF}


def _is_array_type(type_str):
    """Check if type is a variable-length array like 'u32[count]'."""
    return "[" in type_str


def _parse_array_type(type_str):
    """Parse 'u32[count]' into ('u32', 'count')."""
    m = re.match(r"(\w+)\[(\w+)\]", type_str)
    return m.group(1), m.group(2)


def _probe_is_varlen(args):
    """Check if a probe has any variable-length array args."""
    return any(_is_array_type(t) for _, t in args)


def _count_arrays(args):
    """Count number of array-type args in a probe definition."""
    return sum(1 for _, t in args if _is_array_type(t))


# ---------------------------------------------------------------------------
# Probe definitions: (name, opcode, [(arg_name, type_str)])
#
# - name:    USDT probe name (matches txgraph:<name> in src/txgraph.cpp)
# - opcode:  unique byte identifying this operation in the binary trace
# - args:    ordered list of (name, type) pairs defining the binary payload
#
# Scalar types: u8, u32, i32, u64, i64
# Array types:  u32[count_field] — variable-length array of u32, length
#               given by a preceding scalar arg named count_field.
#               The USDT tracepoint passes the array as a pointer (read
#               via bpf_probe_read_user into the BCC event elements buffer).
# ---------------------------------------------------------------------------
PROBES = [
    ("init",                      0x00, [("max_cluster_count", "u32"), ("max_cluster_size", "u64"), ("acceptable_cost", "u64")]),
    ("add_transaction",           0x01, [("graph_idx", "u32"), ("fee", "i64"), ("size", "i32")]),
    ("remove_transaction",        0x02, [("graph_idx", "u32")]),
    ("add_dependency",            0x03, [("parent", "u32"), ("child", "u32")]),
    ("set_transaction_fee",       0x04, [("graph_idx", "u32"), ("fee", "i64")]),
    ("unlink_ref",                0x05, [("graph_idx", "u32")]),
    ("get_block_builder",         0x10, []),
    ("do_work",                   0x11, [("max_cost", "u64")]),
    ("compare_main_order",        0x12, [("idx_a", "u32"), ("idx_b", "u32")]),
    ("get_ancestors",             0x13, [("graph_idx", "u32"), ("level", "u8")]),
    ("get_descendants",           0x14, [("graph_idx", "u32"), ("level", "u8")]),
    ("get_ancestors_union",       0x15, [("count", "u32"), ("level", "u8"), ("indices", "u32[count]")]),
    ("get_descendants_union",     0x16, [("count", "u32"), ("level", "u8"), ("indices", "u32[count]")]),
    ("get_cluster",               0x17, [("graph_idx", "u32"), ("level", "u8")]),
    ("get_main_chunk_feerate",    0x18, [("graph_idx", "u32")]),
    ("count_distinct_clusters",   0x19, [("count", "u32"), ("level", "u8"), ("indices", "u32[count]")]),
    ("get_main_memory_usage",     0x1a, []),
    ("get_worst_main_chunk",      0x1b, []),
    ("get_main_staging_diagrams", 0x1c, []),
    ("trim",                      0x1d, []),
    ("is_oversized",              0x1e, [("level", "u8")]),
    ("exists",                    0x1f, [("graph_idx", "u32"), ("level", "u8")]),
    ("start_staging",             0x20, []),
    ("abort_staging",             0x21, []),
    ("commit_staging",            0x22, []),
    ("get_individual_feerate",    0x23, [("graph_idx", "u32")]),
    ("get_transaction_count",     0x24, [("level", "u8")]),
]


# ---------------------------------------------------------------------------
# Derived lookup tables and constants
# ---------------------------------------------------------------------------

PROBES_BY_OPCODE = {}
for _name, _opcode, _args in PROBES:
    PROBES_BY_OPCODE[_opcode] = (_name, _opcode, _args)
    # Module-level constants: OP_INIT=0x00, OP_ADD_TRANSACTION=0x01, ...
    globals()[f"OP_{_name.upper()}"] = _opcode

# Maximum number of array args across all probes (drives BPF buffer sizing).
MAX_ARRAYS = max((_count_arrays(a) for _, _, a in PROBES), default=0)

# Total elements buffer size: each array arg gets a full MAX_ELEMENTS slot.
# Array k occupies elements[k*MAX_ELEMENTS .. (k+1)*MAX_ELEMENTS-1].
ELEMENTS_BUFFER_SIZE = max(MAX_ELEMENTS * MAX_ARRAYS, MAX_ELEMENTS)


# ---------------------------------------------------------------------------
# Binary format I/O
# ---------------------------------------------------------------------------

def write_event(f, e):
    """Write one trace event in TXGTRACE binary format.

    Args:
        f: writable binary file
        e: BCC perf event with .opcode (u8), .arg0/.arg1/.arg2 (u64),
           and .elements (u64 array) attributes

    Multiple array args are supported: array k occupies
    elements[k*MAX_ELEMENTS .. (k+1)*MAX_ELEMENTS-1], so each array
    gets the full MAX_ELEMENTS capacity without truncation.
    """
    probe = PROBES_BY_OPCODE.get(e.opcode)
    if probe is None:
        return
    _, opcode, args = probe
    f.write(struct.pack("<B", opcode))
    arg_vals = [e.arg0, e.arg1, e.arg2]
    written = {}  # arg_name -> unsigned value (for array count references)
    val_idx = 0
    array_idx = 0
    for arg_name, arg_type in args:
        if _is_array_type(arg_type):
            base_type, count_field = _parse_array_type(arg_type)
            count = min(written[count_field], MAX_ELEMENTS)
            offset = array_idx * MAX_ELEMENTS
            for i in range(count):
                _WRITE_FN[base_type](f, e.elements[offset + i])
            array_idx += 1
        else:
            val = arg_vals[val_idx]
            _WRITE_FN[arg_type](f, val)
            mask = _UINT_MASK.get(arg_type, 0xFFFFFFFFFFFFFFFF)
            written[arg_name] = val & mask
            val_idx += 1


def read_event(f, opcode):
    """Read one trace event payload from TXGTRACE binary format.

    Args:
        f: readable binary file (positioned after the opcode byte)
        opcode: the opcode byte already read

    Returns:
        dict of {arg_name: value}, or None on EOF/short read.
        Array args are returned as lists.

    Each array count is capped at MAX_ELEMENTS to match the BPF
    recording layout (each array gets a full MAX_ELEMENTS slot).
    """
    probe = PROBES_BY_OPCODE.get(opcode)
    if probe is None:
        return None
    _, _, args = probe
    result = {}
    for arg_name, arg_type in args:
        if _is_array_type(arg_type):
            base_type, count_field = _parse_array_type(arg_type)
            count = min(result[count_field], MAX_ELEMENTS)
            sz = _TYPE_SIZE[base_type]
            fmt = _TYPE_FMT[base_type]
            arr = []
            for _ in range(count):
                raw = f.read(sz)
                if len(raw) < sz:
                    return None
                arr.append(struct.unpack(fmt, raw)[0])
            result[arg_name] = arr
        else:
            sz = _TYPE_SIZE[arg_type]
            raw = f.read(sz)
            if len(raw) < sz:
                return None
            result[arg_name] = struct.unpack(_TYPE_FMT[arg_type], raw)[0]
    return result
