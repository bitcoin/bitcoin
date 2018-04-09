#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
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
import re
from subprocess import CalledProcessError
import time

from . import coverage
from .authproxy import AuthServiceProxy, JSONRPCException

logger = logging.getLogger("TestFramework.utils")

# Assert functions
##################

def assert_fee_amount(fee, tx_size, fee_per_kB):
    """Assert the fee was in range"""
    target_fee = round(tx_size * fee_per_kB / 1000, 8)
    if fee < target_fee:
        raise AssertionError("Fee of %s BTC too low! (Should be %s BTC)" % (str(fee), str(target_fee)))
    # allow the wallet's estimation to be at most 2 bytes off
    if fee > (tx_size + 2) * fee_per_kB / 1000:
        raise AssertionError("Fee of %s BTC too high! (Should be %s BTC)" % (str(fee), str(target_fee)))

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

def wait_until(predicate, *, attempts=float('inf'), timeout=float('inf'), lock=None):
    if attempts == float('inf') and timeout == float('inf'):
        timeout = 60
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
    predicate_source = inspect.getsourcelines(predicate)
    logger.error("wait_until() failed. Predicate: {}".format(predicate_source))
    if attempt >= attempts:
        raise AssertionError("Predicate {} not true after {} attempts".format(predicate_source, attempts))
    elif time.time() >= time_end:
        raise AssertionError("Predicate {} not true after {} seconds".format(predicate_source, timeout))
    raise RuntimeError('Unreachable')

# RPC/P2P connection constants and functions
############################################

# The maximum number of nodes a single test can spawn
MAX_NODES = 8
# Don't assign rpc or p2p ports lower than this
PORT_MIN = 11000
# The number of ports to "reserve" for p2p and rpc, each
PORT_RANGE = 5000

class PortSeed:
    # Must be initialized with a unique integer for each process
    n = None

