#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2014-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpful routines for regression testing."""

from base64 import b64encode
from binascii import hexlify, unhexlify
from decimal import Decimal, ROUND_DOWN
import hashlib
import inspect
import json
import logging
import os
import random
import shutil
import re
from subprocess import CalledProcessError
import time

from . import coverage
from .authproxy import AuthServiceProxy, JSONRPCException
from io import BytesIO

logger = logging.getLogger("TestFramework.utils")

# Util options
##############

class Options:
    timeout_scale = 1

def set_timeout_scale(_timeout_scale):
    Options.timeout_scale = _timeout_scale

# Assert functions
##################

def assert_fee_amount(fee, tx_size, fee_per_kB):
    """Assert the fee was in range"""
    target_fee = round(tx_size * fee_per_kB / 1000, 8)
    if fee < target_fee:
        raise AssertionError("Fee of %s DASH too low! (Should be %s DASH)" % (str(fee), str(target_fee)))
    # allow the wallet's estimation to be at most 2 bytes off
    if fee > (tx_size + 2) * fee_per_kB / 1000:
        raise AssertionError("Fee of %s DASH too high! (Should be %s DASH)" % (str(fee), str(target_fee)))

def assert_equal(thing1, thing2, *args):
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
            raise AssertionError("Expected substring not found:" + e.error['message'])
    except Exception as e:
        raise AssertionError("Unexpected exception raised: " + type(e).__name__)
    else:
        raise AssertionError("No exception raised")

def assert_raises_process_error(returncode, output, fun, *args, **kwds):
    """Execute a process and asserts the process return code and output.

    Calls function `fun` with arguments `args` and `kwds`. Catches a CalledProcessError
    and verifies that the return code and output are as expected. Throws AssertionError if
    no CalledProcessError was raised or if the return code and output are not as expected.

    Args:
        returncode (int): the process return code.
        output (string): [a substring of] the process output.
        fun (function): the function to call. This should execute a process.
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

def assert_raises_rpc_error(code, message, fun, *args, **kwds):
    """Run an RPC and verify that a specific JSONRPC exception code and message is raised.

    Calls function `fun` with arguments `args` and `kwds`. Catches a JSONRPCException
    and verifies that the error code and message are as expected. Throws AssertionError if
    no JSONRPCException was raised or if the error code/message are not as expected.

    Args:
        code (int), optional: the error code returned by the RPC call (defined
            in src/rpc/protocol.h). Set to None if checking the error code is not required.
        message (string), optional: [a substring of] the error string returned by the
            RPC call. Set to None if checking the error string is not required.
        fun (function): the function to call. This should be the name of an RPC.
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
            raise AssertionError("Expected substring not found:" + e.error['message'])
        return True
    except Exception as e:
        raise AssertionError("Unexpected exception raised: " + type(e).__name__)
    else:
        return False

def assert_is_hex_string(string):
    try:
        int(string, 16)
    except Exception as e:
        raise AssertionError(
            "Couldn't interpret %r as hexadecimal; raised: %s" % (string, e))

def assert_is_hash_string(string, length=64):
    if not isinstance(string, str):
        raise AssertionError("Expected a string, got type %r" % type(string))
    elif length and len(string) != length:
        raise AssertionError(
            "String of length %d expected; got %d" % (length, len(string)))
    elif not re.match('[abcdef0-9]+$', string):
        raise AssertionError(
            "String %r contains invalid characters for a hash." % string)

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

def bytes_to_hex_str(byte_str):
    return hexlify(byte_str).decode('ascii')

def hash256(byte_str):
    sha256 = hashlib.sha256()
    sha256.update(byte_str)
    sha256d = hashlib.sha256()
    sha256d.update(sha256.digest())
    return sha256d.digest()[::-1]

def hex_str_to_bytes(hex_str):
    return unhexlify(hex_str.encode('ascii'))

def str_to_b64str(string):
    return b64encode(string.encode('utf-8')).decode('ascii')

def satoshi_round(amount):
    return Decimal(amount).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)

