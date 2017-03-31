#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpful routines for regression testing."""

import os
import sys

from binascii import hexlify, unhexlify
from base64 import b64encode
from decimal import Decimal, ROUND_DOWN
import json
import http.client
import random
import shutil
import subprocess
import tempfile
import time
import re
import errno
import logging

from . import coverage
from .authproxy import AuthServiceProxy, JSONRPCException

COVERAGE_DIR = None

logger = logging.getLogger("TestFramework.utils")

# The maximum number of nodes a single test can spawn
MAX_NODES = 8
# Don't assign rpc or p2p ports lower than this
PORT_MIN = 11000
# The number of ports to "reserve" for p2p and rpc, each
PORT_RANGE = 5000

BITCOIND_PROC_WAIT_TIMEOUT = 60


class PortSeed:
    # Must be initialized with a unique integer for each process
    n = None

#Set Mocktime default to OFF.
#MOCKTIME is only needed for scripts that use the
#cached version of the blockchain.  If the cached
#version of the blockchain is used without MOCKTIME
#then the mempools will not sync due to IBD.
MOCKTIME = 0

def enable_mocktime():
    #For backwared compatibility of the python scripts
    #with previous versions of the cache, set MOCKTIME 
    #to Jan 1, 2014 + (201 * 10 * 60)
    global MOCKTIME
    MOCKTIME = 1388534400 + (201 * 10 * 60)

def disable_mocktime():
    global MOCKTIME
    MOCKTIME = 0

def get_mocktime():
    return MOCKTIME

def enable_coverage(dirname):
    """Maintain a log of which RPC calls are made during testing."""
    global COVERAGE_DIR
    COVERAGE_DIR = dirname


def get_rpc_proxy(url, node_number, timeout=None):
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
        COVERAGE_DIR, node_number) if COVERAGE_DIR else None

    return coverage.AuthServiceProxyWrapper(proxy, coverage_logfile)


def p2p_port(n):
    assert(n <= MAX_NODES)
    return PORT_MIN + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)

def rpc_port(n):
    return PORT_MIN + PORT_RANGE + n + (MAX_NODES * PortSeed.n) % (PORT_RANGE - 1 - MAX_NODES)

def check_json_precision():
    """Make sure json library being used does not lose precision converting BTC values"""
    n = Decimal("20000000.00000003")
    satoshis = int(json.loads(json.dumps(float(n)))*1.0e8)
    if satoshis != 2000000000000003:
        raise RuntimeError("JSON encode/decode loses precision")

def count_bytes(hex_string):
    return len(bytearray.fromhex(hex_string))

def bytes_to_hex_str(byte_str):
    return hexlify(byte_str).decode('ascii')

def hex_str_to_bytes(hex_str):
    return unhexlify(hex_str.encode('ascii'))

def str_to_b64str(string):
    return b64encode(string.encode('utf-8')).decode('ascii')

def sync_blocks(rpc_connections, *, wait=1, timeout=60):
    """
    Wait until everybody has the same tip.

    sync_blocks needs to be called with an rpc_connections set that has least
    one node already synced to the latest, stable tip, otherwise there's a
    chance it might return before all nodes are stably synced.
    """
    # Use getblockcount() instead of waitforblockheight() to determine the
    # initial max height because the two RPCs look at different internal global
    # variables (chainActive vs latestBlock) and the former gets updated
    # earlier.
    maxheight = max(x.getblockcount() for x in rpc_connections)
    start_time = cur_time = time.time()
    while cur_time <= start_time + timeout:
        tips = [r.waitforblockheight(maxheight, int(wait * 1000)) for r in rpc_connections]
        if all(t["height"] == maxheight for t in tips):
            if all(t["hash"] == tips[0]["hash"] for t in tips):
                return
            raise AssertionError("Block sync failed, mismatched block hashes:{}".format(
                                 "".join("\n  {!r}".format(tip) for tip in tips)))
        cur_time = time.time()
    raise AssertionError("Block sync to height {} timed out:{}".format(
                         maxheight, "".join("\n  {!r}".format(tip) for tip in tips)))

def sync_chain(rpc_connections, *, wait=1, timeout=60):
    """
    Wait until everybody has the same best block
    """
    while timeout > 0:
        best_hash = [x.getbestblockhash() for x in rpc_connections]
        if best_hash == [best_hash[0]]*len(best_hash):
            return
        time.sleep(wait)
        timeout -= wait
    raise AssertionError("Chain sync failed: Best block hashes don't match")

