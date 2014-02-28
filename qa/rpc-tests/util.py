#
# Helpful routines for regression testing
#

# Add python-bitcoinrpc to module search path:
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinrpc"))

from decimal import Decimal
import json
import shutil
import subprocess
import time

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *

START_P2P_PORT=11000
START_RPC_PORT=11100

def check_json_precision():
    """Make sure json library being used does not lose precision converting BTC values"""
    n = Decimal("20000000.00000003")
    satoshis = int(json.loads(json.dumps(float(n)))*1.0e8)
    if satoshis != 2000000000000003:
        raise RuntimeError("JSON encode/decode loses precision")

def sync_blocks(rpc_connections):
    """
    Wait until everybody has the same block count
    """
    while True:
        counts = [ x.getblockcount() for x in rpc_connections ]
        if counts == [ counts[0] ]*len(counts):
            break
        time.sleep(1)

def sync_mempools(rpc_connections):
    """
    Wait until everybody has the same transactions in their memory
    pools
    """
    while True:
        pool = set(rpc_connections[0].getrawmempool())
        num_match = 1
        for i in range(1, len(rpc_connections)):
            if set(rpc_connections[i].getrawmempool()) == pool:
                num_match = num_match+1
        if num_match == len(rpc_connections):
            break
        time.sleep(1)
        

def initialize_chain(test_dir):
    """
    Create (or copy from cache) a 200-block-long chain and
    4 wallets.
    bitcoind and bitcoin-cli must be in search path.
    """

    if not os.path.isdir(os.path.join("cache", "node0")):
        # Create cache directories, run bitcoinds:
        bitcoinds = []
        for i in range(4):
            datadir = os.path.join("cache", "node"+str(i))
            os.makedirs(datadir)
            with open(os.path.join(datadir, "bitcoin.conf"), 'w') as f:
                f.write("regtest=1\n");
                f.write("rpcuser=rt\n");
                f.write("rpcpassword=rt\n");
                f.write("port="+str(START_P2P_PORT+i)+"\n");
                f.write("rpcport="+str(START_RPC_PORT+i)+"\n");
            args = [ "bitcoind", "-keypool=1", "-datadir="+datadir ]
            if i > 0:
                args.append("-connect=127.0.0.1:"+str(START_P2P_PORT))
            bitcoinds.append(subprocess.Popen(args))
            subprocess.check_output([ "bitcoin-cli", "-datadir="+datadir,
                                      "-rpcwait", "getblockcount"])

        rpcs = []
        for i in range(4):
            try:
                url = "http://rt:rt@127.0.0.1:%d"%(START_RPC_PORT+i,)
                rpcs.append(AuthServiceProxy(url))
            except:
                sys.stderr.write("Error connecting to "+url+"\n")
                sys.exit(1)

        import pdb; pdb.set_trace()

        # Create a 200-block-long chain; each of the 4 nodes
        # gets 25 mature blocks and 25 immature.
        for i in range(4):
            rpcs[i].setgenerate(True, 25)
            sync_blocks(rpcs)
        for i in range(4):
            rpcs[i].setgenerate(True, 25)
            sync_blocks(rpcs)
        # Shut them down
        for i in range(4):
            rpcs[i].stop()

    for i in range(4):
        from_dir = os.path.join("cache", "node"+str(i))
        to_dir = os.path.join(test_dir,  "node"+str(i))
        shutil.copytree(from_dir, to_dir)

bitcoind_processes = []

def start_nodes(num_nodes, dir):
    # Start bitcoinds, and wait for RPC interface to be up and running:
    for i in range(num_nodes):
        datadir = os.path.join(dir, "node"+str(i))
        args = [ "bitcoind", "-datadir="+datadir ]
        bitcoind_processes.append(subprocess.Popen(args))
        subprocess.check_output([ "bitcoin-cli", "-datadir="+datadir,
                                  "-rpcwait", "getblockcount"])
    # Create&return JSON-RPC connections
    rpc_connections = []
    for i in range(num_nodes):
        url = "http://rt:rt@127.0.0.1:%d"%(START_RPC_PORT+i,)
        rpc_connections.append(AuthServiceProxy(url))
    return rpc_connections

def stop_nodes():
    for process in bitcoind_processes:
        process.kill()

def connect_nodes(from_connection, node_num):
    ip_port = "127.0.0.1:"+str(START_P2P_PORT+node_num)
    from_connection.addnode(ip_port, "onetry")

def assert_equal(thing1, thing2):
    if thing1 != thing2:
        raise AssertionError("%s != %s"%(str(thing1),str(thing2)))