def wait_until(predicate, *, attempts=float('inf'), timeout=float('inf'), sleep=0.05, lock=None, do_assert=True, allow_exception=False):
    if attempts == float('inf') and timeout == float('inf'):
        timeout = 60
    attempt = 0
    timeout *= Options.timeout_scale
    time_end = time.time() + timeout

    while attempt < attempts and time.time() < time_end:
        try:
            if lock:
                with lock:
                    if predicate():
                        return True
            else:
                if predicate():
                    return True
        except:
            if not allow_exception:
                raise
        attempt += 1
        time.sleep(sleep)

    if do_assert:
        # Print the cause of the timeout
        predicate_source = "''''\n" + inspect.getsource(predicate) + "'''"
        logger.error("wait_until() failed. Predicate: {}".format(predicate_source))
        if attempt >= attempts:
            raise AssertionError("Predicate {} not true after {} attempts".format(predicate_source, attempts))
        elif time.time() >= time_end:
            raise AssertionError("Predicate {} not true after {} seconds".format(predicate_source, timeout))
        raise RuntimeError('Unreachable')
    else:
        return False

# RPC/P2P connection constants and functions
############################################

# The maximum number of nodes a single test can spawn
MAX_NODES = 15
# Don't assign rpc or p2p ports lower than this
PORT_MIN = 11000
# The number of ports to "reserve" for p2p and rpc, each
PORT_RANGE = 5000

class PortSeed:
    # Must be initialized with a unique integer for each process
    n = None

def get_rpc_proxy(url, node_number, *, timeout=None, coveragedir=None):
    """
    Args:
        url (str): URL of the RPC server to call
        node_number (int): the node number (or id) that this calls to

    Kwargs:
        timeout (int): HTTP timeout in seconds
        coveragedir (str): Directory

    Returns:
        AuthServiceProxy. convenience object for making RPC calls.

    """
    proxy_kwargs = {}
    if timeout is not None:
        proxy_kwargs['timeout'] = timeout

    proxy = AuthServiceProxy(url, **proxy_kwargs)
    proxy.url = url  # store URL on proxy for info

    coverage_logfile = coverage.get_filename(
        coveragedir, node_number) if coveragedir else None

    return coverage.AuthServiceProxyWrapper(proxy, coverage_logfile)

def p2p_port(n):
    assert(n <= MAX_NODES)
    return PORT_MIN + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)

def rpc_port(n):
    return PORT_MIN + PORT_RANGE + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)

def rpc_url(datadir, i, chain, rpchost=None):
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

def initialize_datadir(dirname, n, chain):
    datadir = get_datadir_path(dirname, n)
    if not os.path.isdir(datadir):
        os.makedirs(datadir)
    # Translate chain name to config name
    if chain == 'testnet3':
        chain_name_conf_arg = 'testnet'
        chain_name_conf_section = 'test'
        chain_name_conf_arg_value = '1'
    elif chain == 'devnet':
        chain_name_conf_arg = 'devnet'
        chain_name_conf_section = 'devnet'
        chain_name_conf_arg_value = 'devnet1'
    else:
        chain_name_conf_arg = chain
        chain_name_conf_section = chain
        chain_name_conf_arg_value = '1'
    with open(os.path.join(datadir, "dash.conf"), 'w', encoding='utf8') as f:
        f.write("{}={}\n".format(chain_name_conf_arg, chain_name_conf_arg_value))
        f.write("[{}]\n".format(chain_name_conf_section))
        f.write("port=" + str(p2p_port(n)) + "\n")
        f.write("rpcport=" + str(rpc_port(n)) + "\n")
        f.write("server=1\n")
        f.write("keypool=1\n")
        f.write("discover=0\n")
        f.write("listenonion=0\n")
        f.write("printtoconsole=0\n")
        os.makedirs(os.path.join(datadir, 'stderr'), exist_ok=True)
        os.makedirs(os.path.join(datadir, 'stdout'), exist_ok=True)
    return datadir

def get_datadir_path(dirname, n):
    return os.path.join(dirname, "node" + str(n))

def append_config(datadir, options):
    with open(os.path.join(datadir, "dash.conf"), 'a', encoding='utf8') as f:
        for option in options:
            f.write(option + "\n")

