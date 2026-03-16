#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test signet miner tool"""

import json
import os.path
import shlex
import subprocess
import sys
import time

from test_framework.blocktools import DIFF_1_N_BITS, SIGNET_HEADER
from test_framework.key import ECKey
from test_framework.script_util import CScript, key_to_p2wpkh_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wallet_importprivkey,
)
from test_framework.wallet_util import bytes_to_wif


CHALLENGE_PRIVATE_KEY = (42).to_bytes(32, 'big')

def get_segwit_commitment(node):
    coinbase = node.getblock(node.getbestblockhash(), 2)['tx'][0]
    commitment = coinbase['vout'][1]['scriptPubKey']['hex']
    assert_equal(commitment[0:12], '6a24aa21a9ed')
    return commitment

def get_signet_commitment(segwit_commitment):
    for el in CScript.fromhex(segwit_commitment):
        if isinstance(el, bytes) and el[0:4] == SIGNET_HEADER:
            return el[4:].hex()
    return None

class SignetMinerTest(BitcoinTestFramework):
    def set_test_params(self):
        self.chain = "signet"
        self.setup_clean_chain = True
        self.num_nodes = 4

        # generate and specify signet challenge (simple p2wpkh script)
        privkey = ECKey()
        privkey.set(CHALLENGE_PRIVATE_KEY, True)
        pubkey = privkey.get_pubkey().get_bytes()
        challenge = key_to_p2wpkh_script(pubkey)

        self.extra_args = [
            [f'-signetchallenge={challenge.hex()}'],
            ["-signetchallenge=51"], # OP_TRUE
            ["-signetchallenge=60"], # OP_16
            ["-signetchallenge=202cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"], # sha256("hello")
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_cli()
        self.skip_if_no_wallet()
        self.skip_if_no_bitcoin_util()

    def setup_network(self):
        self.setup_nodes()
        # Nodes with different signet networks are not connected

    # generate block with signet miner tool
    def mine_block(self, node):
        n_blocks = node.getblockcount()
        base_dir = self.config["environment"]["SRCDIR"]
        signet_miner_path = os.path.join(base_dir, "contrib", "signet", "miner")
        rpc_argv = node.binaries.rpc_argv() + [f"-datadir={node.cli.datadir}"]
        util_argv = node.binaries.util_argv() + ["grind"]
        subprocess.run([
                sys.executable,
                signet_miner_path,
                f'--cli={shlex.join(rpc_argv)}',
                'generate',
                f'--address={node.getnewaddress()}',
                f'--grind-cmd={shlex.join(util_argv)}',
                f'--nbits={DIFF_1_N_BITS:08x}',
                f'--set-block-time={int(time.time())}',
                '--poolnum=99',
            ], check=True, stderr=subprocess.STDOUT)
        assert_equal(node.getblockcount(), n_blocks + 1)

    # generate block using the signet miner tool genpsbt and solvepsbt commands
    def mine_block_manual(self, node, *, sign):
        n_blocks = node.getblockcount()
        base_dir = self.config["environment"]["SRCDIR"]
        signet_miner_path = os.path.join(base_dir, "contrib", "signet", "miner")
        rpc_argv = node.binaries.rpc_argv() + [f"-datadir={node.cli.datadir}"]
        util_argv = node.binaries.util_argv() + ["grind"]
        base_cmd = [
            sys.executable,
            signet_miner_path,
            f'--cli={shlex.join(rpc_argv)}',
        ]

        template = node.getblocktemplate(dict(rules=["signet","segwit"]))
        genpsbt = subprocess.run(base_cmd + [
                'genpsbt',
                f'--address={node.getnewaddress()}',
                '--poolnum=98',
            ], check=True, text=True, input=json.dumps(template), capture_output=True)
        psbt = genpsbt.stdout.strip()
        if sign:
            self.log.debug("Sign the PSBT")
            res = node.walletprocesspsbt(psbt=psbt, sign=True, sighashtype='ALL')
            assert res['complete']
            psbt = res['psbt']
        solvepsbt = subprocess.run(base_cmd + [
                'solvepsbt',
                f'--grind-cmd={shlex.join(util_argv)}',
            ], check=True, text=True, input=psbt, capture_output=True)
        node.submitblock(solvepsbt.stdout.strip())
        assert_equal(node.getblockcount(), n_blocks + 1)

    def run_test(self):
        self.log.info("Signet node with single signature challenge")
        node = self.nodes[0]
        # import private key needed for signing block
        wallet_importprivkey(node, bytes_to_wif(CHALLENGE_PRIVATE_KEY), 0)
        self.mine_block(node)
        # MUST include signet commitment
        assert get_signet_commitment(get_segwit_commitment(node))

        self.log.info("Mine manually using genpsbt and solvepsbt")
        self.mine_block_manual(node, sign=True)
        assert get_signet_commitment(get_segwit_commitment(node))

        node = self.nodes[1]
        self.log.info("Signet node with trivial challenge (OP_TRUE)")
        self.mine_block(node)
        # MAY omit signet commitment (BIP 325). Do so for better compatibility
        # with signet unaware mining software and hardware.
        assert get_signet_commitment(get_segwit_commitment(node)) is None

        node = self.nodes[2]
        self.log.info("Signet node with trivial challenge (OP_16)")
        self.mine_block(node)
        assert get_signet_commitment(get_segwit_commitment(node)) is None

        node = self.nodes[3]
        self.log.info("Signet node with trivial challenge (push sha256 hash)")
        self.mine_block(node)
        assert get_signet_commitment(get_segwit_commitment(node)) is None

        self.log.info("Manual mining with a trivial challenge doesn't require a PSBT")
        self.mine_block_manual(node, sign=False)
        assert get_signet_commitment(get_segwit_commitment(node)) is None


if __name__ == "__main__":
    SignetMinerTest(__file__).main()
