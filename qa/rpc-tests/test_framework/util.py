#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


#
# Helpful routines for regression testing
#
import pdb
# Add python-bitcoinrpc to module search path:
import os
import sys
import math
import binascii
from binascii import hexlify, unhexlify
from base64 import b64encode
from decimal import Decimal, ROUND_DOWN
import decimal
import json
import http.client
import random
import shutil
import subprocess
import time
import re
import urllib.parse as urlparse
import errno
import logging

from . import coverage
from .authproxy import AuthServiceProxy, JSONRPCException

COVERAGE_DIR = None

DEFAULT_TX_FEE_PER_BYTE = 50
PerfectFractions = True
BTC = 100000000
mBTC = 100000
uBTC = 100
# The maximum number of nodes a single test can spawn
MAX_NODES = 8
# Don't assign rpc or p2p ports lower than this
PORT_MIN = 11000
# The number of ports to "reserve" for p2p and rpc, each
PORT_RANGE = 5000


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
    # Set the mocktime to be after the Bitcoin Cash fork so
    # in normal tests blockchains the fork is in the past
    global MOCKTIME
    MOCKTIME = 1501600000 + (201 * 10 * 60)

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

def sync_blocks(rpc_connections, wait=1,verbose=1):
    """
    Wait until everybody has the same block count
    """
    while True:
        counts = [ x.getblockcount() for x in rpc_connections ]
        if verbose:
            logging.info("sync blocks: " + str(counts))
        if counts == [ counts[0] ]*len(counts):
            break
        time.sleep(wait)

def sync_mempools(rpc_connections, wait=1,verbose=1):
    """
    Wait until everybody has the same transactions in their memory
    pools
    """
    count = 0
    while True:
        count += 1
        pool = set(rpc_connections[0].getrawmempool())
        num_match = 1
        pool_len = [len(pool)]
        for i in range(1, len(rpc_connections)):
            tmp = set(rpc_connections[i].getrawmempool())
            if tmp == pool:
                num_match = num_match+1
            pool_len.append(len(tmp))
        if verbose and count%30==0:
            logging.info("sync mempool: " + str(pool_len))
        if num_match == len(rpc_connections):
            break
        time.sleep(wait)

bitcoind_processes = {}

def initialize_datadir(dirname, n,bitcoinConfDict=None,wallet=None):
    datadir = os.path.join(dirname, "node"+str(n))
    if not os.path.isdir(datadir):
        os.makedirs(datadir)

    defaults = {"server":1, "discover":0, "regtest":1,"rpcuser":"rt","rpcpassword":"rt",
                "port":p2p_port(n),"rpcport":str(rpc_port(n)),"listenonion":0,"maxlimitertxfee":0}
    if bitcoinConfDict: defaults.update(bitcoinConfDict)

    with open(os.path.join(datadir, "bitcoin.conf"), 'w') as f:
        for (key,val) in defaults.items():
          if type(val) is type([]):
            for v in val:
              f.write("%s=%s\n" % (str(key), str(v)))
          else:
            f.write("%s=%s\n"% (str(key), str(val)))

    if wallet:
      regtestdir = os.path.join(datadir,"regtest")
      if not os.path.isdir(regtestdir):
          os.makedirs(regtestdir)
      print(regtestdir, os.path.join(regtestdir, "wallet.dat"))
      shutil.copyfile(wallet,os.path.join(regtestdir, "wallet.dat"))
      
    return datadir

def rpc_url(i, rpchost=None):
    return "http://rt:rt@%s:%d" % (rpchost or '127.0.0.1', rpc_port(i))

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
                raise # unkown JSON RPC exception
        time.sleep(0.25)

