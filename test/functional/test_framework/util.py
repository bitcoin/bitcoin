#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpful routines for regression testing."""

from base64 import b64encode
from decimal import Decimal, ROUND_DOWN
from subprocess import CalledProcessError
import hashlib
import inspect
import json
import logging
import os
import pathlib
import platform
import random
import re
import time

from . import coverage
from .authproxy import AuthServiceProxy, JSONRPCException
from collections.abc import Callable
from typing import Optional

logger = logging.getLogger("TestFramework.utils")

# Assert functions
##################


def assert_approx(v, vexp, vspan=0.00001):
    """Assert that `v` is within `vspan` of `vexp`"""
    if isinstance(v, Decimal) or isinstance(vexp, Decimal):
        v=Decimal(v)
        vexp=Decimal(vexp)
        vspan=Decimal(vspan)
    if v < vexp - vspan:
        raise AssertionError("%s < [%s..%s]" % (str(v), str(vexp - vspan), str(vexp + vspan)))
    if v > vexp + vspan:
        raise AssertionError("%s > [%s..%s]" % (str(v), str(vexp - vspan), str(vexp + vspan)))


def assert_fee_amount(fee, tx_size, feerate_BTC_kvB):
    """Assert the fee is in range."""
    assert isinstance(tx_size, int)
    target_fee = get_fee(tx_size, feerate_BTC_kvB)
    if fee < target_fee:
        raise AssertionError("Fee of %s BTC too low! (Should be %s BTC)" % (str(fee), str(target_fee)))
    # allow the wallet's estimation to be at most 2 bytes off
    high_fee = get_fee(tx_size + 2, feerate_BTC_kvB)
    if fee > high_fee:
        raise AssertionError("Fee of %s BTC too high! (Should be %s BTC)" % (str(fee), str(target_fee)))


def summarise_dict_differences(thing1, thing2):
    if not isinstance(thing1, dict) or not isinstance(thing2, dict):
        return thing1, thing2
    d1, d2 = {}, {}
    for k in sorted(thing1.keys()):
        if k not in thing2:
            d1[k] = thing1[k]
        elif thing1[k] != thing2[k]:
            d1[k], d2[k] = summarise_dict_differences(thing1[k], thing2[k])
    for k in sorted(thing2.keys()):
        if k not in thing1:
            d2[k] = thing2[k]
    return d1, d2

def assert_equal(thing1, thing2, *args):
    if thing1 != thing2 and not args and isinstance(thing1, dict) and isinstance(thing2, dict):
        d1,d2 = summarise_dict_differences(thing1, thing2)
        raise AssertionError("not(%s == %s)\n  in particular not(%s == %s)" % (thing1, thing2, d1, d2))
    if thing1 != thing2 or any(thing1 != arg for arg in args):
        raise AssertionError("not(%s)" % " == ".join(str(arg) for arg in (thing1, thing2) + args))


def assert_greater_than(thing1, thing2):
    if thing1 <= thing2:
        raise AssertionError("%s <= %s" % (str(thing1), str(thing2)))


def assert_greater_than_or_equal(thing1, thing2):
    if thing1 < thing2:
        raise AssertionError("%s < %s" % (str(thing1), str(thing2)))


def assert_raises(exc, fun, *args, **kwds):
    assert_raises_message(exc, None, fun, *args, **kwds)


def assert_raises_message(exc, message, fun, *args, **kwds):
    try:
        fun(*args, **kwds)
    except JSONRPCException:
        raise AssertionError("Use assert_raises_rpc_error() to test RPC failures")
    except exc as e:
        if message is not None and message not in e.error['message']:
            raise AssertionError(
                "Expected substring not found in error message:\nsubstring: '{}'\nerror message: '{}'.".format(
                    message, e.error['message']))
    except Exception as e:
        raise AssertionError("Unexpected exception raised: " + type(e).__name__)
    else:
        raise AssertionError("No exception raised")


