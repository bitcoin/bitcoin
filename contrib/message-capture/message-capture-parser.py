#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Parse message capture binary files.  To be used in conjunction with -capturemessages."""

import argparse
import os
import shutil
import sys
from io import BytesIO
import json
from pathlib import Path
from typing import Any, Optional

sys.path.append(os.path.join(os.path.dirname(__file__), '../../test/functional'))

from test_framework.messages import ser_uint256     # noqa: E402
from test_framework.p2p import MESSAGEMAP           # noqa: E402

TIME_SIZE = 8
LENGTH_SIZE = 4
MSGTYPE_SIZE = 12

# The test framework classes stores hashes as large ints in many cases.
# These are variables of type uint256 in core.
# There isn't a way to distinguish between a large int and a large int that is actually a blob of bytes.
# As such, they are itemized here.
# Any variables with these names that are of type int are actually uint256 variables.
# (These can be easily found by looking for calls to deser_uint256, deser_uint256_vector, and uint256_from_str in messages.py)
HASH_INTS = [
    "blockhash",
    "block_hash",
    "hash",
    "hashMerkleRoot",
    "hashPrevBlock",
    "hashstop",
    "prev_header",
    "sha256",
    "stop_hash",
]

HASH_INT_VECTORS = [
    "hashes",
    "headers",
    "vHave",
    "vHash",
]


class ProgressBar:
    def __init__(self, total: float):
        self.total = total
        self.running = 0

    def set_progress(self, progress: float):
        cols = shutil.get_terminal_size()[0]
        if cols <= 12:
            return
        max_blocks = cols - 9
        num_blocks = int(max_blocks * progress)
        print('\r[ {}{} ] {:3.0f}%'
              .format('#' * num_blocks,
                      ' ' * (max_blocks - num_blocks),
                      progress * 100),
              end ='')

    def update(self, more: float):
        self.running += more
        self.set_progress(self.running / self.total)


def to_jsonable(obj: Any) -> Any:
    if hasattr(obj, "__dict__"):
        return obj.__dict__
    elif hasattr(obj, "__slots__"):
        ret = {}    # type: Any
        for slot in obj.__slots__:
            val = getattr(obj, slot, None)
            if slot in HASH_INTS and isinstance(val, int):
                ret[slot] = ser_uint256(val).hex()
            elif slot in HASH_INT_VECTORS and all(isinstance(a, int) for a in val):
                ret[slot] = [ser_uint256(a).hex() for a in val]
            else:
                ret[slot] = to_jsonable(val)
        return ret
    elif isinstance(obj, list):
        return [to_jsonable(a) for a in obj]
    elif isinstance(obj, bytes):
        return obj.hex()
    else:
        return obj


def process_file(path: str, messages: list[Any], recv: bool, progress_bar: Optional[ProgressBar]) -> None:
    with open(path, 'rb') as f_in:
        if progress_bar:
            bytes_read = 0

        while True:
            if progress_bar:
                # Update progress bar
                diff = f_in.tell() - bytes_read - 1
                progress_bar.update(diff)
                bytes_read = f_in.tell() - 1

            # Read the Header
            tmp_header_raw = f_in.read(TIME_SIZE + LENGTH_SIZE + MSGTYPE_SIZE)
            if not tmp_header_raw:
                break
            tmp_header = BytesIO(tmp_header_raw)
            time = int.from_bytes(tmp_header.read(TIME_SIZE), "little")      # type: int
            msgtype = tmp_header.read(MSGTYPE_SIZE).split(b'\x00', 1)[0]     # type: bytes
            length = int.from_bytes(tmp_header.read(LENGTH_SIZE), "little")  # type: int

            # Start converting the message to a dictionary
            msg_dict = {}
            msg_dict["direction"] = "recv" if recv else "sent"
            msg_dict["time"] = time
            msg_dict["size"] = length   # "size" is less readable here, but more readable in the output

            msg_ser = BytesIO(f_in.read(length))

            # Determine message type
            if msgtype not in MESSAGEMAP:
                # Unrecognized message type
                try:
                    msgtype_tmp = msgtype.decode()
                    if not msgtype_tmp.isprintable():
                        raise UnicodeDecodeError
                    msg_dict["msgtype"] = msgtype_tmp
                except UnicodeDecodeError:
                    msg_dict["msgtype"] = "UNREADABLE"
                msg_dict["body"] = msg_ser.read().hex()
                msg_dict["error"] = "Unrecognized message type."
                messages.append(msg_dict)
                print(f"WARNING - Unrecognized message type {msgtype} in {path}", file=sys.stderr)
                continue

            # Deserialize the message
            msg = MESSAGEMAP[msgtype]()
            msg_dict["msgtype"] = msgtype.decode()

            try:
                msg.deserialize(msg_ser)
            except KeyboardInterrupt:
                raise
            except Exception:
                # Unable to deserialize message body
                msg_ser.seek(0, os.SEEK_SET)
                msg_dict["body"] = msg_ser.read().hex()
                msg_dict["error"] = "Unable to deserialize message."
                messages.append(msg_dict)
                print(f"WARNING - Unable to deserialize message in {path}", file=sys.stderr)
                continue

            # Convert body of message into a jsonable object
            if length:
                msg_dict["body"] = to_jsonable(msg)
            messages.append(msg_dict)

        if progress_bar:
            # Update the progress bar to the end of the current file
            # in case we exited the loop early
            f_in.seek(0, os.SEEK_END)   # Go to end of file
            diff = f_in.tell() - bytes_read - 1
            progress_bar.update(diff)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog="EXAMPLE \n\t{0} -o out.json <data-dir>/message_capture/**/*.dat".format(sys.argv[0]),
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        "capturepaths",
        nargs='+',
        help="binary message capture files to parse.")
    parser.add_argument(
        "-o", "--output",
        help="output file.  If unset print to stdout")
    parser.add_argument(
        "-n", "--no-progress-bar",
        action='store_true',
        help="disable the progress bar.  Automatically set if the output is not a terminal")
    args = parser.parse_args()
    capturepaths = [Path.cwd() / Path(capturepath) for capturepath in args.capturepaths]
    output = Path.cwd() / Path(args.output) if args.output else False
    use_progress_bar = (not args.no_progress_bar) and sys.stdout.isatty()

    messages = []   # type: list[Any]
    if use_progress_bar:
        total_size = sum(capture.stat().st_size for capture in capturepaths)
        progress_bar = ProgressBar(total_size)
    else:
        progress_bar = None

    for capture in capturepaths:
        process_file(str(capture), messages, "recv" in capture.stem, progress_bar)

    messages.sort(key=lambda msg: msg['time'])

    if use_progress_bar:
        progress_bar.set_progress(1)

    jsonrep = json.dumps(messages)
    if output:
        with open(str(output), 'w+') as f_out:
            f_out.write(jsonrep)
    else:
        print(jsonrep)

if __name__ == "__main__":
    main()