def initialize_chain(test_dir,bitcoinConfDict=None,wallets=None):
    """
    Create (or copy from cache) a 200-block-long chain and
    4 wallets.
    """

    if (not os.path.isdir(os.path.join("cache","node0"))
        or not os.path.isdir(os.path.join("cache","node1"))
        or not os.path.isdir(os.path.join("cache","node2"))
        or not os.path.isdir(os.path.join("cache","node3"))):

        #find and delete old cache directories if any exist
        for i in range(4):
            if os.path.isdir(os.path.join("cache","node"+str(i))):
                shutil.rmtree(os.path.join("cache","node"+str(i)))

        # Create cache directories, run bitcoinds:
        for i in range(4):
            datadir=initialize_datadir("cache", i,bitcoinConfDict)
            args = [ os.getenv("BITCOIND", "bitcoind"), "-keypool=1", "-datadir="+datadir ]
            if i > 0:
                args.append("-connect=127.0.0.1:"+str(p2p_port(0)))
            bitcoind_processes[i] = subprocess.Popen(args)
            if os.getenv("PYTHON_DEBUG", ""):
                print("initialize_chain: bitcoind started, waiting for RPC to come up")
            wait_for_bitcoind_start(bitcoind_processes[i], rpc_url(i), i)
            if os.getenv("PYTHON_DEBUG", ""):
                print("initialize_chain: RPC succesfully started")

        rpcs = []
        for i in range(4):
            try:
                rpcs.append(get_rpc_proxy(rpc_url(i), i))
            except:
                sys.stderr.write("Error connecting to "+url+"\n")
                sys.exit(1)

        # Create a 200-block-long chain; each of the 4 nodes
        # gets 25 mature blocks and 25 immature.
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
        wait_bitcoinds()
        disable_mocktime()
        for i in range(4):
            os.remove(log_filename("cache", i, "debug.log"))
            os.remove(log_filename("cache", i, "db.log"))
            os.remove(log_filename("cache", i, "peers.dat"))
            os.remove(log_filename("cache", i, "fee_estimates.dat"))

    for i in range(4):
        from_dir = os.path.join("cache", "node"+str(i))
        to_dir = os.path.join(test_dir,  "node"+str(i))
        shutil.copytree(from_dir, to_dir)
        initialize_datadir(test_dir, i,bitcoinConfDict,wallets[i] if wallets else None) # Overwrite port/rpcport in bitcoin.conf

def initialize_chain_clean(test_dir, num_nodes, bitcoinConfDict=None, wallets=None):
    """
    Create an empty blockchain and num_nodes wallets.
    Useful if a test case wants complete control over initialization.
    """
    for i in range(num_nodes):
        datadir=initialize_datadir(test_dir, i, bitcoinConfDict, wallets)


def _rpchost_to_args(rpchost):
    '''Convert optional IP:port spec to rpcconnect/rpcport args'''
    if rpchost is None:
        return []

    match = re.match('(\[[0-9a-fA-f:]+\]|[^:]+)(?::([0-9]+))?$', rpchost)
    if not match:
        raise ValueError('Invalid RPC host spec ' + rpchost)

    rpcconnect = match.group(1)
    rpcport = match.group(2)

    if rpcconnect.startswith('['): # remove IPv6 [...] wrapping
        rpcconnect = rpcconnect[1:-1]

    rv = ['-rpcconnect=' + rpcconnect]
    if rpcport:
        rv += ['-rpcport=' + rpcport]
    return rv

def start_node(i, dirname, extra_args=None, rpchost=None, timewait=None, binary=None):
    """
    Start a bitcoind and return RPC connection to it
    """
    datadir = os.path.join(dirname, "node"+str(i))
    if binary is None:
        binary = os.getenv("BITCOIND", "bitcoind")
    # RPC tests still depend on free transactions
    args = [ binary, "-datadir="+datadir, "-rest", "-mocktime="+str(get_mocktime()) ] # // BU removed, "-keypool=1","-blockprioritysize=50000" ]
    if extra_args is not None: args.extend(extra_args)
    bitcoind_processes[i] = subprocess.Popen(args)
    if os.getenv("PYTHON_DEBUG", ""):
        print("start_node: bitcoind started, waiting for RPC to come up")
    url = rpc_url(i, rpchost)
    wait_for_bitcoind_start(bitcoind_processes[i], url, i)
    if os.getenv("PYTHON_DEBUG", ""):
        print("start_node: RPC succesfully started")
    proxy = get_rpc_proxy(url, i, timeout=timewait)

    if COVERAGE_DIR:
        coverage.write_all_rpc_commands(COVERAGE_DIR, proxy)

    return proxy

def start_nodes(num_nodes, dirname, extra_args=None, rpchost=None, binary=None,timewait=None):
    """
    Start multiple bitcoinds, return RPC connections to them
    """
    if extra_args is None: extra_args = [ None for _ in range(num_nodes) ]
    if binary is None: binary = [ None for _ in range(num_nodes) ]
    rpcs = []
    try:
        for i in range(num_nodes):
            rpcs.append(start_node(i, dirname, extra_args[i], rpchost, binary=binary[i],timewait=timewait))
    except: # If one node failed to start, stop the others
        stop_nodes(rpcs)
        raise
    return rpcs