def assert_raises_process_error(returncode: int, output: str, fun: Callable, *args, **kwds):
    """Execute a process and asserts the process return code and output.

    Calls function `fun` with arguments `args` and `kwds`. Catches a CalledProcessError
    and verifies that the return code and output are as expected. Throws AssertionError if
    no CalledProcessError was raised or if the return code and output are not as expected.

    Args:
        returncode: the process return code.
        output: [a substring of] the process output.
        fun: the function to call. This should execute a process.
        args*: positional arguments for the function.
        kwds**: named arguments for the function.
    """
    try:
        fun(*args, **kwds)
    except CalledProcessError as e:
        if returncode != e.returncode:
            raise AssertionError("Unexpected returncode %i" % e.returncode)
        if output not in e.output:
            raise AssertionError("Expected substring not found:" + e.output)
    else:
        raise AssertionError("No exception raised")


def assert_raises_rpc_error(code: Optional[int], message: Optional[str], fun: Callable, *args, **kwds):
    """Run an RPC and verify that a specific JSONRPC exception code and message is raised.

    Calls function `fun` with arguments `args` and `kwds`. Catches a JSONRPCException
    and verifies that the error code and message are as expected. Throws AssertionError if
    no JSONRPCException was raised or if the error code/message are not as expected.

    Args:
        code: the error code returned by the RPC call (defined in src/rpc/protocol.h).
            Set to None if checking the error code is not required.
        message: [a substring of] the error string returned by the RPC call.
            Set to None if checking the error string is not required.
        fun: the function to call. This should be the name of an RPC.
        args*: positional arguments for the function.
        kwds**: named arguments for the function.
    """
    assert try_rpc(code, message, fun, *args, **kwds), "No exception raised"


def try_rpc(code, message, fun, *args, **kwds):
    """Tries to run an rpc command.

    Test against error code and message if the rpc fails.
    Returns whether a JSONRPCException was raised."""
    try:
        fun(*args, **kwds)
    except JSONRPCException as e:
        # JSONRPCException was thrown as expected. Check the code and message values are correct.
        if (code is not None) and (code != e.error["code"]):
            raise AssertionError("Unexpected JSONRPC error code %i" % e.error["code"])
        if (message is not None) and (message not in e.error['message']):
            raise AssertionError(
                "Expected substring not found in error message:\nsubstring: '{}'\nerror message: '{}'.".format(
                    message, e.error['message']))
        return True
    except Exception as e:
        raise AssertionError("Unexpected exception raised: " + type(e).__name__)
    else:
        return False


def assert_is_hex_string(string):
    try:
        int(string, 16)
    except Exception as e:
        raise AssertionError("Couldn't interpret %r as hexadecimal; raised: %s" % (string, e))


def assert_is_hash_string(string, length=64):
    if not isinstance(string, str):
        raise AssertionError("Expected a string, got type %r" % type(string))
    elif length and len(string) != length:
        raise AssertionError("String of length %d expected; got %d" % (length, len(string)))
    elif not re.match('[abcdef0-9]+$', string):
        raise AssertionError("String %r contains invalid characters for a hash." % string)


def assert_array_result(object_array, to_match, expected, should_not_find=False):
    """
        Pass in array of JSON objects, a dictionary with key/value pairs
        to match against, and another dictionary with expected key/value
        pairs.
        If the should_not_find flag is true, to_match should not be found
        in object_array
        """
    if should_not_find:
        assert_equal(expected, {})
    num_matched = 0
    for item in object_array:
        all_match = True
        for key, value in to_match.items():
            if item[key] != value:
                all_match = False
        if not all_match:
            continue
        elif should_not_find:
            num_matched = num_matched + 1
        for key, value in expected.items():
            if item[key] != value:
                raise AssertionError("%s : expected %s=%s" % (str(item), str(key), str(value)))
            num_matched = num_matched + 1
    if num_matched == 0 and not should_not_find:
        raise AssertionError("No objects matched %s" % (str(to_match)))
    if num_matched > 0 and should_not_find:
        raise AssertionError("Objects were found %s" % (str(to_match)))