def sync_mempools(rpc_connections, *, wait=1, timeout=60):
    """
    Wait until everybody has the same transactions in their memory
    pools
    """
    while timeout > 0:
        pool = set(rpc_connections[0].getrawmempool())
        num_match = 1
        for i in range(1, len(rpc_connections)):
            if set(rpc_connections[i].getrawmempool()) == pool:
                num_match = num_match+1
        if num_match == len(rpc_connections):
            return
        time.sleep(wait)
        timeout -= wait
    raise AssertionError("Mempool sync failed")

bitcoind_processes = {}

def initialize_datadir(dirname, n):
    datadir = os.path.join(dirname, "node"+str(n))
    if not os.path.isdir(datadir):
        os.makedirs(datadir)
    rpc_u, rpc_p = rpc_auth_pair(n)
    with open(os.path.join(datadir, "bitcoin.conf"), 'w', encoding='utf8') as f:
        f.write("regtest=1\n")
        f.write("rpcuser=" + rpc_u + "\n")
        f.write("rpcpassword=" + rpc_p + "\n")
        f.write("port="+str(p2p_port(n))+"\n")
        f.write("rpcport="+str(rpc_port(n))+"\n")
        f.write("listenonion=0\n")
    return datadir

def rpc_auth_pair(n):
    return 'rpcuser💻' + str(n), 'rpcpass🔑' + str(n)

def rpc_url(i, rpchost=None):
    rpc_u, rpc_p = rpc_auth_pair(i)
    host = '127.0.0.1'
    port = rpc_port(i)
    if rpchost:
        parts = rpchost.split(':')
        if len(parts) == 2:
            host, port = parts
        else:
            host = rpchost
    return "http://%s:%s@%s:%d" % (rpc_u, rpc_p, host, int(port))

def wait_for_bitcoind_start(process, url, i):
    '''
    Wait for bitcoind to start. This means that RPC is accessible and fully initialized.
    Raise an exception if bitcoind exits during initialization.
    '''
    while True:
        if process.poll() is not None:
            raise Exception('bitcoind exited with status %i during initialization' % process.returncode)
        try:
            rpc = get_rpc_proxy(url, i)
            blocks = rpc.getblockcount()
            break # break out of loop on success
        except IOError as e:
            if e.errno != errno.ECONNREFUSED: # Port not yet open?
                raise # unknown IO error
        except JSONRPCException as e: # Initialization phase
            if e.error['code'] != -28: # RPC in warmup?
                raise # unknown JSON RPC exception
        time.sleep(0.25)

def initialize_chain(test_dir, num_nodes, cachedir):
    """
    Create a cache of a 200-block-long chain (with wallet) for MAX_NODES
    Afterward, create num_nodes copies from the cache
    """

    assert num_nodes <= MAX_NODES
    create_cache = False
    for i in range(MAX_NODES):
        if not os.path.isdir(os.path.join(cachedir, 'node'+str(i))):
            create_cache = True
            break

    if create_cache:
        logger.debug("Creating data directories from cached datadir")

        #find and delete old cache directories if any exist
        for i in range(MAX_NODES):
            if os.path.isdir(os.path.join(cachedir,"node"+str(i))):
                shutil.rmtree(os.path.join(cachedir,"node"+str(i)))

        # Create cache directories, run bitcoinds:
        for i in range(MAX_NODES):
            datadir=initialize_datadir(cachedir, i)
            args = [ os.getenv("BITCOIND", "bitcoind"), "-server", "-keypool=1", "-datadir="+datadir, "-discover=0" ]
            if i > 0:
                args.append("-connect=127.0.0.1:"+str(p2p_port(0)))
            bitcoind_processes[i] = subprocess.Popen(args)
            logger.debug("initialize_chain: bitcoind started, waiting for RPC to come up")
            wait_for_bitcoind_start(bitcoind_processes[i], rpc_url(i), i)
            logger.debug("initialize_chain: RPC successfully started")

        rpcs = []
        for i in range(MAX_NODES):
            try:
                rpcs.append(get_rpc_proxy(rpc_url(i), i))
            except:
                sys.stderr.write("Error connecting to "+url+"\n")
                sys.exit(1)

        # Create a 200-block-long chain; each of the 4 first nodes
        # gets 25 mature blocks and 25 immature.
        # Note: To preserve compatibility with older versions of
        # initialize_chain, only 4 nodes will generate coins.
        #
        # blocks are created with timestamps 10 minutes apart
        # starting from 2010 minutes in the past
        enable_mocktime()
        block_time = get_mocktime() - (201 * 10 * 60)
        for i in range(2):
            for peer in range(4):
                for j in range(25):
                    set_node_times(rpcs, block_time)
                    rpcs[peer].generate(1)
                    block_time += 10*60
                # Must sync before next peer starts generating blocks
                sync_blocks(rpcs)

        # Shut them down, and clean up cache directories:
        stop_nodes(rpcs)
        disable_mocktime()
        for i in range(MAX_NODES):
            os.remove(log_filename(cachedir, i, "debug.log"))
            os.remove(log_filename(cachedir, i, "db.log"))
            os.remove(log_filename(cachedir, i, "peers.dat"))
            os.remove(log_filename(cachedir, i, "fee_estimates.dat"))

    for i in range(num_nodes):
        from_dir = os.path.join(cachedir, "node"+str(i))
        to_dir = os.path.join(test_dir,  "node"+str(i))
        shutil.copytree(from_dir, to_dir)
        initialize_datadir(test_dir, i) # Overwrite port/rpcport in bitcoin.conf