def log_filename(dirname, n_node, logname):
    return os.path.join(dirname, "node"+str(n_node), "regtest", logname)

def stop_node(node, i):
    try:
        node.stop()
    except http.client.CannotSendRequest as e:
        print("WARN: Unable to stop node: " + repr(e))
    bitcoind_processes[i].wait()
    del bitcoind_processes[i]

def stop_nodes(nodes):
    for node in nodes:
        try:
            node.stop()
        except http.client.CannotSendRequest as e:
            print("WARN: Unable to stop node: " + repr(e))
    del nodes[:] # Emptying array closes connections as a side effect

def set_node_times(nodes, t):
    for node in nodes:
        node.setmocktime(t)

def wait_bitcoinds():
    # Wait for all bitcoinds to cleanly exit
    for bitcoind in bitcoind_processes.values():
        bitcoind.wait()
    bitcoind_processes.clear()

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

def interconnect_nodes(nodes):
    """Connect every node in this list to every other node in the list"""
    for frm in nodes:
      for to in nodes:
        if frm == to: continue
        up = urlparse.urlparse(to.url)
        ip_port = up.hostname + ":" + str(up.port - PORT_RANGE)  # this is the RPC port but we want the p2p port so -1000
        frm.addnode(ip_port, "onetry")

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

def send_zeropri_transaction(from_node, to_node, amount, fee):
    """
    Create&broadcast a zero-priority transaction.
    Returns (txid, hex-encoded-txdata)
    Ensures transaction is zero-priority by first creating a send-to-self,
    then using its output
    """

    # Create a send-to-self with confirmed inputs:
    self_address = from_node.getnewaddress()
    (total_in, inputs) = gather_inputs(from_node, amount+fee*2)
    outputs = make_change(from_node, total_in, amount+fee, fee)
    outputs[self_address] = float(amount+fee)

    self_rawtx = from_node.createrawtransaction(inputs, outputs)
    self_signresult = from_node.signrawtransaction(self_rawtx)
    self_txid = from_node.sendrawtransaction(self_signresult["hex"], True)

    vout = find_output(from_node, self_txid, amount+fee)
    # Now immediately spend the output to create a 1-input, 1-output
    # zero-priority transaction:
    inputs = [ { "txid" : self_txid, "vout" : vout } ]
    outputs = { to_node.getnewaddress() : float(amount) }

    rawtx = from_node.createrawtransaction(inputs, outputs)
    signresult = from_node.signrawtransaction(rawtx)
    txid = from_node.sendrawtransaction(signresult["hex"], True)

    return (txid, signresult["hex"])

def random_zeropri_transaction(nodes, amount, min_fee, fee_increment, fee_variants):
    """
    Create a random zero-priority transaction.
    Returns (txid, hex-encoded-transaction-data, fee)
    """
    from_node = random.choice(nodes)
    to_node = random.choice(nodes)
    fee = min_fee + fee_increment*random.randint(0,fee_variants)
    (txid, txhex) = send_zeropri_transaction(from_node, to_node, amount, fee)
    return (txid, txhex, fee)

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

