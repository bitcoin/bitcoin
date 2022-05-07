"""Base test module for Signet test utilities."""
import configparser
import os
import tempfile
from pathlib import Path
from typing import List, Optional, Sequence, Callable, Any, Dict, Set

from test_framework.address import base58_to_byte

from test_framework.conftest import (
    DEFAULT_CONFIG_PATH,
    DEFAULT_RPC_TIMEOUT,
    DEFAULT_RPC_TIMEOUT_FACTOR,
    # TODO: this should use the default wallet already used in the test framework
    # DEFAULT_WALLET_NAME,
    REGTEST,
    SIGNET, ITCOIN_CORE_ROOT_DIR,
)
from test_framework.key import ECKey
from test_framework.messages import CBlock
from test_framework.test_framework import TMPDIR_PREFIX, BitcoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import PortSeed, initialize_datadir, assert_equal

# import itcoin's miner
from .itcoin_abs_import import import_miner
miner = import_miner()

class NodeWrapper:
    """Wrapper to a 'TestNode' instance."""

    def __init__(self, node: TestNode, key_pair: TestNode.AddressKeyPair) -> None:
        """
        Initialize a node wrapper.

        :param node: the test node created by the BitcoinTestFramework class
        """
        self._node = node

        self._address = self._node.getnewaddress()
        self._key_pair = key_pair
        self.args = self._build_args(self._get_node_bcli(), self._address)

        # TODO: this should use the default wallet already used in the test framework
        # self._import_private_key()

    @property
    def test_node(self) -> TestNode:
        """Get the test node."""
        return self._node

    @property
    def address(self) -> str:
        """Get the node's address."""
        return self._address

    @property
    def key_pair(self) -> TestNode.AddressKeyPair:
        """Get the node's key pair."""
        return self._key_pair

    def get_blockchaininfo_field(self, field_name: str) -> Any:
        """
        Get a blockchain info field from the node's view.

        :param field_name: the field name to read from blockchaininfo response.
        :return: the value
        """
        b_info = self._node.getblockchaininfo()
        if field_name not in b_info:
            raise ValueError(f"field name {field_name} not valid")
        return b_info[field_name]

    def _get_node_bcli(self) -> Callable:
        """
        Get the Bitcoin CLI.

        :return: the callable function.
        """
        return lambda command=None, *args, **kwargs: miner.bitcoin_cli(self.test_node.cli.send_cli, command, *args, **kwargs)

    def _import_private_key(self) -> None:
        """Import the private key into the node's wallet."""
        self._node.importprivkey(self.key_pair.key)

    @classmethod
    def _build_args(cls, bcli: Callable, address: str) -> miner.DoGenerateArgs:
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