# Utility functions
###################


def check_json_precision():
    """Make sure json library being used does not lose precision converting BTC values"""
    n = Decimal("20000000.00000003")
    satoshis = int(json.loads(json.dumps(float(n))) * 1.0e8)
    if satoshis != 2000000000000003:
        raise RuntimeError("JSON encode/decode loses precision")


def count_bytes(hex_string):
    return len(bytearray.fromhex(hex_string))


def str_to_b64str(string):
    return b64encode(string.encode('utf-8')).decode('ascii')


def ceildiv(a, b):
    """
    Divide 2 ints and round up to next int rather than round down
    Implementation requires python integers, which have a // operator that does floor division.
    Other types like decimal.Decimal whose // operator truncates towards 0 will not work.
    """
    assert isinstance(a, int)
    assert isinstance(b, int)
    return -(-a // b)


def random_bitflip(data):
    data = list(data)
    data[random.randrange(len(data))] ^= (1 << (random.randrange(8)))
    return bytes(data)


def get_fee(tx_size, feerate_btc_kvb):
    """Calculate the fee in BTC given a feerate is BTC/kvB. Reflects CFeeRate::GetFee"""
    feerate_sat_kvb = int(feerate_btc_kvb * Decimal(1e8)) # Fee in sat/kvb as an int to avoid float precision errors
    target_fee_sat = ceildiv(feerate_sat_kvb * tx_size, 1000) # Round calculated fee up to nearest sat
    return target_fee_sat / Decimal(1e8) # Return result in  BTC


def satoshi_round(amount):
    return Decimal(amount).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)


def wait_until_helper_internal(predicate, *, attempts=float('inf'), timeout=float('inf'), lock=None, timeout_factor=1.0):
    """Sleep until the predicate resolves to be True.

    Warning: Note that this method is not recommended to be used in tests as it is
    not aware of the context of the test framework. Using the `wait_until()` members
    from `BitcoinTestFramework` or `P2PInterface` class ensures the timeout is
    properly scaled. Furthermore, `wait_until()` from `P2PInterface` class in
    `p2p.py` has a preset lock.
    """
    if attempts == float('inf') and timeout == float('inf'):
        timeout = 60
    timeout = timeout * timeout_factor
    attempt = 0
    time_end = time.time() + timeout

    while attempt < attempts and time.time() < time_end:
        if lock:
            with lock:
                if predicate():
                    return
        else:
            if predicate():
                return
        attempt += 1
        time.sleep(0.05)

    # Print the cause of the timeout
    predicate_source = "''''\n" + inspect.getsource(predicate) + "'''"
    logger.error("wait_until() failed. Predicate: {}".format(predicate_source))
    if attempt >= attempts:
        raise AssertionError("Predicate {} not true after {} attempts".format(predicate_source, attempts))
    elif time.time() >= time_end:
        raise AssertionError("Predicate {} not true after {} seconds".format(predicate_source, timeout))
    raise RuntimeError('Unreachable')


def sha256sum_file(filename):
    h = hashlib.sha256()
    with open(filename, 'rb') as f:
        d = f.read(4096)
        while len(d) > 0:
            h.update(d)
            d = f.read(4096)
    return h.digest()


# RPC/P2P connection constants and functions
############################################

# The maximum number of nodes a single test can spawn
MAX_NODES = 12
# Don't assign rpc or p2p ports lower than this
PORT_MIN = int(os.getenv('TEST_RUNNER_PORT_MIN', default=11000))
# The number of ports to "reserve" for p2p and rpc, each
PORT_RANGE = 5000


class PortSeed:
    # Must be initialized with a unique integer for each process
    n = None


