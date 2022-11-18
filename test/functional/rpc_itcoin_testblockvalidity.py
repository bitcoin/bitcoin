#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''Test testblockvalidity rpc.'''
import os
import subprocess

from test_framework.conftest import ITCOIN_CORE_ROOT_DIR, ITCOIN_PBFT_ROOT_DIR
from test_framework.messages import CBlock, CBlockHeader, from_hex, ser_uint256
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner
miner = import_miner()


class TestBlockValidityTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1

        # parent class attributes
        self.requires_wallet = True
        self.setup_clean_chain = True

    @classmethod
    def _build_args(cls, bcli, address: str) -> miner.DoGenerateArgs:
        """Build arguments for the current node."""
        args = miner.DoGenerateArgs()
        args.max_blocks = None
        args.ongoing = False
        args.poisson = False
        args.min_nbits = True
        args.multiminer = None
        args.cli = ""
        args.grind_cmd = f"{ITCOIN_CORE_ROOT_DIR}/src/bitcoin-util grind"

        args.nbits = None
        args.bcli = bcli
        args.address = address
        return args

    def _get_node_bcli(self, node):
        """
        Get the Bitcoin CLI.

        :return: the callable function.
        """
        return lambda command=None, *args, **kwargs: miner.bitcoin_cli(node.cli, command, *args, **kwargs)

    def _grind_block(self, args, block) -> CBlock:
        grind_cmd = args.grind_cmd
        assert grind_cmd is not None
        self.log.debug(f"Grinding block...")
        headhex = CBlockHeader.serialize(block).hex()
        cmd = grind_cmd.split(" ") + [headhex]
        subp_env = dict(os.environ, LD_LIBRARY_PATH=f'$LD_LIBRARY_PATH:{ITCOIN_PBFT_ROOT_DIR}/usrlocal/lib')
        newheadhex = subprocess.run(cmd, env=subp_env, stdout=subprocess.PIPE, input=b"", check=True).stdout.strip()
        newhead = from_hex(CBlockHeader(), newheadhex.decode('utf8'))
        block.nNonce = newhead.nNonce
        block.rehash()
        self.log.debug(f"Block merkle root (function which includes signatures) after mining = { ser_uint256(block.calc_merkle_root()).hex() }")
        return block

    def _log_test_success(self):
        self.log.info("Test success")

    def run_test(self):
        node = self.nodes[0]
        address = node.getnewaddress()
        args = self._build_args(self._get_node_bcli(node), address)

        self.test_valid_block_passes_the_check(args)
        self.test_dummy_hex_string_gives_block_decoding_error(args)
        self.test_block_without_coinbase_gives_block_decoding_error(args)
        self.test_block_invalid(args)
        self.test_block_already_present_gives_duplicate_error(args)

    def test_valid_block_passes_the_check(self, args: miner.DoGenerateArgs) -> None:
        """Check that a block generated from 'getblocktemplate' is valid."""
        self.log.info("Check that a block generated from 'getblocktemplate' is valid")
        block, _, _ = miner.do_generate_next_block(args)
        block_hex = block.serialize().hex()
        is_block_valid = args.bcli("testblockvalidity", block_hex)
        assert is_block_valid == ""
        self._log_test_success()

    def test_dummy_hex_string_gives_block_decoding_error(self, args: miner.DoGenerateArgs) -> None:
        """Check that a dummy hex string gives a block decoding error."""
        self.log.info("Check that a dummy hex string is not a valid block.")
        assert_raises_rpc_error(-22, "Block decode failed", args.bcli, "testblockvalidity", "abcdef")
        self._log_test_success()

    def test_block_without_coinbase_gives_block_decoding_error(self, args) -> None:
        """Check that a block without coinbase tx gives a block decoding error."""
        self.log.info("Check that a block without coinbase tx gives a block decoding error.")
        block, _, _ = miner.do_generate_next_block(args)
        block.vtx = []
        block_hex = block.serialize().hex()
        assert_raises_rpc_error(-22, "Block does not start with a coinbase", args.bcli, "testblockvalidity", block_hex)
        self._log_test_success()

    def test_block_already_present_gives_duplicate_error(self, args):
        """Check that a block already present gives a 'duplicate' error."""
        self.log.info("Check that a block already present gives a 'duplicate' error.")
        block, _, _ = miner.do_generate_next_block(args)
        block_hex = block.serialize().hex()
        args.bcli("submitblock", block_hex)
        # now test for validity of the same block
        result = args.bcli("testblockvalidity", block_hex)
        assert result == "duplicate"
        self._log_test_success()

    def test_block_invalid(self, args):
        """Check that an invalid block gives the right BIP-22 error."""
        self.log.info("Check that an invalid block gives the right BIP-22 error.")
        block, _, _ = miner.do_generate_next_block(args)
        # set different script without updating merkle root
        block.vtx[0].vin[0].scriptSig = b"wrong"
        block_hex = block.serialize().hex()
        # now test for validity of the same block
        assert_raises_rpc_error(-25, "bad-txnmrklroot, hashMerkleRoot mismatch", args.bcli, "testblockvalidity", block_hex)

        block.vtx[0].rehash()
        block.hashMerkleRoot = block.calc_merkle_root()
        block = self._grind_block(args, block)
        assert_raises_rpc_error(-25, "bad-cb-height, block height mismatch in coinbase", args.bcli, "testblockvalidity", block.serialize().hex())

        # regenerate block
        block, _, _ = miner.do_generate_next_block(args)
        # ... but set wrong nBits
        correctnBits = block.nBits
        block.nBits = 1
        block.rehash()
        assert_raises_rpc_error(-25, "bad-diffbits, incorrect proof of work", args.bcli, "testblockvalidity", block.serialize().hex())

        self._log_test_success()


if __name__ == '__main__':
    TestBlockValidityTest().main()