def get_auth_cookie(datadir, chain):
    user = None
    password = None
    if os.path.isfile(os.path.join(datadir, "dash.conf")):
        with open(os.path.join(datadir, "dash.conf"), 'r', encoding='utf8') as f:
            for line in f:
                if line.startswith("rpcuser="):
                    assert user is None  # Ensure that there is only one rpcuser line
                    user = line.split("=")[1].strip("\n")
                if line.startswith("rpcpassword="):
                    assert password is None  # Ensure that there is only one rpcpassword line
                    password = line.split("=")[1].strip("\n")
    chain = get_chain_folder(datadir, chain)
    if os.path.isfile(os.path.join(datadir, chain, ".cookie")) and os.access(os.path.join(datadir, chain, ".cookie"), os.R_OK):
        with open(os.path.join(datadir, chain, ".cookie"), 'r', encoding="ascii") as f:
            userpass = f.read()
            split_userpass = userpass.split(':')
            user = split_userpass[0]
            password = split_userpass[1]
    if user is None or password is None:
        raise ValueError("No RPC credentials")
    return user, password

def copy_datadir(from_node, to_node, dirname, chain):
    from_datadir = os.path.join(dirname, "node"+str(from_node), chain)
    to_datadir = os.path.join(dirname, "node"+str(to_node), chain)

    dirs = ["blocks", "chainstate", "evodb", "llmq"]
    for d in dirs:
        try:
            src = os.path.join(from_datadir, d)
            dst = os.path.join(to_datadir, d)
            shutil.copytree(src, dst)
        except:
            pass

# If a cookie file exists in the given datadir, delete it.
def delete_cookie_file(datadir, chain):
    chain = get_chain_folder(datadir, chain)
    if os.path.isfile(os.path.join(datadir, chain, ".cookie")):
        logger.debug("Deleting leftover cookie file")
        os.remove(os.path.join(datadir, chain, ".cookie"))

"""
since devnets can be named we won't always know what the folders name is unless we would pass it through all functions,
which shouldn't be needed as if we are to test multiple different devnets we would just override setup_chain and make our own configs files.
"""
def get_chain_folder(datadir, chain):
    # if try fails the directory doesn't exist
    try:
        for i in range(len(os.listdir(datadir))):
            if chain in os.listdir(datadir)[i]:
                chain = os.listdir(datadir)[i]
                break
    except:
        pass
    return chain

def get_bip9_status(node, key):
    info = node.getblockchaininfo()
    return info['bip9_softforks'][key]

def set_node_times(nodes, t):
    for node in nodes:
        node.mocktime = t
        node.setmocktime(t)

def disconnect_nodes(from_connection, node_num):
    for peer_id in [peer['id'] for peer in from_connection.getpeerinfo() if "testnode%d" % node_num in peer['subver']]:
        try:
            from_connection.disconnectnode(nodeid=peer_id)
        except JSONRPCException as e:
            # If this node is disconnected between calculating the peer id
            # and issuing the disconnect, don't worry about it.
            # This avoids a race condition if we're mass-disconnecting peers.
            if e.error['code'] != -29: # RPC_CLIENT_NODE_NOT_CONNECTED
                raise

    # wait to disconnect
    wait_until(lambda: [peer['id'] for peer in from_connection.getpeerinfo() if "testnode%d" % node_num in peer['subver']] == [], timeout=5)

def connect_nodes(from_connection, node_num):
    ip_port = "127.0.0.1:" + str(p2p_port(node_num))
    from_connection.addnode(ip_port, "onetry")
    # poll until version handshake complete to avoid race conditions
    # with transaction relaying
    # See comments in net_processing:
    # * Must have a version message before anything else
    # * Must have a verack message before anything else
    wait_until(lambda: all(peer['version'] != 0 for peer in from_connection.getpeerinfo()))
    wait_until(lambda: all(peer['bytesrecv_per_msg'].pop('verack', 0) == 24 for peer in from_connection.getpeerinfo()))

def connect_nodes_bi(nodes, a, b):
    connect_nodes(nodes[a], b)
    connect_nodes(nodes[b], a)