def get_rpc_proxy(url: str, node_number: int, *, timeout: Optional[int]=None, coveragedir: Optional[str]=None) -> coverage.AuthServiceProxyWrapper:
    """
    Args:
        url: URL of the RPC server to call
        node_number: the node number (or id) that this calls to

    Kwargs:
        timeout: HTTP timeout in seconds
        coveragedir: Directory

    Returns:
        AuthServiceProxy. convenience object for making RPC calls.

    """
    proxy_kwargs = {}
    if timeout is not None:
        proxy_kwargs['timeout'] = int(timeout)

    proxy = AuthServiceProxy(url, **proxy_kwargs)

    coverage_logfile = coverage.get_filename(coveragedir, node_number) if coveragedir else None

    return coverage.AuthServiceProxyWrapper(proxy, url, coverage_logfile)


def p2p_port(n):
    assert n <= MAX_NODES
    return PORT_MIN + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)


def rpc_port(n):
    return PORT_MIN + PORT_RANGE + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)


def rpc_url(datadir, i, chain, rpchost):
    rpc_u, rpc_p = get_auth_cookie(datadir, chain)
    host = '127.0.0.1'
    port = rpc_port(i)
    if rpchost:
        parts = rpchost.split(':')
        if len(parts) == 2:
            host, port = parts
        else:
            host = rpchost
    return "http://%s:%s@%s:%d" % (rpc_u, rpc_p, host, int(port))


# Node functions
################


def initialize_datadir(dirname, n, chain, disable_autoconnect=True):
    datadir = get_datadir_path(dirname, n)
    if not os.path.isdir(datadir):
        os.makedirs(datadir)
    write_config(os.path.join(datadir, "bitcoin.conf"), n=n, chain=chain, disable_autoconnect=disable_autoconnect)
    os.makedirs(os.path.join(datadir, 'stderr'), exist_ok=True)
    os.makedirs(os.path.join(datadir, 'stdout'), exist_ok=True)
    return datadir


def write_config(config_path, *, n, chain, extra_config="", disable_autoconnect=True):
    # Translate chain subdirectory name to config name
    if chain == 'testnet3':
        chain_name_conf_arg = 'testnet'
        chain_name_conf_section = 'test'
    else:
        chain_name_conf_arg = chain
        chain_name_conf_section = chain
    with open(config_path, 'w', encoding='utf8') as f:
        if chain_name_conf_arg:
            f.write("{}=1\n".format(chain_name_conf_arg))
        if chain_name_conf_section:
            f.write("[{}]\n".format(chain_name_conf_section))
        f.write("port=" + str(p2p_port(n)) + "\n")
        f.write("rpcport=" + str(rpc_port(n)) + "\n")
        # Disable server-side timeouts to avoid intermittent issues
        f.write("rpcservertimeout=99000\n")
        f.write("rpcdoccheck=1\n")
        f.write("fallbackfee=0.0002\n")
        f.write("server=1\n")
        f.write("keypool=1\n")
        f.write("discover=0\n")
        f.write("dnsseed=0\n")
        f.write("fixedseeds=0\n")
        f.write("listenonion=0\n")
        # Increase peertimeout to avoid disconnects while using mocktime.
        # peertimeout is measured in mock time, so setting it large enough to
        # cover any duration in mock time is sufficient. It can be overridden
        # in tests.
        f.write("peertimeout=999999999\n")
        f.write("printtoconsole=0\n")
        f.write("upnp=0\n")
        f.write("natpmp=0\n")
        f.write("shrinkdebugfile=0\n")
        f.write("deprecatedrpc=create_bdb\n")  # Required to run the tests
        # To improve SQLite wallet performance so that the tests don't timeout, use -unsafesqlitesync
        f.write("unsafesqlitesync=1\n")
        if disable_autoconnect:
            f.write("connect=0\n")
        f.write(extra_config)


def get_datadir_path(dirname, n):
    return pathlib.Path(dirname) / f"node{n}"


def get_temp_default_datadir(temp_dir: pathlib.Path) -> tuple[dict, pathlib.Path]:
    """Return os-specific environment variables that can be set to make the
    GetDefaultDataDir() function return a datadir path under the provided
    temp_dir, as well as the complete path it would return."""
    if platform.system() == "Windows":
        env = dict(APPDATA=str(temp_dir))
        datadir = temp_dir / "Bitcoin"
    else:
        env = dict(HOME=str(temp_dir))
        if platform.system() == "Darwin":
            datadir = temp_dir / "Library/Application Support/Bitcoin"
        else:
            datadir = temp_dir / ".bitcoin"
    return env, datadir