def split_transaction(node, prevouts, toAddrs, txfeePer=DEFAULT_TX_FEE_PER_BYTE,**kwargs):
    """
      Create a transaction that divides the sum of all the passed utxos into all the destination addresses
      pass:
           node: (node object) where to send the RPC calls 
           prevouts: a single UTXO description dictionary, or a list of them
           toAddrs: a list of strings specifying the output addresses
           "sendtx=False" if you don't want to transaction to be submitted.
      Returns (transaction in hex, Vin list, Vout list)
    """
    if type(prevouts) == type({}): prevouts = [prevouts]  # If the user passes just one transaction then put a list around it 
    txid = None
    inp = []
    decContext = decimal.getcontext().prec
    try:  # try finally block to put the decimal precision back to what it was prior to this routine
      decimal.getcontext().prec = 8 + 8 # 8 digits to get to 21million, and each bitcoin is 100 million satoshis
      amount = Decimal(0)
      iamount = 0
      count = 0
      for tx in prevouts:
        inp.append({"txid":str(tx["txid"]),"vout":tx["vout"]})
        amount += tx["amount"]*Decimal(BTC)
        iamount += int(tx["amount"]*Decimal(BTC))
        count += 1

      assert(amount == iamount) # make sure Decimal and integer math is consistent
      txLen = (len(prevouts)*100) + (len(toAddrs)*100)  # Guess the tx Size

      while 1:
          outp = {}
          if amount - Decimal(txfeePer*txLen) < 0:  # fee too big, find something smaller
            txfeePer = (float(amount)/txLen)/1.5

          txfee = int(math.ceil(txfeePer*txLen))
          amtPer = (Decimal(amount-txfee)/len(toAddrs)).to_integral_value()
          # print "amount: ", amount, " amount per: ", amtPer, "from :", len(prevouts), "to: ", len(toAddrs), "tx fee: ", txfeePer, txfee

          for a in toAddrs[0:-1]:
            if PerfectFractions:
              outp[str(a)] = str(amtPer/Decimal(BTC))
            else:
              outp[str(a)] = float(amtPer/BTC)

          a = toAddrs[-1]
          amtPer = (amount - ((len(toAddrs)-1)*amtPer)) - txfee
          # print "final amt: ", amtPer
          if PerfectFractions:
              outp[str(a)] = str(amtPer/BTC)
          else:
              outp[str(a)] = float(amtPer/BTC)

          totalOutputs = sum([Decimal(x) for x in outp.values()])
          assert(totalOutputs < amount)
          txn = node.createrawtransaction(inp, outp)
          if kwargs.get("sendtx",True):
              #print time.strftime('%X %x %Z')
              try:
                  s = str(txn)
                  # print "tx len: ", len(binascii.unhexlify(s))
                  signedtxn = node.signrawtransaction(s)
                  txLen = len(binascii.unhexlify(signedtxn["hex"]))  # Get the actual transaction size for better tx fee estimation the next time around
              finally:
                  #print time.strftime('%X %x %Z')
                  pass

              if signedtxn["complete"]:
                  try:
                      txid = node.sendrawtransaction(signedtxn["hex"],True)  # In the unit tests, we'll just allow high fees
                      return (txn,inp,outp,txid)
                  except JSONRPCException as e:
                      tmp = e.error["message"]
                      (code, msg) = tmp.split(":")
                      if int(code) == 64: raise # bad transaction
                      if int(code) == 258: # txn-mempool-conflict
                          # we are reusing inputs so this is all the splitting we can do
                          return (txn,inp,outp,txid)
                      # print tmp
                      if e.error["code"] == -26:  # insufficient priority
                          txfeePer = txfeePer * 2
                          print(str(e))
                          print("Insufficient priority, raising tx fee per byte to: ", txfeePer)
                          continue
                      else:
                          raise
              else:
                  for err in signedtxn["errors"]:
                      print(err["error"])
          else:
              return (txn,inp,outp,txid)
    finally:
      decimal.getcontext().prec = decContext

def assert_not_equal(thing1, thing2):
    if thing1 == thing2:
        raise AssertionError("%s != %s"%(str(thing1),str(thing2)))

def assert_equal(thing1, thing2):
    if thing1 != thing2:
        raise AssertionError("%s != %s"%(str(thing1),str(thing2)))

def assert_greater_than(thing1, thing2):
    if thing1 <= thing2:
        raise AssertionError("%s <= %s"%(str(thing1),str(thing2)))

def assert_raises(exc, fun, *args, **kwds):
    try:
        fun(*args, **kwds)
    except exc:
        pass
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
def create_lots_of_big_transactions(node, txouts, utxos, fee):
    addr = node.getnewaddress()
    txids = []
    for i in range(len(utxos)):
        t = utxos.pop()
        inputs = []
        inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
        outputs = {}
        send_value = t['amount'] - fee
        outputs[addr] = satoshi_round(send_value)
        rawtx = node.createrawtransaction(inputs, outputs)
        newtx = rawtx[0:92]
        newtx = newtx + txouts
        newtx = newtx + rawtx[94:]
        signresult = node.signrawtransaction(newtx, None, None, "NONE")
        txid = node.sendrawtransaction(signresult["hex"], True)
        txids.append(txid)
    return txids

def get_bip9_status(node, key):
    info = node.getblockchaininfo()
    return info['bip9_softforks'][key]
