# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Capture USDT arguments with GDB running the node as its child."""

import json
from pathlib import Path
from types import SimpleNamespace


class USDT:
    """Wrap a node before starting it; read() drains captured probe events.

    Probe fields are (name, kind) pairs: integers (i), strings (s), hashes
    in display order (h), or message bytes (b, length in argument five).
    GDB copies pointed-to data before resuming the node. Each restart appends
    to the same event file so shutdown probes remain available to the test.
    """

    def __init__(self, node, probes):
        self.probes = probes
        self.path = node.datadir_path / "usdt.jsonl"
        self.path.write_text("")
        self.offset = 0
        script = node.datadir_path / "capture.gdb"
        # The bitcoin launcher execs the daemon; its own ELF has no probes.
        wrapped = node.binaries.paths.bitcoin_cmd is not None
        launch = "catch exec\nrun\ndelete breakpoints\n" if wrapped else ""
        script.write_text(
            "set pagination off\nset confirm off\nset debuginfod enabled off\n"
            "set disable-randomization off\n"
            "set print thread-events off\npython\nimport gdb, os, runpy\n"
            # LeakSanitizer cannot suspend threads already traced by GDB.
            "gdb.execute('set environment LSAN_OPTIONS ' + os.environ.get('LSAN_OPTIONS', '') + ':detect_leaks=0')\n"
            "end\n" + launch + "python\n"
            f"runpy.run_path({str(Path(__file__).resolve())!r}, "
            f"init_globals={{'events_path': {str(self.path)!r}, 'probes': {probes!r}}}, run_name='__gdb__')\n"
            "end\n" + ("continue\n" if wrapped else "run\n")
        )
        node.args = ["gdb", "-q", "-nx", "--batch", "--return-child-result", "-x", str(script), "--args", *node.args]

    def read(self):
        events = []
        with self.path.open() as stream:
            stream.seek(self.offset)
            while line := stream.readline():
                # Leave an incomplete final record for the next read.
                if not line.endswith("\n"):
                    break
                row = json.loads(line)
                if "error" in row:
                    raise AssertionError(row["error"])
                fields = {}
                for (name, kind), value in zip(self.probes[row["probe"]], row["args"], strict=True):
                    fields[name] = bytes.fromhex(value) if kind == "b" else value
                events.append(SimpleNamespace(probe=row["probe"], **fields))
                self.offset = stream.tell()
        return events

    def clear(self):
        self.read()


def record_probes():
    import gdb  # type: ignore[import]
    import re
    import subprocess

    output = open(globals()["events_path"], "a", buffering=1)
    char_pointer = gdb.lookup_type("char").pointer()

    def read_operand(description):
        # GDB 17.2 rejects GCC's r8b..r15b names, calling them r8l..r15l.
        # Read the unmodified ELF operand only for that specific parser error.
        size, operand = description.split("@", 1)
        size = int(size)
        arg_type = gdb.lookup_type(("signed " if size < 0 else "unsigned ") +
                                   {1: "char", 2: "short", 4: "int", 8: "long long"}[abs(size)])
        if operand.startswith("%"):
            register = re.sub(r"^(r(?:[89]|1[0-5]))b$", r"\1l", operand[1:])
            value = gdb.parse_and_eval(f"${register}")
        elif operand.startswith("$"):
            value = gdb.Value(int(operand[1:], 0))
        else:
            address = re.fullmatch(r"(-?\d*)\(%(\w+)\)", operand)
            if address is None:
                raise AssertionError(f"Unsupported USDT operand: {description}")
            offset, register = address.groups()
            value = gdb.Value(int(gdb.parse_and_eval(f"${register}")) + int(offset or 0)).cast(arg_type.pointer()).dereference()
        return int(value.cast(arg_type))

    class Probe(gdb.Breakpoint):
        def __init__(self, name, fields):
            super().__init__(f"-probe-stap {name}", internal=True)
            self.probe_name = name
            self.fields = fields
            self.operands = None

        def stop(self):
            try:
                try:
                    count = int(gdb.parse_and_eval("$_probe_argc"))
                    raw = [int(gdb.parse_and_eval(f"$_probe_arg{i}")) for i in range(count)]
                except gdb.error as error:
                    if not re.match(r"Invalid register name .r(?:[89]|1[0-5])b", str(error)):
                        raise
                    if self.operands is None:
                        notes = subprocess.check_output(["readelf", "--notes", gdb.current_progspace().filename], text=True)
                        layouts = {f"{provider}:{name}": args.split() for provider, name, args in
                                   re.findall(r"Provider: (\w+)\n\s+Name: (\w+).*?Arguments: ([^\n]*)", notes, re.S)}
                        self.operands = layouts[self.probe_name]
                    raw = [read_operand(operand) for operand in self.operands]
                    count = len(raw)
                if count != len(self.fields):
                    raise AssertionError(f"{self.probe_name}: expected {len(self.fields)} arguments, got {count}")
                args = []
                for (_, field), value in zip(self.fields, raw):
                    if field == "s":
                        value = gdb.Value(value).cast(char_pointer).string(encoding="utf-8")
                    elif field == "h":
                        value = bytes(gdb.selected_inferior().read_memory(value, 32))[::-1].hex()
                    elif field == "b":
                        message_size = raw[4]
                        # Empty messages, e.g. verack, have no bytes to read.
                        # GDB's read_memory requires a strictly positive size.
                        if message_size == 0:
                            value = ""
                        else:
                            value = bytes(gdb.selected_inferior().read_memory(value, message_size)).hex()
                    args.append(value)
                output.write(json.dumps({"probe": self.probe_name, "args": args}) + "\n")
                return False
            except Exception as error:
                message = f"{self.probe_name}: {error}"
                output.write(json.dumps({"error": message}) + "\n")
                gdb.write(message + "\n", gdb.STDERR)
                gdb.execute("quit 1")

    for name, fields in globals()["probes"].items():
        Probe(name, fields)


if __name__ == "__gdb__":
    record_probes()