class BaseSignetTest(BitcoinTestFramework):

    @property
    def signet_key_pairs(self) -> Sequence[TestNode.AddressKeyPair]:
        """Get the key pairs."""
        return self._key_pairs

    def node(self, i: int) -> NodeWrapper:
        """Get the ith test node, wrapped."""
        self._validate_node_id(i)
        return self._node_wrappers[i]

    def set_test_params(self, set_signet_challenge_as_extra_arg=True) -> None:
        """Configure the test."""
        self.signet_num_signers: int
        self.signet_num_signatures: int

        # Validate parameters
        if not (0 < self.signet_num_signatures <= self.signet_num_signers):
            raise ValueError(f"expected 0 < K <= N, got K={self.signet_num_signatures} and N={self.signet_num_signers}, please configure signet_num_signers and signet_num_signatures")

        # parent class attributes
        self.chain = SIGNET
        self.setup_clean_chain = True

        # supposed to be private
        self._key_pairs: Sequence[TestNode.AddressKeyPair]
        self._node_wrappers: Sequence[NodeWrapper]
        self._pubkey_to_index: Dict[str, int] = {}
        self._index_to_pubkey: List[str]

        # Get deterministic keypairs for signet
        self._key_pairs = self.deterministic_key_pairs(self.signet_num_signers)

        # Extract the public keys
        node_keys = [ECKey() for i in range(self.signet_num_signers)]
        for i, key in enumerate(node_keys):
            key_b = base58_to_byte(self._key_pairs[i].key)[0][:-1]
            key.set(key_b, True)
        pubkeys = [key.get_pubkey().get_bytes().hex() for key in node_keys]

        # we use str.lower to avoid case-sensitive ordering in case of misformatted input
        self._index_to_pubkey = sorted(pubkeys, key=str.lower)
        self._pubkey_to_index = dict(map(reversed, enumerate(self._index_to_pubkey)))
        self.signet_challenge = self.make_signet_challenge(self._index_to_pubkey, self.signet_num_signatures)

        if set_signet_challenge_as_extra_arg:
            arg_signetchallenge = f"-signetchallenge={self.signet_challenge}"
            self.extra_args = [[arg_signetchallenge]] * self.num_nodes

    def setup_nodes(self):
        """Set up nodes."""
        super().setup_nodes()

        self._node_wrappers = tuple(NodeWrapper(self.nodes[i], self._key_pairs[i]) for i in range(self.signet_num_signers))

    @classmethod
    def make_signet_challenge(cls, sorted_pubkeys: Sequence[str], k: int) -> str:
        """
        Compute the signet challenge.

        Given a collection of public keys and the number of signature,
        computer the signet challenge.

        Note: we require a sorted public key list due to known limitation of OP_CHECKMULTISIG:

            https://btcinformation.org/en/developer-reference#opcodes

        :param sorted_pubkeys: the sequence of public keys.
        :param k: the signature count.
        :return: the signet challenge.
        """
        # check pubkeys are sorted
        assert all(sorted_pubkeys[i] < sorted_pubkeys[i+1] for i in range(len(sorted_pubkeys) - 1))
        # remove leading '0x'
        k_hex = hex(k)[2:]
        signet_challenge = f"5{k_hex}"
        n = len(sorted_pubkeys)
        n_hex = hex(n)[2:]
        for i, pubkey in enumerate(sorted_pubkeys):
            signet_challenge += "21" + pubkey
        signet_challenge = signet_challenge + f"5{n_hex}"
        signet_challenge = signet_challenge + "ae"
        return signet_challenge

    @classmethod
    def deterministic_key_pairs(cls, n: int) -> Sequence[TestNode.AddressKeyPair]:
        result: List[TestNode.AddressKeyPair] = TestNode.PRIV_KEYS[:n]
        return result

    @classmethod
    def generate_key_pairs(cls, n: int) -> Sequence[TestNode.AddressKeyPair]:
        """
        Generate key pairs.

        This function spawns a temporary Bitcoind process and
        apply n times the procedure explained at this link:

            https://en.bitcoin.it/wiki/Signet#Generating_keys_used_for_signing_a_block

        :param n: the number of key pairs to generate.
        :return: a sequence of key pairs
        """
        result: List[TestNode.AddressKeyPair] = []
        # start a temporary bitcoind process
        with tempfile.TemporaryDirectory(prefix=TMPDIR_PREFIX) as tmpdir, BitcoindNode(
            tmpdir, configfile=DEFAULT_CONFIG_PATH
        ) as node:
            for _ in range(n):
                key_pair = node.new_key_pair()
                result.append(key_pair)
        return result

    def _validate_node_id(self, node_id: int) -> None:
        """Validate the node id."""
        assert 0 <= node_id < self.signet_num_signers, f"expected 0 <= node_id < {self.signet_num_signers}, got node id {node_id}"

    def get_blockchaininfo_field_from_node(self, node_id: int, field_name: str) -> Any:
        """
        Get a blockchain info field from a node's view.

        :param node_id: the node id
        :param field_name: the field name to read from blockchaininfo response.
        :return: the value
        """
        return self.node(node_id).get_blockchaininfo_field(field_name)

    def assert_blockchaininfo_property_forall_nodes(self, field_name: str, expected_value: Any) -> None:
        """
        Assert that a 'getblockchaininfo' property is equal to an expected value, for every node.

        :param field_name: the field name.
        :param expected_value: the expected value.
        """
        for i in range(self.signet_num_signers):
            self.assert_blockchaininfo_property(i, field_name, expected_value)

    def assert_blockchaininfo_property(self, node_id: int, field_name: str, expected_value: Any):
        """
        Assert that a 'getblockchaininfo' property is equal to an expected value for a certain node.

        :param node_id: the id of the node.
        :param field_name: the field name.
        :param expected_value: the expected value.
        :raises: ValueError: if the field name is not in the object returned by the RPC method 'getblockchaininfo'.
        """
        value = self.get_blockchaininfo_field_from_node(node_id, field_name)
        try:
            assert_equal(value, expected_value)
        except AssertionError as e:
            raise AssertionError(f"field value for node {node_id} is not as expected: {e}")

    def do_multisign_block(self, signers: Set[int], block: CBlock, signet_challenge: str) -> CBlock:
        """
        Sign a block with multiple signers.

        :param signers: the set of signers' ids.
        :param block: the block to sign.
        :param signet_challenge: the signet challenge to use.
        :return: the signet block.
        """
        assert len(signers) >= self.signet_num_signatures, f"number of signers is not enough; expected >={self.signet_num_signatures}, got {len(signers)}"
        assert all(0 <= signer_id < self.signet_num_signers for signer_id in signers), "set of signer contains an invalid signer id"
        sorted_signers = sorted(signers)

        # get Bitcoin CLI wrappers in order
        node_args = [self.node(node_id).args for node_id in sorted_signers]
        args0 = node_args[0]

        sign_results = [miner.do_sign_block_partial(args, block, signet_challenge) for args in node_args]
        psbts, complete_flags = zip(*sign_results)
        combined_psbt = miner.do_combine_psbt(args0, *psbts)
        finalized_psbt, is_final = miner.do_finalize_psbt(args0, combined_psbt)
        signed_block = miner.extract_block(args0, finalized_psbt)
        return signed_block

    def run_test(self) -> None:
        """
        Run the test.

        This method must be overridden and implemented by subclasses.
        """
        raise NotImplementedError