def initialize_chain_clean(test_dir, num_nodes):
    """
    Create an empty blockchain and num_nodes wallets.
    Useful if a test case wants complete control over initialization.
    """
    for i in range(num_nodes):
        datadir=initialize_datadir(test_dir, i)


def start_node(i, dirname, extra_args=None, rpchost=None, timewait=None, binary=None, stderr=None):
    """
    Start a bitcoind and return RPC connection to it
    """
    datadir = os.path.join(dirname, "node"+str(i))
    if binary is None:
        binary = os.getenv("BITCOIND", "bitcoind")
    args = [binary, "-datadir=" + datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros", "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(get_mocktime())]
    if extra_args is not None: args.extend(extra_args)
    bitcoind_processes[i] = subprocess.Popen(args, stderr=stderr)
    logger.debug("initialize_chain: bitcoind started, waiting for RPC to come up")
    url = rpc_url(i, rpchost)
    wait_for_bitcoind_start(bitcoind_processes[i], url, i)
    logger.debug("initialize_chain: RPC successfully started")
    proxy = get_rpc_proxy(url, i, timeout=timewait)

    if COVERAGE_DIR:
        coverage.write_all_rpc_commands(COVERAGE_DIR, proxy)

    return proxy

def assert_start_raises_init_error(i, dirname, extra_args=None, expected_msg=None):
    with tempfile.SpooledTemporaryFile(max_size=2**16) as log_stderr:
        try:
            node = start_node(i, dirname, extra_args, stderr=log_stderr)
            stop_node(node, i)
        except Exception as e:
            assert 'bitcoind exited' in str(e) #node must have shutdown
            if expected_msg is not None:
                log_stderr.seek(0)
                stderr = log_stderr.read().decode('utf-8')
                if expected_msg not in stderr:
                    raise AssertionError("Expected error \"" + expected_msg + "\" not found in:\n" + stderr)
        else:
            if expected_msg is None:
                assert_msg = "bitcoind should have exited with an error"
            else:
                assert_msg = "bitcoind should have exited with expected error " + expected_msg
            raise AssertionError(assert_msg)

def start_nodes(num_nodes, dirname, extra_args=None, rpchost=None, timewait=None, binary=None):
    """
    Start multiple bitcoinds, return RPC connections to them
    """
    if extra_args is None: extra_args = [ None for _ in range(num_nodes) ]
    if binary is None: binary = [ None for _ in range(num_nodes) ]
    rpcs = []
    try:
        for i in range(num_nodes):
            rpcs.append(start_node(i, dirname, extra_args[i], rpchost, timewait=timewait, binary=binary[i]))
    except: # If one node failed to start, stop the others
        stop_nodes(rpcs)
        raise
    return rpcs

def log_filename(dirname, n_node, logname):
    return os.path.join(dirname, "node"+str(n_node), "regtest", logname)

def stop_node(node, i):
    logger.debug("Stopping node %d" % i)
    try:
        node.stop()
    except http.client.CannotSendRequest as e:
        logger.exception("Unable to stop node")
    return_code = bitcoind_processes[i].wait(timeout=BITCOIND_PROC_WAIT_TIMEOUT)
    assert_equal(return_code, 0)
    del bitcoind_processes[i]

def stop_nodes(nodes):
    for i, node in enumerate(nodes):
        stop_node(node, i)
    assert not bitcoind_processes.values() # All connections must be gone now

def set_node_times(nodes, t):
    for node in nodes:
        node.setmocktime(t)