def append_config(datadir, options):
    with open(os.path.join(datadir, "bitcoin.conf"), 'a', encoding='utf8') as f:
        for option in options:
            f.write(option + "\n")


def get_auth_cookie(datadir, chain):
    user = None
    password = None
    if os.path.isfile(os.path.join(datadir, "bitcoin.conf")):
        with open(os.path.join(datadir, "bitcoin.conf"), 'r', encoding='utf8') as f:
            for line in f:
                if line.startswith("rpcuser="):
                    assert user is None  # Ensure that there is only one rpcuser line
                    user = line.split("=")[1].strip("\n")
                if line.startswith("rpcpassword="):
                    assert password is None  # Ensure that there is only one rpcpassword line
                    password = line.split("=")[1].strip("\n")
    try:
        with open(os.path.join(datadir, chain, ".cookie"), 'r', encoding="ascii") as f:
            userpass = f.read()
            split_userpass = userpass.split(':')
            user = split_userpass[0]
            password = split_userpass[1]
    except OSError:
        pass
    if user is None or password is None:
        raise ValueError("No RPC credentials")
    return user, password


# If a cookie file exists in the given datadir, delete it.
def delete_cookie_file(datadir, chain):
    if os.path.isfile(os.path.join(datadir, chain, ".cookie")):
        logger.debug("Deleting leftover cookie file")
        os.remove(os.path.join(datadir, chain, ".cookie"))


def softfork_active(node, key):
    """Return whether a softfork is active."""
    return node.getdeploymentinfo()['deployments'][key]['active']


def set_node_times(nodes, t):
    for node in nodes:
        node.setmocktime(t)


def check_node_connections(*, node, num_in, num_out):
    info = node.getnetworkinfo()
    assert_equal(info["connections_in"], num_in)
    assert_equal(info["connections_out"], num_out)


# Transaction/Block functions
#############################


# Create large OP_RETURN txouts that can be appended to a transaction
# to make it large (helper for constructing large transactions). The
# total serialized size of the txouts is about 66k vbytes.
def gen_return_txouts():
    from .messages import CTxOut
    from .script import CScript, OP_RETURN
    txouts = [CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN, b'\x01'*67437]))]
    assert_equal(sum([len(txout.serialize()) for txout in txouts]), 67456)
    return txouts


# Create a spend of each passed-in utxo, splicing in "txouts" to each raw
# transaction to make it large.  See gen_return_txouts() above.
def create_lots_of_big_transactions(mini_wallet, node, fee, tx_batch_size, txouts, utxos=None):
    txids = []
    use_internal_utxos = utxos is None
    for _ in range(tx_batch_size):
        tx = mini_wallet.create_self_transfer(
            utxo_to_spend=None if use_internal_utxos else utxos.pop(),
            fee=fee,
        )["tx"]
        tx.vout.extend(txouts)
        res = node.testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res['fees']['base'], fee)
        txids.append(node.sendrawtransaction(tx.serialize().hex()))
    return txids


def mine_large_block(test_framework, mini_wallet, node):
    # generate a 66k transaction,
    # and 14 of them is close to the 1MB block limit
    txouts = gen_return_txouts()
    fee = 100 * node.getnetworkinfo()["relayfee"]
    create_lots_of_big_transactions(mini_wallet, node, fee, 14, txouts)
    test_framework.generate(node, 1)


def find_vout_for_address(node, txid, addr):
    """
    Locate the vout index of the given transaction sending to the
    given address. Raises runtime error exception if not found.
    """
    tx = node.getrawtransaction(txid, True)
    for i in range(len(tx["vout"])):
        if addr == tx["vout"][i]["scriptPubKey"]["address"]:
            return i
    raise RuntimeError("Vout not found for address: txid=%s, addr=%s" % (txid, addr))
