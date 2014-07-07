#!/usr/bin/env python

#
# Test fee estimation code
#

# Add python-bitcoinrpc to module search path:
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinrpc"))

import json
import random
import shutil
import subprocess
import tempfile
import traceback

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *


def run_test(nodes, test_dir):
    nodes.append(start_node(0, test_dir,
                            ["-debug=mempool", "-debug=estimatefee"]))
    # Node1 mines small-but-not-tiny blocks, and allows free transactions.
    # NOTE: the CreateNewBlock code starts counting block size at 1,000 bytes,
    # so blockmaxsize of 2,000 is really just 1,000 bytes (room enough for
    # 6 or 7 transactions)
    nodes.append(start_node(1, test_dir,
                            ["-blockprioritysize=1500", "-blockmaxsize=2000",
                             "-debug=mempool", "-debug=estimatefee"]))
    connect_nodes(nodes[1], 0)

    # Node2 is a stingy miner, that
    # produces very small blocks (room for only 3 or so transactions)
    node2args = [ "-blockprioritysize=0", "-blockmaxsize=1500",
                             "-debug=mempool", "-debug=estimatefee"]
    nodes.append(start_node(2, test_dir, node2args))
    connect_nodes(nodes[2], 0)

    sync_blocks(nodes)

    # Prime the memory pool with pairs of transactions
    # (high-priority, random fee and zero-priority, random fee)
    min_fee = Decimal("0.001")
    fees_per_kb = [];
    for i in range(12):
        (txid, txhex, fee) = random_zeropri_transaction(nodes, Decimal("1.1"),
                                                        min_fee, min_fee, 20)
        tx_kbytes = (len(txhex)/2)/1000.0
        fees_per_kb.append(float(fee)/tx_kbytes)

    # Mine blocks with node2 until the memory pool clears:
    count_start = nodes[2].getblockcount()
    while len(nodes[2].getrawmempool()) > 0:
        nodes[2].setgenerate(True, 1)
        sync_blocks(nodes)

    all_estimates = [ nodes[0].estimatefee(i) for i in range(1,20) ]
    print("Fee estimates, super-stingy miner: "+str([str(e) for e in all_estimates]))

    # Estimates should be within the bounds of what transactions fees actually were:
    delta = 1.0e-6 # account for rounding error
    for e in filter(lambda x: x >= 0, all_estimates):
        if float(e)+delta < min(fees_per_kb) or float(e)-delta > max(fees_per_kb):
            raise AssertionError("Estimated fee (%f) out of range (%f,%f)"%(float(e), min_fee_kb, max_fee_kb))

    # Generate transactions while mining 30 more blocks, this time with node1:
    for i in range(30):
        for j in range(random.randrange(6-4,6+4)):
            (txid, txhex, fee) = random_transaction(nodes, Decimal("1.1"),
                                                    Decimal("0.0"), min_fee, 20)
            tx_kbytes = (len(txhex)/2)/1000.0
            fees_per_kb.append(float(fee)/tx_kbytes)
        nodes[1].setgenerate(True, 1)
        sync_blocks(nodes)

    all_estimates = [ nodes[0].estimatefee(i) for i in range(1,20) ]
    print("Fee estimates, more generous miner: "+str([ str(e) for e in all_estimates]))
    for e in filter(lambda x: x >= 0, all_estimates):
        if float(e)+delta < min(fees_per_kb) or float(e)-delta > max(fees_per_kb):
            raise AssertionError("Estimated fee (%f) out of range (%f,%f)"%(float(e), min_fee_kb, max_fee_kb))

    # Finish by mining a normal-sized block:
    while len(nodes[0].getrawmempool()) > 0:
        nodes[0].setgenerate(True, 1)
        sync_blocks(nodes)

    final_estimates = [ nodes[0].estimatefee(i) for i in range(1,20) ]
    print("Final fee estimates: "+str([ str(e) for e in final_estimates]))

def main():
    import optparse

    parser = optparse.OptionParser(usage="%prog [options]")
    parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                      help="Leave bitcoinds and test.* datadir on exit or error")
    parser.add_option("--srcdir", dest="srcdir", default="../../src",
                      help="Source directory containing bitcoind/bitcoin-cli (default: %default%)")
    parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                      help="Root directory for datadirs")
    (options, args) = parser.parse_args()

    os.environ['PATH'] = options.srcdir+":"+os.environ['PATH']

    check_json_precision()

    success = False
    nodes = []
    try:
        print("Initializing test directory "+options.tmpdir)
        print("  node0 running at: 127.0.0.1:%d"%(p2p_port(0)))
        if not os.path.isdir(options.tmpdir):
            os.makedirs(options.tmpdir)
        initialize_chain(options.tmpdir)

        run_test(nodes, options.tmpdir)

        success = True

    except AssertionError as e:
        print("Assertion failed: "+e.message)
    except Exception as e:
        print("Unexpected exception caught during testing: "+str(e))
        traceback.print_tb(sys.exc_info()[2])

    if not options.nocleanup:
        print("Cleaning up")
        stop_nodes(nodes)
        wait_bitcoinds()
        shutil.rmtree(options.tmpdir)

    if success:
        print("Tests successful")
        sys.exit(0)
    else:
        print("Failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