def connect_nodes(from_connection, node_num):
    ip_port = "127.0.0.1:"+str(p2p_port(node_num))
    from_connection.addnode(ip_port, "onetry")
    # poll until version handshake complete to avoid race conditions
    # with transaction relaying
    while any(peer['version'] == 0 for peer in from_connection.getpeerinfo()):
        time.sleep(0.1)

def connect_nodes_bi(nodes, a, b):
    connect_nodes(nodes[a], b)
    connect_nodes(nodes[b], a)

def find_output(node, txid, amount):
    """
    Return index to output of txid with value amount
    Raises exception if there is none.
    """
    txdata = node.getrawtransaction(txid, 1)
    for i in range(len(txdata["vout"])):
        if txdata["vout"][i]["value"] == amount:
            return i
    raise RuntimeError("find_output txid %s : %s not found"%(txid,str(amount)))


def gather_inputs(from_node, amount_needed, confirmations_required=1):
    """
    Return a random set of unspent txouts that are enough to pay amount_needed
    """
    assert(confirmations_required >=0)
    utxo = from_node.listunspent(confirmations_required)
    random.shuffle(utxo)
    inputs = []
    total_in = Decimal("0.00000000")
    while total_in < amount_needed and len(utxo) > 0:
        t = utxo.pop()
        total_in += t["amount"]
        inputs.append({ "txid" : t["txid"], "vout" : t["vout"], "address" : t["address"] } )
    if total_in < amount_needed:
        raise RuntimeError("Insufficient funds: need %d, have %d"%(amount_needed, total_in))
    return (total_in, inputs)

def make_change(from_node, amount_in, amount_out, fee):
    """
    Create change output(s), return them
    """
    outputs = {}
    amount = amount_out+fee
    change = amount_in - amount
    if change > amount*2:
        # Create an extra change output to break up big inputs
        change_address = from_node.getnewaddress()
        # Split change in two, being careful of rounding:
        outputs[change_address] = Decimal(change/2).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)
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
    fee = min_fee + fee_increment*random.randint(0,fee_variants)

    (total_in, inputs) = gather_inputs(from_node, amount+fee)
    outputs = make_change(from_node, total_in, amount, fee)
    outputs[to_node.getnewaddress()] = float(amount)

    rawtx = from_node.createrawtransaction(inputs, outputs)
    signresult = from_node.signrawtransaction(rawtx)
    txid = from_node.sendrawtransaction(signresult["hex"], True)

    return (txid, signresult["hex"], fee)

def assert_fee_amount(fee, tx_size, fee_per_kB):
    """Assert the fee was in range"""
    target_fee = tx_size * fee_per_kB / 1000
    if fee < target_fee:
        raise AssertionError("Fee of %s BTC too low! (Should be %s BTC)"%(str(fee), str(target_fee)))
    # allow the wallet's estimation to be at most 2 bytes off
    if fee > (tx_size + 2) * fee_per_kB / 1000:
        raise AssertionError("Fee of %s BTC too high! (Should be %s BTC)"%(str(fee), str(target_fee)))

def assert_equal(thing1, thing2, *args):
    if thing1 != thing2 or any(thing1 != arg for arg in args):
        raise AssertionError("not(%s)" % " == ".join(str(arg) for arg in (thing1, thing2) + args))

def assert_greater_than(thing1, thing2):
    if thing1 <= thing2:
        raise AssertionError("%s <= %s"%(str(thing1),str(thing2)))

def assert_greater_than_or_equal(thing1, thing2):
    if thing1 < thing2:
        raise AssertionError("%s < %s"%(str(thing1),str(thing2)))

def assert_raises(exc, fun, *args, **kwds):
    assert_raises_message(exc, None, fun, *args, **kwds)

def assert_raises_message(exc, message, fun, *args, **kwds):
    try:
        fun(*args, **kwds)
    except exc as e:
        if message is not None and message not in e.error['message']:
            raise AssertionError("Expected substring not found:"+e.error['message'])
    except Exception as e:
        raise AssertionError("Unexpected exception raised: "+type(e).__name__)
    else:
        raise AssertionError("No exception raised")

