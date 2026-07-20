#!/usr/bin/env python3
# Copyright (c) 2026 The Syscoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""NEVM connect should only follow successful Core consensus checks."""

from io import BytesIO
from threading import Thread
import random
import struct
import time

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, force_finish_mnsync
from test_framework.blocktools import create_block, create_coinbase, add_witness_commitment
from test_framework.messages import (
    COIN,
    CNEVMBlock,
    CNEVMBlockConnect,
    CNEVMBlockDisconnect,
    CTxOut,
    hash256,
    ser_string,
    ser_uint256,
    ser_vector,
    tx_from_hex,
    uint256_from_str,
)

try:
    import zmq
except ImportError:
    pass


class FeatureNEVMConnectAfterConsensus(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=True, legacy=False)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[
            "-whitelist=noban@127.0.0.1",
            "-nevmstartheight=1",
            "-mncollateral=100",
            "-dip3params=1000:1000",
            "-par=2",
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_syscoind_zmq()
        self.options.descriptors = True
        self.default_wallet_name = "default_wallet"
        self.skip_if_no_wallet()

    def _start_zmq_responder(self, address):
        self._zmq_ctx = zmq.Context()
        self._zmq_sock = self._zmq_ctx.socket(zmq.REP)
        self._zmq_sock.bind(address)
        self._zmq_sock.setsockopt(zmq.RCVTIMEO, 1000)
        self._zmq_running = True
        self._connect_syshashes = []
        self._disconnect_syshashes = []
        self._last_nevm_block_data = b"nevmblock"

        def _loop():
            while self._zmq_running:
                try:
                    parts = self._zmq_sock.recv_multipart()
                except zmq.Again:
                    continue
                except zmq.ContextTerminated:
                    break
                except zmq.ZMQError:
                    if not self._zmq_running:
                        break
                    continue
                if not parts:
                    continue
                topic = parts[0]
                payload = parts[1] if len(parts) > 1 else b""
                if topic == b"nevmcomms":
                    self._zmq_sock.send_multipart([b"nevmcomms", b"ack"])
                elif topic == b"nevmblock":
                    h = hash256(str(random.randint(-0x80000000, 0x7FFFFFFF)).encode())
                    u = uint256_from_str(h)
                    nevm_block = CNEVMBlock()
                    nevm_block.nBlockHash = u
                    nevm_block.nTxRoot = u
                    nevm_block.nReceiptRoot = u
                    nevm_block.vchNEVMBlockData = self._last_nevm_block_data
                    self._zmq_sock.send_multipart([b"nevmblock", nevm_block.serialize()])
                elif topic == b"nevmblockinfo":
                    self._zmq_sock.send_multipart(
                        [b"nevmblockinfo", str(self.nodes[0].getblockcount()).encode()]
                    )
                elif topic == b"nevmconnect":
                    try:
                        nevm_connect = CNEVMBlockConnect()
                        nevm_connect.deserialize(BytesIO(payload))
                        self._connect_syshashes.append(nevm_connect.sysblockhash)
                    except Exception as e:
                        self.log.warning("failed to decode nevmconnect: %s", e)
                        self._connect_syshashes.append(-1)
                    self._zmq_sock.send_multipart([b"nevmconnect", b"connected"])
                elif topic == b"nevmdisconnect":
                    try:
                        nevm_disconnect = CNEVMBlockDisconnect()
                        nevm_disconnect.deserialize(BytesIO(payload))
                        self._disconnect_syshashes.append(nevm_disconnect.sysblockhash)
                    except Exception:
                        self._disconnect_syshashes.append(-1)
                    self._zmq_sock.send_multipart([b"nevmdisconnect", b"disconnected"])
                else:
                    self._zmq_sock.send_multipart([topic, b"ack"])

        self._zmq_thread = Thread(target=_loop, daemon=True)
        self._zmq_thread.start()

    def _stop_zmq_responder(self):
        self._zmq_running = False
        if hasattr(self, "_zmq_thread"):
            self._zmq_thread.join(timeout=5)
        if hasattr(self, "_zmq_sock"):
            self._zmq_sock.close(linger=0)
        if hasattr(self, "_zmq_ctx"):
            self._zmq_ctx.destroy(linger=0)

    def _nonzero_connects_since(self, start_len):
        return [h for h in self._connect_syshashes[start_len:] if h not in (0, -1)]

    def _serialize_nevm_block(self, block, nevm_data):
        # Core CBlockHeader does not emit an extra NEVM byte; encode explicitly.
        assert block.is_nevm()
        r = b""
        r += struct.pack("<i", block.nVersion)
        r += ser_uint256(block.hashPrevBlock)
        r += ser_uint256(block.hashMerkleRoot)
        r += struct.pack("<I", block.nTime)
        r += struct.pack("<I", block.nBits)
        r += struct.pack("<I", block.nNonce)
        r += ser_vector(block.vtx, "serialize_with_witness")
        r += ser_string(nevm_data)
        return r

    def _build_bad_cb_amount_block(self, node):
        tmpl = node.getblocktemplate({"rules": ["segwit"]})
        base_extra = bytes.fromhex(tmpl.get("default_witness_commitment_extra", ""))
        if base_extra == b"":
            raise AssertionError("getblocktemplate missing default_witness_commitment_extra")
        if b"nevm" not in base_extra:
            raise AssertionError("getblocktemplate extra missing NEVM tag")
        if (tmpl.get("version", 0) & (1 << 7)) == 0:
            raise AssertionError("getblocktemplate version missing VERSION_NEVM")

        coinbase = create_coinbase(height=tmpl["height"])
        coinbase.nVersion = tmpl.get("version_coinbase", coinbase.nVersion)
        coinbase.vout[0].nValue = tmpl["coinbasevalue"] + COIN
        for mn_out in tmpl.get("masternode", []):
            coinbase.vout.append(CTxOut(mn_out["amount"], bytes.fromhex(mn_out["script"])))
        for sb_out in tmpl.get("superblock", []):
            coinbase.vout.append(CTxOut(sb_out["amount"], bytes.fromhex(sb_out["script"])))
        coinbase.extraData = base_extra
        coinbase.rehash()

        txlist = [tx_from_hex(e["data"]) for e in tmpl.get("transactions", [])]
        block = create_block(tmpl=tmpl, coinbase=coinbase, txlist=txlist)
        add_witness_commitment(block, nonce=0)
        block.solve()
        return block

    def run_test(self):
        address = "tcp://127.0.0.1:29601"
        self._start_zmq_responder(address)
        try:
            self.extra_args[0].append(f"-zmqpubnevm={address}")
            self.extra_args[0].append("-debug=zmq")
            self.restart_node(0, self.extra_args[0])
            force_finish_mnsync(self.nodes[0])

            self.generate(self.nodes[0], 5)
            tip_before = self.nodes[0].getbestblockhash()
            height_before = self.nodes[0].getblockcount()

            # Rejected late (coinbase value) must not emit a nonzero nevmconnect.
            bad_block = self._build_bad_cb_amount_block(self.nodes[0])
            time.sleep(0.2)
            connect_len = len(self._connect_syshashes)
            disconnect_len = len(self._disconnect_syshashes)

            raw = self._serialize_nevm_block(bad_block, self._last_nevm_block_data)
            result = self.nodes[0].submitblock(hexdata=raw.hex())
            time.sleep(0.5)

            assert_equal(self.nodes[0].getbestblockhash(), tip_before)
            assert_equal(self.nodes[0].getblockcount(), height_before)
            assert_equal(result, "bad-cb-amount")
            assert_equal(self._nonzero_connects_since(connect_len), [])
            assert_equal(self._disconnect_syshashes[disconnect_len:], [])
        finally:
            self._stop_zmq_responder()


if __name__ == "__main__":
    FeatureNEVMConnectAfterConsensus().main()
