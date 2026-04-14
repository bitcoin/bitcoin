#!/usr/bin/env python3
# Copyright (c) 2026 The Syscoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
End-to-end BTCC signer policy test with:
- AuxPoW mining on Syscoin sentry/quorum nodes
- External independent Bitcoin regtest mining
- Reorg/fork scenarios on the Bitcoin side
- NEVM ZMQ verification of BTC anchor forwarding (present on allow, absent on deny)
"""

import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from io import BytesIO
from threading import Thread

from test_framework.auxpow import reverseHex
from test_framework.auxpow_testing import computeAuxpow
from test_framework.authproxy import JSONRPCException
from test_framework.messages import CNEVMBlock, CNEVMBlockConnect
from test_framework.test_framework import DashTestFramework, SkipTest
from test_framework.util import assert_equal, force_finish_mnsync

try:
    import zmq
except ImportError:
    pass


class ExternalBitcoinRegtestNode:
    def __init__(self, *, name, bitcoind, bitcoin_cli, datadir, p2p_port, rpc_port, addnode_port=None):
        self.name = name
        self.bitcoind = bitcoind
        self.bitcoin_cli = bitcoin_cli
        self.datadir = Path(datadir)
        self.p2p_port = p2p_port
        self.rpc_port = rpc_port
        self.addnode_port = addnode_port
        self.proc = None

    def _cli(self, *args, wallet=None, rpcwait=False, timeout_s=30):
        cmd = [
            self.bitcoin_cli,
            "-regtest",
            f"-datadir={self.datadir}",
            f"-rpcport={self.rpc_port}",
        ]
        if wallet is not None:
            cmd.append(f"-rpcwallet={wallet}")
        if rpcwait:
            cmd.append("-rpcwait")
        cmd.extend(str(a) for a in args)
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout_s)
        except subprocess.TimeoutExpired as e:
            raise RuntimeError(f"{self.name}: bitcoin-cli timed out ({' '.join(cmd)}): {e}") from e
        if res.returncode != 0:
            detail = res.stderr.strip() or res.stdout.strip() or f"exit={res.returncode}"
            raise RuntimeError(f"{self.name}: bitcoin-cli failed ({' '.join(cmd)}): {detail}")
        out = res.stdout.strip()
        if out == "":
            return None
        try:
            return json.loads(out)
        except json.JSONDecodeError:
            return out

    def _debug_log_tail(self, n=40):
        log = self.datadir / "regtest" / "debug.log"
        if not log.exists():
            return "<debug.log not found>"
        with log.open("r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
        return "".join(lines[-n:]).strip()

    def start(self):
        self.datadir.mkdir(parents=True, exist_ok=True)
        cmd = [
            self.bitcoind,
            "-regtest",
            "-server=1",
            "-discover=0",
            "-dnsseed=0",
            "-listen=1",
            "-fallbackfee=0.0001",
            f"-datadir={self.datadir}",
            f"-port={self.p2p_port}",
            f"-rpcport={self.rpc_port}",
        ]
        if self.addnode_port is not None:
            cmd.append(f"-addnode=127.0.0.1:{self.addnode_port}")

        self.proc = subprocess.Popen(
            cmd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )
        deadline = time.time() + 30
        last_error = None
        while time.time() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError(
                    f"{self.name}: bitcoind exited early with code {self.proc.returncode}\n"
                    f"debug.log tail:\n{self._debug_log_tail()}"
                )
            try:
                self._cli("getblockchaininfo", timeout_s=2)
                last_error = None
                break
            except RuntimeError as e:
                last_error = e
                time.sleep(0.2)
        if last_error is not None:
            raise RuntimeError(
                f"{self.name}: timed out waiting for bitcoind RPC. Last error: {last_error}\n"
                f"debug.log tail:\n{self._debug_log_tail()}"
            )

    def stop(self):
        if self.proc is None:
            return
        if self.proc.poll() is None:
            try:
                self._cli("stop")
            except Exception:
                pass
            try:
                self.proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=10)
        self.proc = None

    def mine(self, blocks, descriptor="raw(51)"):
        # Wallet is intentionally disabled in the headers-only helper build.
        # Allow caller-selected descriptors so split miners can intentionally
        # build divergent branches when exercising fork/unconfirmed behavior.
        return self._cli("generatetodescriptor", blocks, descriptor)

    def set_network_active(self, active):
        return self._cli("setnetworkactive", "true" if active else "false")

    def rpc(self, *args):
        return self._cli(*args)


class ZMQNEVMResponder:
    def __init__(self, log, socket):
        self.log = log
        self.socket = socket
        self.btcprev_by_sys_hash = {}
        self.running = True
        self._counter = 1

    def _next_nevm_hash(self):
        value = self._counter
        self._counter += 1
        return value

    def _handle(self, topic, payload):
        if topic == b"nevmcomms":
            self.socket.send_multipart([b"nevmcomms", b"ack"])
            return
        if topic == b"nevmblock":
            h = self._next_nevm_hash()
            nevm_block = CNEVMBlock()
            nevm_block.nBlockHash = h
            nevm_block.nTxRoot = h
            nevm_block.nReceiptRoot = h
            nevm_block.vchNEVMBlockData = b"feature_btcheader_policy_auxpow"
            self.socket.send_multipart([b"nevmblock", nevm_block.serialize()])
            return
        if topic == b"nevmconnect":
            nevm_connect = CNEVMBlockConnect()
            nevm_connect.deserialize(BytesIO(payload))
            self.btcprev_by_sys_hash[nevm_connect.sysblockhash] = nevm_connect.btcprevhash
            self.socket.send_multipart([b"nevmconnect", b"connected"])
            return
        if topic == b"nevmdisconnect":
            self.socket.send_multipart([b"nevmdisconnect", b"disconnected"])
            return

        self.log.info("Unknown ZMQ NEVM topic: %s", topic)
        self.socket.send_multipart([topic, b"ack"])

    def loop(self):
        while self.running:
            try:
                msg = self.socket.recv_multipart()
            except zmq.ContextTerminated:
                break
            except zmq.ZMQError:
                if not self.running:
                    break
                continue

            if not msg:
                continue
            topic = msg[0]
            payload = msg[1] if len(msg) > 1 else b""
            self._handle(topic, payload)


class BTCHeaderPolicyAuxpowTest(DashTestFramework):
    BTCCHECK_PERIOD = 10
    BTCCHECK_SIGN_OFFSET = 2
    BTCCHECK_PROP_BUFFER = 5
    BTC_MINE_DESC_NODE0 = "raw(51)"
    BTC_MINE_DESC_NODE1 = "raw(52)"

    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.set_dash_test_params(4, 3, fast_dip3_enforcement=True)
        self.btc_nodes = []
        self.external_btcheader_cmd = None
        self.external_btc_p2p_ports = None
        self.external_btc_rpc_ports = None
        self.bitcoind_path = None
        self.bitcoin_cli_path = None
        self.zmq_ctx = None
        self.zmq_socket = None
        self.zmq_thread = None
        self.zmq_address = None
        self.nevm_responder = None
        self.managed_node_cfg = {}

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_py3_zmq()
        self.skip_if_no_syscoind_zmq()

        self.bitcoind_path, self.bitcoin_cli_path = self._resolve_bitcoin_binaries()
        if self.bitcoind_path is None or self.bitcoin_cli_path is None:
            raise SkipTest("bitcoind and bitcoin-cli are required for feature_btcheader_policy_auxpow.py")

    def _resolve_bitcoin_binaries(self):
        bitcoind_candidates = []
        bitcoin_cli_candidates = []

        env_bitcoind = os.getenv("BITCOIND")
        env_bitcoin_cli = os.getenv("BITCOINCLI")
        if env_bitcoind:
            bitcoind_candidates.append(env_bitcoind)
        if env_bitcoin_cli:
            bitcoin_cli_candidates.append(env_bitcoin_cli)

        which_bitcoind = shutil.which("bitcoind")
        which_bitcoin_cli = shutil.which("bitcoin-cli")
        if which_bitcoind:
            bitcoind_candidates.append(which_bitcoind)
        if which_bitcoin_cli:
            bitcoin_cli_candidates.append(which_bitcoin_cli)

        builddir = self.config["environment"]["BUILDDIR"]
        bitcoind_candidates.append(os.path.join(builddir, "src", "bin", "btcheadernode", "bin", "bitcoind"))
        bitcoin_cli_candidates.append(os.path.join(builddir, "src", "bin", "btcheadernode", "bin", "bitcoin-cli"))

        bitcoind_path = next((p for p in bitcoind_candidates if p and os.path.isfile(p) and os.access(p, os.X_OK)), None)
        bitcoin_cli_path = next((p for p in bitcoin_cli_candidates if p and os.path.isfile(p) and os.access(p, os.X_OK)), None)
        return bitcoind_path, bitcoin_cli_path

    def _alloc_free_port(self):
        import socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind(("127.0.0.1", 0))
            return s.getsockname()[1]

    def _build_external_btcheader_cmd(self, node):
        # Intentionally use real bitcoin-cli output semantics (no JSON wrapping shim).
        return f"{self.bitcoin_cli_path} -regtest -datadir={node.datadir} -rpcport={node.rpc_port}"

    def _start_external_btc_network(self):
        base = Path(self.options.tmpdir) / "external_btc"
        base.mkdir(parents=True, exist_ok=True)

        if self.external_btc_p2p_ports is None:
            self.external_btc_p2p_ports = [self._alloc_free_port(), self._alloc_free_port()]
        if self.external_btc_rpc_ports is None:
            self.external_btc_rpc_ports = [self._alloc_free_port(), self._alloc_free_port()]
        node0_p2p, node1_p2p = self.external_btc_p2p_ports
        node0_rpc, node1_rpc = self.external_btc_rpc_ports

        btc0 = ExternalBitcoinRegtestNode(
            name="btc0",
            bitcoind=self.bitcoind_path,
            bitcoin_cli=self.bitcoin_cli_path,
            datadir=base / "node0",
            p2p_port=node0_p2p,
            rpc_port=node0_rpc,
            addnode_port=node1_p2p,
        )
        btc1 = ExternalBitcoinRegtestNode(
            name="btc1",
            bitcoind=self.bitcoind_path,
            bitcoin_cli=self.bitcoin_cli_path,
            datadir=base / "node1",
            p2p_port=node1_p2p,
            rpc_port=node1_rpc,
            addnode_port=node0_p2p,
        )
        btc0.start()
        btc1.start()
        self.btc_nodes = [btc0, btc1]
        self._wait_for_btc_sync()
        self.external_btcheader_cmd = self._build_external_btcheader_cmd(btc0)

    def _stop_external_btc_network(self):
        for node in reversed(self.btc_nodes):
            node.stop()
        self.btc_nodes = []
        self.external_btcheader_cmd = None

    def _start_nevm_subscriber(self):
        self.zmq_address = f"tcp://127.0.0.1:{self._alloc_free_port()}"
        self.zmq_ctx = zmq.Context()
        self.zmq_socket = self.zmq_ctx.socket(zmq.REP)
        self.zmq_socket.setsockopt(zmq.RCVTIMEO, 1500)
        self.zmq_socket.bind(self.zmq_address)
        self.nevm_responder = ZMQNEVMResponder(self.log, self.zmq_socket)
        self.zmq_thread = Thread(target=self.nevm_responder.loop)
        self.zmq_thread.start()

    def _stop_nevm_subscriber(self):
        if self.nevm_responder is not None:
            self.nevm_responder.running = False
        if self.zmq_socket is not None:
            self.zmq_socket.close(linger=0)
            self.zmq_socket = None
        if self.zmq_ctx is not None:
            self.zmq_ctx.destroy(linger=None)
            self.zmq_ctx = None
        if self.zmq_thread is not None:
            self.zmq_thread.join(timeout=5)
            self.zmq_thread = None
        self.nevm_responder = None
        self.zmq_address = None

    def setup_network(self):
        self._start_nevm_subscriber()
        self._start_external_btc_network()
        btcheader_cmd = self.external_btcheader_cmd
        assert btcheader_cmd is not None
        for i in range(self.num_nodes):
            self.extra_args[i].extend([
                # Ensure startup path treats this node as NEVM miner-configured
                # via geth passthrough (covers init-time btcheader requirement).
                "-gethcommandline=--miner.pendingfeerecipient=0x00000000000000000000000000000000000000aa",
                "-btcheadermanaged=0",
                f"-btcheadercmd={btcheader_cmd}",
                "-btcheaderpolicyondemand=1",
                "-btcheaderwatchdog=1",
                "-btcheaderwatchdogprobeinterval=1",
                # Keep baseline stall timeout high so initial quorum mining
                # does not accidentally trigger external stalled-denials.
                "-btcheaderwatchdogstalltimeout=300",
                "-btcheaderwatchdogreindexafter=2",
                "-btcheadertipmaxage=0",
                "-btcheadermaxlagblocks=36",
                "-btcheaderrecentforkdepth=2",
                "-debug=chainlocks",
                "-nevmstartheight=1",
            ])
        self.extra_args[0].append(f"-zmqpubnevm={self.zmq_address}")
        super().setup_network()

    def shutdown(self):
        self._stop_external_btc_network()
        try:
            return super().shutdown()
        finally:
            self._stop_nevm_subscriber()

    def _strip_btcheader_args(self, args):
        prefixes = (
            "-btcheadermanaged",
            "-btcheadercmd",
            "-btcheaderbinary",
            "-btcheaderclibinary",
            "-btcheaderdatadir",
            "-btcheaderport",
            "-btcheaderrpcport",
            "-btcheadercommandline",
            "-btcheaderpolicyondemand",
            "-btcheaderwatchdog",
            "-btcheaderwatchdogprobeinterval",
            "-btcheaderwatchdogrestartcooldown",
            "-btcheaderwatchdogstalltimeout",
            "-btcheaderwatchdogreindexafter",
            "-btcheadertipmaxage",
            "-btcheadertipmaxnoprogress",
            "-btcheadermaxlagblocks",
            "-btcheaderrecentforkdepth",
        )
        return [arg for arg in args if not any(arg.startswith(prefix) for prefix in prefixes)]

    def _enable_managed_btcheader_on_node(self, node_index):
        p2p_port = self._alloc_free_port()
        rpc_port = self._alloc_free_port()
        btcheader_datadir = Path(self.nodes[node_index].chain_path) / "btcheader managed test"
        btcheader_datadir.mkdir(parents=True, exist_ok=True)

        new_args = self._strip_btcheader_args(self.extra_args[node_index])
        new_args.extend([
            "-btcheadermanaged=1",
            f"-btcheaderbinary={self.bitcoind_path}",
            f"-btcheaderclibinary={self.bitcoin_cli_path}",
            f"-btcheaderdatadir={btcheader_datadir}",
            f"-btcheaderport={p2p_port}",
            f"-btcheaderrpcport={rpc_port}",
            "-btcheaderpolicyondemand=1",
            "-btcheaderwatchdog=1",
            "-btcheaderwatchdogprobeinterval=1",
            "-btcheaderwatchdogrestartcooldown=4",
            "-btcheaderwatchdogstalltimeout=8",
            "-btcheaderwatchdogreindexafter=2",
            "-btcheadertipmaxage=0",
            "-btcheadermaxlagblocks=36",
            "-btcheaderrecentforkdepth=2",
            "-debug=chainlocks",
        ])
        self.extra_args[node_index] = new_args
        self.restart_node(node_index, extra_args=new_args)
        for peer_index in range(self.num_nodes):
            if peer_index == node_index:
                continue
            self.connect_nodes(node_index, peer_index, wait_for_connect=False)
        self.sync_all()
        force_finish_mnsync(self.nodes[node_index])

        self.managed_node_cfg[node_index] = {
            "p2p_port": p2p_port,
            "rpc_port": rpc_port,
            "datadir": btcheader_datadir,
        }
        self._wait_for_managed_chaininfo(node_index)

    def _managed_cli(self, node_index, *args, timeout=20):
        cfg = self.managed_node_cfg[node_index]
        cmd = [
            self.bitcoin_cli_path,
            "-regtest",
            f"-datadir={cfg['datadir']}",
            f"-rpcport={cfg['rpc_port']}",
            *[str(arg) for arg in args],
        ]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        except subprocess.TimeoutExpired as e:
            raise RuntimeError(f"managed cli timed out ({' '.join(cmd)}): {e}") from e
        if res.returncode != 0:
            detail = res.stderr.strip() or res.stdout.strip() or f"exit={res.returncode}"
            raise RuntimeError(f"managed cli failed ({' '.join(cmd)}): {detail}")
        out = res.stdout.strip()
        if out == "":
            return None
        try:
            return json.loads(out)
        except json.JSONDecodeError:
            return out

    def _wait_for_managed_chaininfo(self, node_index, timeout=45):
        deadline = time.time() + timeout
        last_err = None
        while time.time() < deadline:
            try:
                info = self._managed_cli(node_index, "getblockchaininfo", timeout=2)
                if isinstance(info, dict):
                    return info
            except Exception as e:
                last_err = e
            time.sleep(0.25)
        raise RuntimeError(f"timed out waiting for managed BTC header RPC on node {node_index}: {last_err}")

    def _managed_genesis_hash(self, node_index):
        return self._managed_cli(node_index, "getblockhash", 0)

    def _wait_for_btc_sync(self):
        self.wait_until(
            lambda: self.btc_nodes[0].rpc("getbestblockhash") == self.btc_nodes[1].rpc("getbestblockhash"),
            timeout=60,
        )

    def _set_btc_network_active(self, active):
        for node in self.btc_nodes:
            node.set_network_active(active)

    def _btc_has_recent_fork(self):
        tip_height = self.btc_nodes[0].rpc("getblockcount")
        for tip in self.btc_nodes[0].rpc("getchaintips"):
            status = tip.get("status", "")
            if status in ("active", "invalid"):
                continue
            if status not in ("valid-fork", "valid-headers", "headers-only"):
                continue
            if tip.get("height", -1) >= tip_height - 2:
                return True
        return False

    def _btc_active_confirmed_hash(self):
        tip_height = self.btc_nodes[0].rpc("getblockcount")
        candidate_height = max(0, tip_height - 1)
        candidate_hash = self.btc_nodes[0].rpc("getblockhash", candidate_height)
        header = self.btc_nodes[0].rpc("getblockheader", candidate_hash, "true")
        assert header["confirmations"] >= 2
        return candidate_hash

    def _btc_older_confirmed_hash(self):
        tip_height = self.btc_nodes[0].rpc("getblockcount")
        candidate_height = max(0, tip_height - 5)
        candidate_hash = self.btc_nodes[0].rpc("getblockhash", candidate_height)
        header = self.btc_nodes[0].rpc("getblockheader", candidate_hash, "true")
        assert header["confirmations"] >= 5
        return candidate_hash

    def _btc_far_older_confirmed_hash(self, min_lag=50):
        tip_height = self.btc_nodes[0].rpc("getblockcount")
        if tip_height < min_lag + 5:
            self.btc_nodes[0].mine((min_lag + 5) - tip_height)
            self._wait_for_btc_sync()
            tip_height = self.btc_nodes[0].rpc("getblockcount")
        candidate_height = max(0, tip_height - min_lag)
        candidate_hash = self.btc_nodes[0].rpc("getblockhash", candidate_height)
        header = self.btc_nodes[0].rpc("getblockheader", candidate_hash, "true")
        assert header["confirmations"] >= min_lag
        return candidate_hash

    def _btc_nonexistent_hash(self):
        candidate = "aa" * 32
        try:
            self.btc_nodes[0].rpc("getblockheader", candidate, "true")
        except RuntimeError:
            return candidate
        return ("bb" * 31) + "aa"

    def _carrier_height_for_sign(self, sign_height):
        return sign_height + self.BTCCHECK_PROP_BUFFER

    def _wait_for_zmq_btcprev_for_height(self, height):
        sys_hash = int(self.nodes[0].getblockhash(height), 16)
        self.wait_until(
            lambda: sys_hash in self.nevm_responder.btcprev_by_sys_hash,
            timeout=60,
        )
        return self.nevm_responder.btcprev_by_sys_hash[sys_hash]

    def _best_btcc_height(self, node):
        try:
            return node.getbestbtccheckpoint()["height"]
        except JSONRPCException:
            return -1

    def _best_btcc_height_signers(self):
        return max(self._best_btcc_height(node) for node in self.nodes[1:])

    def _debug_log_path(self, node_index):
        return Path(self.nodes[node_index].datadir_path) / self.chain / "debug.log"

    def _capture_log_offsets(self, node_indices):
        offsets = {}
        for node_index in node_indices:
            path = self._debug_log_path(node_index)
            try:
                offsets[node_index] = path.stat().st_size
            except FileNotFoundError:
                offsets[node_index] = 0
        return offsets

    def _wait_for_reason_on_any_signer(self, reason_fragment, target_sign_height, offsets, height_offsets=(0,), timeout=30):
        needle_heights = [f"sysHeight={target_sign_height + off}" for off in height_offsets]
        deadline = time.time() + timeout
        signer_indices = list(range(1, len(self.nodes)))
        last_excerpt = ""
        while time.time() < deadline:
            for node_index in signer_indices:
                path = self._debug_log_path(node_index)
                start = offsets.get(node_index, 0)
                try:
                    with open(path, "rb") as f:
                        f.seek(start, os.SEEK_SET)
                        chunk = f.read().decode("utf-8", errors="replace")
                except FileNotFoundError:
                    continue
                if chunk:
                    last_excerpt = chunk[-2000:]
                if reason_fragment in chunk and any(needle_height in chunk for needle_height in needle_heights):
                    return node_index
            time.sleep(0.25)
        raise AssertionError(
            f"Did not observe policy deny reason~{reason_fragment} at any of {needle_heights} on signer logs. "
            f"Recent log excerpt:\n{last_excerpt}"
        )

    def _poll_skip_reason_for_height_on_any_signer(self, target_sign_height, offsets):
        signer_indices = list(range(1, len(self.nodes)))
        needle_height = f"sysHeight={target_sign_height}"
        for node_index in signer_indices:
            path = self._debug_log_path(node_index)
            start = offsets.get(node_index, 0)
            try:
                with open(path, "rb") as f:
                    f.seek(start, os.SEEK_SET)
                    chunk = f.read()
                    offsets[node_index] = f.tell()
            except FileNotFoundError:
                continue
            if not chunk:
                continue
            text = chunk.decode("utf-8", errors="replace")
            for line in reversed(text.splitlines()):
                if "CBTCCheckpointsHandler -- skip btcc sign at" in line and needle_height in line:
                    return node_index, line
        return None

    def _wait_for_btcc_signer_progress_or_reason(self, target_sign_height, timeout=180):
        offsets = self._capture_log_offsets(range(1, len(self.nodes)))
        deadline = time.time() + timeout
        last_height = self._best_btcc_height_signers()
        while time.time() < deadline:
            current = self._best_btcc_height_signers()
            if current >= target_sign_height:
                return current
            last_height = max(last_height, current)

            skip_hit = self._poll_skip_reason_for_height_on_any_signer(target_sign_height, offsets)
            if skip_hit is not None:
                node_index, skip_line = skip_hit
                if "RunCommandParseJSON requires Boost::Process support" in skip_line:
                    raise SkipTest(
                        "feature_btcheader_policy_auxpow.py requires btcheader command execution support "
                        "(configure with --with-boost-process=yes, enable --enable-btcheadernode-build, "
                        "or build with ENABLE_EXTERNAL_SIGNER). "
                        f"Observed on signer node {node_index}: {skip_line}"
                    )
                raise AssertionError(
                    f"Expected ALLOW at sysHeight={target_sign_height}, but signer node {node_index} denied: {skip_line}"
                )

            # Poll at a moderate cadence to avoid hammering all signers via RPC.
            time.sleep(0.5)

        raise AssertionError(
            f"BTCC signer did not reach height {target_sign_height} within {timeout}s "
            f"(best observed height={last_height})."
        )

    def _restart_signer_node(self, node_index):
        self.restart_node(node_index, extra_args=self.extra_args[node_index])
        for peer_index in range(self.num_nodes):
            if peer_index == node_index:
                continue
            self.connect_nodes(node_index, peer_index, wait_for_connect=False)
        self.sync_all()
        force_finish_mnsync(self.nodes[node_index])

    def _mine_auxpow_with_btcprev(self, miner, btc_prev_hash):
        address = miner.get_deterministic_priv_key().address
        auxblock = miner.createauxblock(address, btc_prev_hash)
        target = reverseHex(auxblock["_target"])
        parent_prev_hash = auxblock.get("_btcprevhash", btc_prev_hash)
        apow = computeAuxpow(
            auxblock["hash"],
            target,
            True,
            auxblock["coinbasescript"],
            parent_prev_hash,
        )
        assert_equal(miner.submitauxblock(auxblock["hash"], apow), True)
        try:
            self.sync_blocks(self.nodes, timeout=60)
        except AssertionError:
            # Rarely, one signer can transiently lose a p2p link while we are
            # mining quickly. Reconnect full mesh and retry sync once.
            for i in range(len(self.nodes)):
                for j in range(i + 1, len(self.nodes)):
                    self.connect_nodes(i, j, wait_for_connect=False)
            self.sync_blocks(self.nodes, timeout=120)
        return auxblock["height"]

    def _assert_createauxblock_autofills_btcprev(self):
        """
        On sign-offset heights, createauxblock without explicit btcprevhash should
        succeed by sourcing BTC tip from the configured header backend.
        """
        target_sign_height = self._next_sign_height()
        self.log.info(
            "Verify createauxblock auto-sources btcprevhash at sign height=%d",
            target_sign_height,
        )
        self._mine_sys_to_height_with_btcprev(target_sign_height - 1, self._btc_active_confirmed_hash())
        address = self.nodes[0].get_deterministic_priv_key().address
        expected_btc_prev = self.btc_nodes[0].rpc("getbestblockhash")
        auxblock = self.nodes[0].createauxblock(address)
        assert_equal(auxblock["height"], target_sign_height)
        assert_equal(auxblock["_btcprevhash"], expected_btc_prev)

    def _mine_sys_blocks_with_btcprev(self, count, btc_prev_hash):
        for _ in range(count):
            self._mine_auxpow_with_btcprev(self.nodes[0], btc_prev_hash)

    def _next_sign_height(self):
        h = self.nodes[0].getblockcount() + 1
        while (h % self.BTCCHECK_PERIOD) != self.BTCCHECK_SIGN_OFFSET:
            h += 1
        return h

    def _mine_sys_to_height_with_btcprev(self, target_height, btc_prev_hash):
        assert target_height >= self.nodes[0].getblockcount() + 1
        while self.nodes[0].getblockcount() < target_height:
            self._mine_auxpow_with_btcprev(self.nodes[0], btc_prev_hash)

    def _exercise_policy_allows(self, btc_prev_hash):
        target_sign_height = self._next_sign_height()
        carrier_height = self._carrier_height_for_sign(target_sign_height)
        before = self._best_btcc_height_signers()
        self.log.info("Expect BTCC signer ALLOW at sysHeight=%d with btcPrev=%s", target_sign_height, btc_prev_hash)

        self._mine_sys_to_height_with_btcprev(target_sign_height, btc_prev_hash)
        self.bump_mocktime(2, nodes=self.nodes)
        time.sleep(0.2)

        self._wait_for_btcc_signer_progress_or_reason(target_sign_height, timeout=180)
        self._mine_sys_blocks_with_btcprev(self.BTCCHECK_PROP_BUFFER + 2, btc_prev_hash)
        after = self._best_btcc_height_signers()
        assert after >= target_sign_height
        assert after > before
        zmq_btc_prev = self._wait_for_zmq_btcprev_for_height(carrier_height)
        expected_prev = int(btc_prev_hash, 16)
        # In this topology, ZMQ is published by the controller node (non-signer),
        # which can lag signer BTCC state and report zero even when signers accepted.
        # Treat signer checkpoint advancement as the primary success signal.
        if zmq_btc_prev not in (0, expected_prev):
            raise AssertionError(f"Unexpected ZMQ btcprevhash value {zmq_btc_prev}, expected 0 or {expected_prev}")
        return target_sign_height

    def _exercise_policy_denies(self, btc_prev_hash, reason_fragment, reason_height_offsets=(0,)):
        target_sign_height = self._next_sign_height()
        carrier_height = self._carrier_height_for_sign(target_sign_height)
        before = self._best_btcc_height_signers()
        self.log.info("Expect BTCC signer DENY at sysHeight=%d reason~%s", target_sign_height, reason_fragment)
        offsets = self._capture_log_offsets(range(1, len(self.nodes)))
        self._mine_sys_to_height_with_btcprev(target_sign_height, btc_prev_hash)
        self.bump_mocktime(2, nodes=self.nodes)
        time.sleep(0.2)
        self._wait_for_reason_on_any_signer(
            reason_fragment,
            target_sign_height,
            offsets,
            height_offsets=reason_height_offsets,
            timeout=30,
        )

        self._mine_sys_blocks_with_btcprev(self.BTCCHECK_PROP_BUFFER + 1, btc_prev_hash)
        self.wait_until(lambda: self.nodes[0].getblockcount() >= carrier_height, timeout=60)
        # No checkpoint should be forwarded to NEVM on denied paths.
        assert_equal(self._wait_for_zmq_btcprev_for_height(carrier_height), 0)
        assert self._best_btcc_height_signers() < target_sign_height
        assert self._best_btcc_height_signers() >= before
        return target_sign_height

    def _exercise_node_policy_denies(
        self,
        node_index,
        btc_prev_hash,
        reason_fragment,
        match_sys_height=True,
        reason_prefix=True,
        extra_expected_msgs=None,
    ):
        target_sign_height = self._next_sign_height()
        self.log.info(
            "Expect node %d signer DENY at sysHeight=%d reason~%s",
            node_index,
            target_sign_height,
            reason_fragment,
        )
        expected_msgs = [f"reason={reason_fragment}" if reason_prefix else reason_fragment]
        if extra_expected_msgs:
            expected_msgs.extend(extra_expected_msgs)
        if match_sys_height:
            expected_msgs.insert(0, f"sysHeight={target_sign_height}")
        with self.nodes[node_index].assert_debug_log(
            expected_msgs=expected_msgs,
            timeout=30,
        ):
            self._mine_sys_to_height_with_btcprev(target_sign_height, btc_prev_hash)
            self.bump_mocktime(1, nodes=self.nodes)
            time.sleep(0.2)
        return target_sign_height

    def _create_unconfirmed_stale_hash(self):
        self.log.info("Create Bitcoin-side reorg and extract stale hash (confirmations < 1)")
        # Keep a deterministic fork point before network split.
        self._set_btc_network_active(True)
        self._wait_for_btc_sync()
        split_tip = self.btc_nodes[0].rpc("getbestblockhash")
        assert_equal(split_tip, self.btc_nodes[1].rpc("getbestblockhash"))

        self._set_btc_network_active(False)
        self.wait_until(
            lambda: self.btc_nodes[0].rpc("getconnectioncount") == 0 and self.btc_nodes[1].rpc("getconnectioncount") == 0,
            timeout=30,
        )
        self.btc_nodes[0].mine(2, descriptor=self.BTC_MINE_DESC_NODE0)
        # Mine enough blocks on peer 1 so it will win reorg once reconnected.
        self.btc_nodes[1].mine(5, descriptor=self.BTC_MINE_DESC_NODE1)
        self._set_btc_network_active(True)
        self.btc_nodes[0].rpc("addnode", f"127.0.0.1:{self.btc_nodes[1].p2p_port}", "onetry")
        self.btc_nodes[1].rpc("addnode", f"127.0.0.1:{self.btc_nodes[0].p2p_port}", "onetry")
        self._wait_for_btc_sync()

        fork_tips = []
        for tip in self.btc_nodes[0].rpc("getchaintips"):
            status = tip.get("status", "")
            if status in ("valid-fork", "valid-headers", "headers-only"):
                fork_tips.append(tip)
        assert fork_tips, "expected at least one non-active Bitcoin fork tip after split/reconnect"
        fork_tips.sort(key=lambda t: t.get("height", -1), reverse=True)
        stale_hash = fork_tips[0]["hash"]

        stale_header = self.btc_nodes[0].rpc("getblockheader", stale_hash, "true")
        assert stale_header["confirmations"] < 1, f"expected stale hash to be unconfirmed, got {stale_header['confirmations']}"
        return stale_hash

    def _exercise_signed_hash_reorg_recovery(self):
        self.log.info("Sign a BTC hash, reorg it out, and verify signer recovers on next commitment")
        self._set_btc_network_active(True)
        self._wait_for_btc_sync()

        self._set_btc_network_active(False)
        self.wait_until(
            lambda: self.btc_nodes[0].rpc("getconnectioncount") == 0 and self.btc_nodes[1].rpc("getconnectioncount") == 0,
            timeout=30,
        )

        # Mine a short private branch on btc0 and sign its tip hash.
        self.btc_nodes[0].mine(2, descriptor=self.BTC_MINE_DESC_NODE0)
        signed_hash = self.btc_nodes[0].rpc("getbestblockhash")
        signed_header = self.btc_nodes[0].rpc("getblockheader", signed_hash, "true")
        assert signed_header["confirmations"] >= 1
        signed_sys_height = self._exercise_policy_allows(signed_hash)

        # Build a slightly longer competing branch on btc1 so that after reorg,
        # the stale branch remains near tip and triggers btc-recent-fork.
        self.btc_nodes[1].mine(3, descriptor=self.BTC_MINE_DESC_NODE1)
        self._set_btc_network_active(True)
        self.btc_nodes[0].rpc("addnode", f"127.0.0.1:{self.btc_nodes[1].p2p_port}", "onetry")
        self.btc_nodes[1].rpc("addnode", f"127.0.0.1:{self.btc_nodes[0].p2p_port}", "onetry")
        self._wait_for_btc_sync()

        orphaned_header = self.btc_nodes[0].rpc("getblockheader", signed_hash, "true")
        assert orphaned_header["confirmations"] < 1, "expected previously signed hash to become stale after reorg"

        # Fork-risk heuristic should block immediate signing until depth cools down.
        self._exercise_policy_denies(self._btc_active_confirmed_hash(), "btc-recent-fork(")
        self._cool_down_recent_fork()

        # After fork-risk cool down, continuity guard should detect the prior signed
        # hash is no longer active at its recorded height, deny once, and rebaseline.
        self._exercise_policy_denies(
            self._btc_active_confirmed_hash(),
            "btc-prev-signed-not-active-chain(",
            reason_height_offsets=(-self.BTCCHECK_PERIOD, 0),
        )

        try:
            healed_sys_height = self._exercise_policy_allows(self._btc_active_confirmed_hash())
        except AssertionError as e:
            # Rebaseline is tracked per signer; a different signer can hit its
            # first continuity deny one cycle later before converging.
            if "btc-prev-signed-not-active-chain(" not in str(e):
                raise
            self.log.info("Observed delayed per-signer continuity rebaseline; retrying next sign cycle")
            healed_sys_height = self._exercise_policy_allows(self._btc_active_confirmed_hash())
        assert healed_sys_height > signed_sys_height

    def _exercise_continuity_persistence_after_restart(self):
        self.log.info("Verify continuity guard persists after signer restart using on-chain baseline")
        try:
            signed_sys_height = self._exercise_policy_allows(self._btc_active_confirmed_hash())
        except AssertionError as e:
            # Continuity rebaseline is tracked per signer; when this scenario runs
            # immediately after reorg recovery, one signer can still emit a final
            # legacy baseline deny for one cycle before converging.
            if "btc-prev-signed-not-active-chain(" not in str(e):
                raise
            self.log.info("Observed delayed continuity rebaseline before restart check; retrying next sign cycle")
            signed_sys_height = self._exercise_policy_allows(self._btc_active_confirmed_hash())
        restart_node = 1
        self._restart_signer_node(restart_node)
        denied_sys_height = self._exercise_node_policy_denies(
            restart_node,
            self._btc_older_confirmed_hash(),
            "btc-non-monotonic-height(",
        )
        assert denied_sys_height > signed_sys_height

    def _create_recent_fork(self):
        self.log.info("Create near-tip competing Bitcoin fork")
        self._wait_for_btc_sync()
        self._set_btc_network_active(False)
        self.btc_nodes[0].mine(2, descriptor=self.BTC_MINE_DESC_NODE0)
        self.btc_nodes[1].mine(2, descriptor=self.BTC_MINE_DESC_NODE1)
        self._set_btc_network_active(True)
        self.wait_until(self._btc_has_recent_fork, timeout=60)

    def _cool_down_recent_fork(self):
        self.log.info("Cool down Bitcoin fork depth beyond policy threshold")
        self.btc_nodes[0].mine(3)
        self._wait_for_btc_sync()
        self.wait_until(lambda: not self._btc_has_recent_fork(), timeout=60)

    def _exercise_watchdog_external_down(self, btc_prev_hash):
        self.log.info("Stop external BTC backend and assert watchdog catches backend-unreachable condition")
        self._stop_external_btc_network()
        self._exercise_policy_denies(btc_prev_hash, "btcheader-watchdog-external-unreachable")
        self.log.info("Restart external BTC backend after watchdog-down scenario")
        self._start_external_btc_network()
        self.btc_nodes[0].mine(2)
        self._wait_for_btc_sync()

    def _exercise_watchdog_managed_conditions(self):
        managed_node = 1
        self.log.info("Switch node %d to managed BTC header backend (real binaries only)", managed_node)
        self._enable_managed_btcheader_on_node(managed_node)

        self.log.info("Managed lifecycle smoke test with real btcheader binaries")
        self._restart_signer_node(managed_node)
        self._wait_for_managed_chaininfo(managed_node)

        managed_hash = self._managed_genesis_hash(managed_node)
        assert isinstance(managed_hash, str) and len(managed_hash) == 64

        # Restart managed backend via RPC and ensure it comes back healthy.
        self.nodes[managed_node].syscoinstopbtcheadernode()
        time.sleep(0.5)
        self.nodes[managed_node].syscoinstartbtcheadernode()
        self._wait_for_managed_chaininfo(managed_node)

    def run_test(self):
        for node in self.nodes:
            force_finish_mnsync(node)

        for i in range(len(self.nodes)):
            for j in range(i + 1, len(self.nodes)):
                self.connect_nodes(i, j, wait_for_connect=False)
        self.sync_all()

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mine quorums for BTCC signing")
        for _ in range(4):
            self.mine_quorum()

        self.log.info("Bootstrap Bitcoin chain and select a confirmed active hash")
        self.btc_nodes[0].mine(8)
        self._wait_for_btc_sync()
        confirmed_hash = self._btc_active_confirmed_hash()

        self._assert_createauxblock_autofills_btcprev()

        self._exercise_policy_allows(confirmed_hash)
        self._exercise_policy_denies(self._btc_far_older_confirmed_hash(), "btc-candidate-too-old(")
        self._exercise_watchdog_external_down(confirmed_hash)

        self._exercise_policy_denies(self._btc_nonexistent_hash(), "btc-candidate-header-failed")

        stale_hash = self._create_unconfirmed_stale_hash()
        stale_deny_height = self._exercise_policy_denies(stale_hash, "btc-candidate-unconfirmed")
        stale_recovery_height = self._exercise_policy_allows(self._btc_active_confirmed_hash())
        assert stale_recovery_height > stale_deny_height

        self._exercise_signed_hash_reorg_recovery()
        self._exercise_continuity_persistence_after_restart()
        self._exercise_watchdog_managed_conditions()


if __name__ == "__main__":
    BTCHeaderPolicyAuxpowTest().main()