def isolate_node(node, timeout=5):
    node.setnetworkactive(False)
    st = time.time()
    while time.time() < st + timeout:
        if node.getconnectioncount() == 0:
            return
        time.sleep(0.5)
    raise AssertionError("disconnect_node timed out")

def reconnect_isolated_node(node, node_num):
    node.setnetworkactive(True)
    connect_nodes(node, node_num)

def sync_blocks(rpc_connections, *, wait=1, timeout=60):
    """
    Wait until everybody has the same tip.

    sync_blocks needs to be called with an rpc_connections set that has least
    one node already synced to the latest, stable tip, otherwise there's a
    chance it might return before all nodes are stably synced.
    """
    timeout *= Options.timeout_scale

    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        best_hash = [x.getbestblockhash() for x in rpc_connections]
        if best_hash.count(best_hash[0]) == len(rpc_connections):
            return
        time.sleep(wait)
    raise AssertionError("Block sync timed out:{}".format("".join("\n  {!r}".format(b) for b in best_hash)))

def sync_mempools(rpc_connections, *, wait=1, timeout=60, flush_scheduler=True, wait_func=None):
    """
    Wait until everybody has the same transactions in their memory
    pools
    """
    timeout *= Options.timeout_scale
    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        pool = [set(r.getrawmempool()) for r in rpc_connections]
        if pool.count(pool[0]) == len(rpc_connections):
            if flush_scheduler:
                for r in rpc_connections:
                    r.syncwithvalidationinterfacequeue()
            return
        if wait_func is not None:
            wait_func()
        time.sleep(wait)
    raise AssertionError("Mempool sync timed out:{}".format("".join("\n  {!r}".format(m) for m in pool)))

def force_finish_mnsync(node):
    """
    Masternodes won't accept incoming connections while IsSynced is false.
    Force them to switch to this state to speed things up.
    """
    while True:
        if node.mnsync("status")['IsSynced']:
            break
        node.mnsync("next")

# Transaction/Block functions
#############################

def find_output(node, txid, amount):
    """
    Return index to output of txid with value amount
    Raises exception if there is none.
    """
    txdata = node.getrawtransaction(txid, 1)
    for i in range(len(txdata["vout"])):
        if txdata["vout"][i]["value"] == amount:
            return i
    raise RuntimeError("find_output txid %s : %s not found" % (txid, str(amount)))

def gather_inputs(from_node, amount_needed, confirmations_required=1):
    """
    Return a random set of unspent txouts that are enough to pay amount_needed
    """
    assert(confirmations_required >= 0)
    utxo = from_node.listunspent(confirmations_required)
    random.shuffle(utxo)
    inputs = []
    total_in = Decimal("0.00000000")
    while total_in < amount_needed and len(utxo) > 0:
        t = utxo.pop()
        total_in += t["amount"]
        inputs.append({"txid": t["txid"], "vout": t["vout"], "address": t["address"]})
    if total_in < amount_needed:
        raise RuntimeError("Insufficient funds: need %d, have %d" % (amount_needed, total_in))
    return (total_in, inputs)

def make_change(from_node, amount_in, amount_out, fee):
    """
    Create change output(s), return them
    """
    outputs = {}
    amount = amount_out + fee
    change = amount_in - amount
    if change > amount * 2:
        # Create an extra change output to break up big inputs
        change_address = from_node.getnewaddress()
        # Split change in two, being careful of rounding:
        outputs[change_address] = Decimal(change / 2).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)
        change = amount_in - amount - outputs[change_address]
    if change > 0:
        outputs[from_node.getnewaddress()] = change
    return outputs

def random_transaction(nodes, amount, min_fee, fee_increment, fee_variants):
    """
    Create a random transaction.
    Returns (txid, hex-encoded-transaction-data, fee)
    """
    from_node = random.choice(nodes)
    to_node = random.choice(nodes)
    fee = min_fee + fee_increment * random.randint(0, fee_variants)

    (total_in, inputs) = gather_inputs(from_node, amount + fee)
    outputs = make_change(from_node, total_in, amount, fee)
    outputs[to_node.getnewaddress()] = float(amount)

    rawtx = from_node.createrawtransaction(inputs, outputs)
    signresult = from_node.signrawtransactionwithwallet(rawtx)
    txid = from_node.sendrawtransaction(signresult["hex"], True)

    return (txid, signresult["hex"], fee)