class BitcoindNode:
    """
    An extension of the 'TestNode' utility.

    In particular, it supports execution in a with-statement context.
    Use this class like:

    with BitcoindNode("some/directory", "path/to/config") as node:
        node.rpc.getblockcount()
    """

    def __init__(
        self,
        directory: str,
        configfile: str = DEFAULT_CONFIG_PATH,
        rpc_timeout: int = DEFAULT_RPC_TIMEOUT,
    ) -> None:
        """Initialize the Bitcoind node."""
        self.directory = directory
        self.configfile = configfile
        self.rpc_timeout = rpc_timeout

        self.config = self._parse_config(configfile)
        self.chain = REGTEST
        self.index = 0

        self.node_dir = Path(directory, f"node{self.index}")

        self._rpc: Optional[TestNode] = None
        self._old_port_seed: Optional[int] = None

    @classmethod
    def _parse_config(cls, configfile: str) -> configparser.ConfigParser:
        """Parse a configuration file."""
        config = configparser.ConfigParser()
        config.read_file(open(configfile))
        return config

    @property
    def rpc(self) -> TestNode:
        """Get the RPC wrapper object."""
        return self._rpc

    @property
    def bitcoind(self) -> str:
        """Get the bitcoind binary."""
        return str(
            Path(self.config["environment"]["BUILDDIR"]).joinpath(
                "src",
                "bitcoind" + self.config["environment"]["EXEEXT"],
            )
        )

    @property
    def bitcoin_cli(self) -> str:
        """get the bitcoin-cli binary."""
        return str(
            Path(self.config["environment"]["BUILDDIR"]).joinpath(
                "src",
                "bitcoin-cli" + self.config["environment"]["EXEEXT"],
            )
        )

    def new_key_pair(self) -> TestNode.AddressKeyPair:
        """
        Get a new key pair.

        The implementation follows the instruction reported here:

            https://en.bitcoin.it/wiki/Signet#Generating_keys_used_for_signing_a_block

        :return: the key pair for the signet.
        """
        addr = self._rpc.getnewaddress()
        privkey = self._rpc.dumpprivkey(addr)
        address_info = self._rpc.getaddressinfo(addr)
        pubkey = address_info["pubkey"]
        return TestNode.AddressKeyPair(pubkey, privkey)

    def __enter__(self) -> "BitcoindNode":
        """Start the node."""
        self._old_port_seed = PortSeed.n
        PortSeed.n = os.getpid()
        initialize_datadir(self.directory, self.index, self.chain)
        self._rpc = TestNode(
            self.index,
            str(self.node_dir),
            chain=self.chain,
            rpchost=None,
            timewait=DEFAULT_RPC_TIMEOUT,
            timeout_factor=DEFAULT_RPC_TIMEOUT_FACTOR,
            bitcoind=self.bitcoind,
            bitcoin_cli=self.bitcoin_cli,
            coverage_dir=None,
            cwd=self.directory,
            extra_args=[],
        )
        self._rpc.start()
        self._rpc.wait_for_rpc_connection()
        # TODO: this should use the default wallet already used in the test framework
        # self._rpc.createwallet(DEFAULT_WALLET_NAME)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Stop the node."""
        self._rpc.stop_node()
        self._node = None
        PortSeed.n = self._old_port_seed
