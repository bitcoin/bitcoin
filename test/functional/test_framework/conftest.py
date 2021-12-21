"""Test configuration module."""
import inspect
from pathlib import Path

CUR_PATH = Path(inspect.getfile(inspect.currentframe())).parent  # type: ignore
ITCOIN_CORE_ROOT_DIR = Path(CUR_PATH, "..", "..", "..").resolve().absolute()
ITCOIN_CORE_TEST_DIR = ITCOIN_CORE_ROOT_DIR / "test"

# we assume itcoin-pbft project is in the same folder of the itcoin-core project
ITCOIN_PBFT_ROOT_DIR = Path(ITCOIN_CORE_ROOT_DIR, "..", "itcoin-pbft").resolve().absolute()


DEFAULT_CONFIG_PATH = ITCOIN_CORE_TEST_DIR / "config.ini"
DEFAULT_RPC_TIMEOUT = 60
DEFAULT_RPC_TIMEOUT_FACTOR = 1.0
DEFAULT_WALLET_NAME = "default"
LOCALHOST = "127.0.0.1"

REGTEST = "regtest"
SIGNET = "signet"