def get_rpc_proxy(url, node_number, timeout=None, coveragedir=None):
    """
    Args:
        url (str): URL of the RPC server to call
        node_number (int): the node number (or id) that this calls to

    Kwargs:
        timeout (int): HTTP timeout in seconds

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

def rpc_url(datadir, i, rpchost=None):
    rpc_u, rpc_p = get_auth_cookie(datadir)
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

def initialize_datadir(dirname, n):
    datadir = get_datadir_path(dirname, n)
    if not os.path.isdir(datadir):
        os.makedirs(datadir)
    with open(os.path.join(datadir, "bitcoin.conf"), 'w', encoding='utf8') as f:
        f.write("regtest=1\n")
        f.write("port=" + str(p2p_port(n)) + "\n")
        f.write("rpcport=" + str(rpc_port(n)) + "\n")
        f.write("server=1\n")
        f.write("keypool=1\n")
        f.write("discover=0\n")
        f.write("listenonion=0\n")
    return datadir

def get_datadir_path(dirname, n):
    return os.path.join(dirname, "node" + str(n))

def append_config(datadir, options):
    with open(os.path.join(datadir, "bitcoin.conf"), 'a', encoding='utf8') as f:
        for option in options:
            f.write(option + "\n")

def get_auth_cookie(datadir):
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
    if os.path.isfile(os.path.join(datadir, "regtest", ".cookie")):
        with open(os.path.join(datadir, "regtest", ".cookie"), 'r') as f:
            userpass = f.read()
            split_userpass = userpass.split(':')
            user = split_userpass[0]
            password = split_userpass[1]
    if user is None or password is None:
        raise ValueError("No RPC credentials")
    return user, password

def get_bip9_status(node, key):
    info = node.getblockchaininfo()
    return info['bip9_softforks'][key]

def set_node_times(nodes, t):
    for node in nodes:
        node.setmocktime(t)

def disconnect_nodes(from_connection, node_num):
    for peer_id in [peer['id'] for peer in from_connection.getpeerinfo() if "testnode%d" % node_num in peer['subver']]:
        from_connection.disconnectnode(nodeid=peer_id)

    # wait to disconnect
    wait_until(lambda: [peer['id'] for peer in from_connection.getpeerinfo() if "testnode%d" % node_num in peer['subver']] == [], timeout=5)

def connect_nodes(from_connection, node_num):
    ip_port = "127.0.0.1:" + str(p2p_port(node_num))
    from_connection.addnode(ip_port, "onetry")
    # poll until version handshake complete to avoid race conditions
    # with transaction relaying
    wait_until(lambda:  all(peer['version'] != 0 for peer in from_connection.getpeerinfo()))

def connect_nodes_bi(nodes, a, b):
    connect_nodes(nodes[a], b)
    connect_nodes(nodes[b], a)

def sync_blocks(rpc_connections, *, wait=1, timeout=60):
    """
    Wait until everybody has the same tip.

    sync_blocks needs to be called with an rpc_connections set that has least
    one node already synced to the latest, stable tip, otherwise there's a
    chance it might return before all nodes are stably synced.
    """
    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        best_hash = [x.getbestblockhash() for x in rpc_connections]
        if best_hash.count(best_hash[0]) == len(rpc_connections):
            return
        time.sleep(wait)
    raise AssertionError("Block sync timed out:{}".format("".join("\n  {!r}".format(b) for b in best_hash)))

def sync_mempools(rpc_connections, *, wait=1, timeout=60, flush_scheduler=True):
    """
    Wait until everybody has the same transactions in their memory
    pools
    """
    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        pool = [set(r.getrawmempool()) for r in rpc_connections]
        if pool.count(pool[0]) == len(rpc_connections):
            if flush_scheduler:
                for r in rpc_connections:
                    r.syncwithvalidationinterfacequeue()
            return
        time.sleep(wait)
    raise AssertionError("Mempool sync timed out:{}".format("".join("\n  {!r}".format(m) for m in pool)))

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


# Flip the endian-ness of a hex str
#
# '0033aaff' -> 'ffaa3300'
def reverse_endian(hex_str):
    return ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])


# Get a varint hex string from an integer
# According to protocol:
#
# Value            Storage length    Format
# < 0xFD            1                uint8_t
# <= 0xFFFF         3                0xFD followed by the length as uint16_t
# <= 0xFFFF FFFF    5                0xFE followed by the length as uint32_t
# -                 9                0xFF followed by the length as uint64_t
def get_varint_hex(num):
    if num < int("fd", 16):
        return "{:02x}".format(num)
    elif num <= int("ffff", 16):
        return "fd" + reverse_endian("{:04x}".format(num))
    elif num <= int("ffffffff", 16):
        return "fe" + reverse_endian("{:08x}".format(num))
    else:
        return "ff" + reverse_endian("{:16x}".format(num))


# Get bytes required for a PUSHDATA of size num
def pushdata_size_bytes(num):
    if num < int("ff", 16):
        return 1
    elif num <= int("ffff", 16):
        return 2
    elif num <= int("ffffffff", 16):
        return 4


# Given numoutputs pushing data_push_sz to the tx stack
# calculate the expected size of the outputs splice
def calculate_txouts_size(numoutputs, data_push_sz):
    # 2 bytes for [OP_RETURN, OP_PUSHDATAX] + DATA_SIZE + <DATA BYTES>
    script_pubkey_bytes = 2 + pushdata_size_bytes(data_push_sz) + data_push_sz
    num_outputs_varint_sz = int(len(get_varint_hex(numoutputs + 1)) / 2)
    script_pubkey_varint_sz = int(len(get_varint_hex(script_pubkey_bytes)) / 2)
    return (num_outputs_varint_sz + (numoutputs * (8 + script_pubkey_varint_sz + script_pubkey_bytes)))


# Create OP_RETURN txouts that can be appended to a transaction
# to make it large (helper for constructing large transactions).
def gen_return_txouts(numoutputs=128, data_push_sz=512):

    # OP_RETURN OP_PUSHDATA1 DATA_SIZE <DATA_BYTES>
    if data_push_sz <= int('ff', 16):
        script_pubkey = "6a4c{:02x}".format(data_push_sz) + ("01" * data_push_sz)
    # OP_RETURN OP_PUSHDATA2 DATA_SIZE <DATA_BYTES>
    elif data_push_sz <= int('ffff', 16):
        script_pubkey = "6a4d{:04x}".format(data_push_sz) + ("01" * data_push_sz)
    # OP_RETURN OP_PUSHDATA4 DATA_SIZE <DATA_BYTES>
    else:
        script_pubkey = "6a4e{:08x}".format(data_push_sz) + ("01" * data_push_sz)

    # number is + 1 because this will be spliced into single output tx
    num_outputs_varint = get_varint_hex(numoutputs + 1)

    # 3-6 bytes for OP_RETURN OP_PUSHDATA DATA_SIZE
    sz_script_pubkey_varint = get_varint_hex(2 + pushdata_size_bytes(data_push_sz) + data_push_sz)

    # Output format:
    #   8 bytes for Amount = 0x0000000000000000
    #   VarInt for scriptPubKey size
    #   scriptPubKey
    tx_output = "0000000000000000" + sz_script_pubkey_varint + script_pubkey

    # Combine number of outputs as varint with the repeated outputs
    txouts = num_outputs_varint + (tx_output * numoutputs)

    assert_equal(len(txouts)/2, calculate_txouts_size(numoutputs, data_push_sz))

    return txouts


def create_tx(node, coinbase, to_address, amount):
    inputs = [{"txid": coinbase, "vout": 0}]
    outputs = {to_address: amount}
    rawtx = node.createrawtransaction(inputs, outputs)
    signresult = node.signrawtransactionwithwallet(rawtx)
    assert_equal(signresult["complete"], True)
    return signresult["hex"]


# For very large transactions, need to find utxo that has enough to pay large fee
def get_utxo_with_amount(node, amount):
    for utxo in node.listunspent():
        if utxo['amount'] >= amount:
            return utxo

    fee = node.getnetworkinfo()['relayfee'] * 1000
    for utxo in create_confirmed_utxos(fee, node, 1):
        if utxo['amount'] >= amount:
            return utxo

# Create a transaction that is target_vsize. May not be exact in cases where
# (target_vsize - base_vsize) < 13 bytes (min output size)
# or when the difference is close to a VarInt boundary.
def create_tx_with_size(node, target_vsize, utxo=None, addr=None, fee=None):

    if not fee:
        # 10 sat / byte
        fee = target_vsize / 10000000
    if not utxo:
        utxo = get_utxo_with_amount(node, fee + 1/10000000)
    if not addr:
        addr = node.getnewaddress()

    fee = Decimal(fee)
    inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
    outputs = {addr: satoshi_round(utxo['amount'] - fee)}
    tx = (node.decoderawtransaction(
          node.signrawtransactionwithwallet(
          node.createrawtransaction(inputs, outputs), None, "NONE")['hex']))

    base_vsize = tx['vsize']

    assert_greater_than(target_vsize, base_vsize)

    # Output is:
    # 8 bytes for amount
    # 1/3/5 byte VarInt for size of scriptPubKey
    # 2 bytes for OP codes
    # 1/2/4 bytes for <DATA_SIZE>
    # DATA_BYTES (can range from 1 byte to just under maxblocksize)

    # For 8 byte amount and 2 byes of OP codes
    base_output_size = 10
    bytes_to_add = target_vsize - base_vsize

    # Now have to figure out the size of varints needed based on bytes to add
    if bytes_to_add <= 12:
        logger.warn("Bytes to add to transaction is smaller than minimum output size")
        data_push_sz = 1
    elif bytes_to_add <= 264:
        # VarInt for scriptPubKey size is 1 byte and 1 byte for PUSHDATA1
        data_push_sz = bytes_to_add - base_output_size - 2
    elif bytes_to_add <= 65550:
        # VarInt for scriptPubKey size is 3 bytes and 2 byte for PUSHDATA2
        data_push_sz = bytes_to_add - base_output_size - 5
    else:
        # VarInt for scriptPubKey size is 5 bytes and 4 byte for PUSHDATA4
        data_push_sz = bytes_to_add - base_output_size - 9

    txouts = gen_return_txouts(numoutputs=1, data_push_sz=data_push_sz)

    return create_transaction_with_outputs(node, txouts, utxo, addr, fee)


# Create a normal transaction form the utxo and then
# splice in the additional ouputs in txouts that are
# generated with gen_return_txouts()
def create_transaction_with_outputs(node, txouts, utxo, addr, fee):
    inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
    outputs = {}
    change = utxo['amount'] - fee
    outputs[addr] = satoshi_round(change)
    rawtx = node.createrawtransaction(inputs, outputs)
    newtx = rawtx[0:92]
    newtx = newtx + txouts
    newtx = newtx + rawtx[94:]
    signresult = node.signrawtransactionwithwallet(newtx, None, "NONE")
    txid = node.sendrawtransaction(signresult["hex"], True)
    return txid


# Create a spend of each passed-in utxo, splicing in "txouts" to each raw
# transaction to make it large.  See gen_return_txouts() above.
def create_lots_of_big_transactions(node, utxos, num, fee):
    txouts = gen_return_txouts()
    addr = node.getnewaddress()
    txids = []
    assert_greater_than_or_equal(num, len(utxos))
    for _ in range(num):
        txids.append(create_transaction_with_outputs(node, txouts, utxos.pop(), addr, fee))
    return txids

def mine_large_block(node, utxos=None):
    # generate a 66k transaction,
    # and 14 of them is close to the 1MB block limit
    num = 14
    utxos = utxos if utxos is not None else []
    if len(utxos) < num:
        utxos.clear()
        utxos.extend(node.listunspent())
    fee = 100 * node.getnetworkinfo()["relayfee"]
    create_lots_of_big_transactions(node, utxos, num, fee=fee)
    node.generate(1)