def assert_raises_jsonrpc(code, message, fun, *args, **kwds):
    """Run an RPC and verify that a specific JSONRPC exception code and message is raised.

    Calls function `fun` with arguments `args` and `kwds`. Catches a JSONRPCException
    and verifies that the error code and message are as expected. Throws AssertionError if
    no JSONRPCException was returned or if the error code/message are not as expected.

    Args:
        code (int), optional: the error code returned by the RPC call (defined
            in src/rpc/protocol.h). Set to None if checking the error code is not required.
        message (string), optional: [a substring of] the error string returned by the
            RPC call. Set to None if checking the error string is not required
        fun (function): the function to call. This should be the name of an RPC.
        args*: positional arguments for the function.
        kwds**: named arguments for the function.
    """
    try:
        fun(*args, **kwds)
    except JSONRPCException as e:
        # JSONRPCException was thrown as expected. Check the code and message values are correct.
        if (code is not None) and (code != e.error["code"]):
            raise AssertionError("Unexpected JSONRPC error code %i" % e.error["code"])
        if (message is not None) and (message not in e.error['message']):
            raise AssertionError("Expected substring not found:"+e.error['message'])
    except Exception as e:
        raise AssertionError("Unexpected exception raised: "+type(e).__name__)
    else:
        raise AssertionError("No exception raised")

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

def assert_array_result(object_array, to_match, expected, should_not_find = False):
    """
        Pass in array of JSON objects, a dictionary with key/value pairs
        to match against, and another dictionary with expected key/value
        pairs.
        If the should_not_find flag is true, to_match should not be found
        in object_array
        """
    if should_not_find == True:
        assert_equal(expected, { })
    num_matched = 0
    for item in object_array:
        all_match = True
        for key,value in to_match.items():
            if item[key] != value:
                all_match = False
        if not all_match:
            continue
        elif should_not_find == True:
            num_matched = num_matched+1
        for key,value in expected.items():
            if item[key] != value:
                raise AssertionError("%s : expected %s=%s"%(str(item), str(key), str(value)))
            num_matched = num_matched+1
    if num_matched == 0 and should_not_find != True:
        raise AssertionError("No objects matched %s"%(str(to_match)))
    if num_matched > 0 and should_not_find == True:
        raise AssertionError("Objects were found %s"%(str(to_match)))

def satoshi_round(amount):
    return Decimal(amount).quantize(Decimal('0.00000001'), rounding=ROUND_DOWN)

# Helper to create at least "count" utxos
# Pass in a fee that is sufficient for relay and mining new transactions.
def create_confirmed_utxos(fee, node, count):
    node.generate(int(0.5*count)+101)
    utxos = node.listunspent()
    iterations = count - len(utxos)
    addr1 = node.getnewaddress()
    addr2 = node.getnewaddress()
    if iterations <= 0:
        return utxos
    for i in range(iterations):
        t = utxos.pop()
        inputs = []
        inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
        outputs = {}
        send_value = t['amount'] - fee
        outputs[addr1] = satoshi_round(send_value/2)
        outputs[addr2] = satoshi_round(send_value/2)
        raw_tx = node.createrawtransaction(inputs, outputs)
        signed_tx = node.signrawtransaction(raw_tx)["hex"]
        txid = node.sendrawtransaction(signed_tx)

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
    script_pubkey = "6a4d0200" #OP_RETURN OP_PUSH2 512 bytes
    for i in range (512):
        script_pubkey = script_pubkey + "01"
    # concatenate 128 txouts of above script_pubkey which we'll insert before the txout for change
    txouts = "81"
    for k in range(128):
        # add txout value
        txouts = txouts + "0000000000000000"
        # add length of script_pubkey
        txouts = txouts + "fd0402"
        # add script_pubkey
        txouts = txouts + script_pubkey
    return txouts

def create_tx(node, coinbase, to_address, amount):
    inputs = [{ "txid" : coinbase, "vout" : 0}]
    outputs = { to_address : amount }
    rawtx = node.createrawtransaction(inputs, outputs)
    signresult = node.signrawtransaction(rawtx)
    assert_equal(signresult["complete"], True)
    return signresult["hex"]

# Create a spend of each passed-in utxo, splicing in "txouts" to each raw
# transaction to make it large.  See gen_return_txouts() above.
def create_lots_of_big_transactions(node, txouts, utxos, num, fee):
    addr = node.getnewaddress()
    txids = []
    for _ in range(num):
        t = utxos.pop()
        inputs=[{ "txid" : t["txid"], "vout" : t["vout"]}]
        outputs = {}
        change = t['amount'] - fee
        outputs[addr] = satoshi_round(change)
        rawtx = node.createrawtransaction(inputs, outputs)
        newtx = rawtx[0:92]
        newtx = newtx + txouts
        newtx = newtx + rawtx[94:]
        signresult = node.signrawtransaction(newtx, None, None, "NONE")
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

def get_bip9_status(node, key):
    info = node.getblockchaininfo()
    return info['bip9_softforks'][key]