# Helper to create at least "count" utxos
# Pass in a fee that is sufficient for relay and mining new transactions.
def create_confirmed_utxos(fee, node, count):
    to_generate = int(0.5 * count) + 101
    while to_generate > 0:
        node.generate(min(25, to_generate))
        to_generate -= 25
    utxos = node.listunspent()
    iterations = count - len(utxos)
    addr1 = node.getnewaddress()
    addr2 = node.getnewaddress()
    if iterations <= 0:
        return utxos
    for i in range(iterations):
        t = utxos.pop()
        inputs = []
        inputs.append({"txid": t["txid"], "vout": t["vout"]})
        outputs = {}
        send_value = t['amount'] - fee
        outputs[addr1] = satoshi_round(send_value / 2)
        outputs[addr2] = satoshi_round(send_value / 2)
        raw_tx = node.createrawtransaction(inputs, outputs)
        signed_tx = node.signrawtransactionwithwallet(raw_tx)["hex"]
        node.sendrawtransaction(signed_tx)

    while (node.getmempoolinfo()['size'] > 0):
        node.generate(1)

    utxos = node.listunspent()
    assert(len(utxos) >= count)
    return utxos

# Create large OP_RETURN txouts that can be appended to a transaction
# to make it large (helper for constructing large transactions).
def gen_return_txouts():
    # Some pre-processing to create a bunch of OP_RETURN txouts to insert into transactions we create
    # So we have big transactions (and therefore can't fit very many into each block)
    # create one script_pubkey
    script_pubkey = "6a4d0200"  # OP_RETURN OP_PUSH2 512 bytes
    for i in range(512):
        script_pubkey = script_pubkey + "01"
    # concatenate 128 txouts of above script_pubkey which we'll insert before the txout for change
    txouts = []
    from .messages import CTxOut
    txout = CTxOut()
    txout.nValue = 0
    txout.scriptPubKey = hex_str_to_bytes(script_pubkey)
    for k in range(128):
        txouts.append(txout)
    return txouts

# Create a spend of each passed-in utxo, splicing in "txouts" to each raw
# transaction to make it large.  See gen_return_txouts() above.
def create_lots_of_big_transactions(node, txouts, utxos, num, fee):
    addr = node.getnewaddress()
    txids = []
    from .messages import CTransaction
    for _ in range(num):
        t = utxos.pop()
        inputs = [{"txid": t["txid"], "vout": t["vout"]}]
        outputs = {}
        change = t['amount'] - fee
        outputs[addr] = satoshi_round(change)
        rawtx = node.createrawtransaction(inputs, outputs)
        tx = CTransaction()
        tx.deserialize(BytesIO(hex_str_to_bytes(rawtx)))
        for txout in txouts:
            tx.vout.append(txout)
        newtx = tx.serialize().hex()
        signresult = node.signrawtransactionwithwallet(newtx, None, "NONE")
        txid = node.sendrawtransaction(signresult["hex"], True)
        txids.append(txid)
    return txids

def mine_large_block(node, utxos=None):
    # generate a 66k transaction,
    # and 14 of them is close to the 1MB block limit
    num = 14
    txouts = gen_return_txouts()
    utxos = utxos if utxos is not None else []
    if len(utxos) < num:
        utxos.clear()
        utxos.extend(node.listunspent())
    fee = 100 * node.getnetworkinfo()["relayfee"]
    create_lots_of_big_transactions(node, txouts, utxos, num, fee=fee)
    node.generate(1)

def find_vout_for_address(node, txid, addr):
    """
    Locate the vout index of the given transaction sending to the
    given address. Raises runtime error exception if not found.
    """
    tx = node.getrawtransaction(txid, True)
    for i in range(len(tx["vout"])):
        if any([addr == a for a in tx["vout"][i]["scriptPubKey"]["addresses"]]):
            return i
    raise RuntimeError("Vout not found for address: txid=%s, addr=%s" % (txid, addr))
