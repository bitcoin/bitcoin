#!/usr/bin/env python

# Exercise the listtransactions API

# Add python-bitcoinrpc to module search path:
import os
import sys
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinrpc"))

import json
import shutil
import subprocess
import tempfile
import traceback

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *


def check_array_result(object_array, to_match, expected):
    """
    Pass in array of JSON objects, a dictionary with key/value pairs
    to match against, and another dictionary with expected key/value
    pairs.
    """
    num_matched = 0
    for item in object_array:
        all_match = True
        for key,value in to_match.items():
            if item[key] != value:
                all_match = False
        if not all_match:
            continue
        for key,value in expected.items():
            if item[key] != value:
                raise AssertionError("%s : expected %s=%s"%(str(item), str(key), str(value)))
            num_matched = num_matched+1
    if num_matched == 0:
        raise AssertionError("No objects matched %s"%(str(to_match)))

def run_test(nodes):
    # Simple send, 0 to 1:
    txid = nodes[0].sendtoaddress(nodes[1].getnewaddress(), 0.1)
    sync_mempools(nodes)
    check_array_result(nodes[0].listtransactions(),
                       {"txid":txid},
                       {"category":"send","account":"","amount":Decimal("-0.1"),"confirmations":0})
    check_array_result(nodes[1].listtransactions(),
                       {"txid":txid},
                       {"category":"receive","account":"","amount":Decimal("0.1"),"confirmations":0})
    # mine a block, confirmations should change:
    nodes[0].setgenerate(True, 1)
    sync_blocks(nodes)
    check_array_result(nodes[0].listtransactions(),
                       {"txid":txid},
                       {"category":"send","account":"","amount":Decimal("-0.1"),"confirmations":1})
    check_array_result(nodes[1].listtransactions(),
                       {"txid":txid},
                       {"category":"receive","account":"","amount":Decimal("0.1"),"confirmations":1})

    # send-to-self:
    txid = nodes[0].sendtoaddress(nodes[0].getnewaddress(), 0.2)
    check_array_result(nodes[0].listtransactions(),
                       {"txid":txid, "category":"send"},
                       {"amount":Decimal("-0.2")})
    check_array_result(nodes[0].listtransactions(),
                       {"txid":txid, "category":"receive"},
                       {"amount":Decimal("0.2")})

    # sendmany from node1: twice to self, twice to node2:
    send_to = { nodes[0].getnewaddress() : 0.11, nodes[1].getnewaddress() : 0.22,
                nodes[0].getaccountaddress("from1") : 0.33, nodes[1].getaccountaddress("toself") : 0.44 }
    txid = nodes[1].sendmany("", send_to)
    sync_mempools(nodes)
    check_array_result(nodes[1].listtransactions(),
                       {"category":"send","amount":Decimal("-0.11")},
                       {"txid":txid} )
    check_array_result(nodes[0].listtransactions(),
                       {"category":"receive","amount":Decimal("0.11")},
                       {"txid":txid} )
    check_array_result(nodes[1].listtransactions(),
                       {"category":"send","amount":Decimal("-0.22")},
                       {"txid":txid} )
    check_array_result(nodes[1].listtransactions(),
                       {"category":"receive","amount":Decimal("0.22")},
                       {"txid":txid} )
    check_array_result(nodes[1].listtransactions(),
                       {"category":"send","amount":Decimal("-0.33")},
                       {"txid":txid} )
    check_array_result(nodes[0].listtransactions(),
                       {"category":"receive","amount":Decimal("0.33")},
                       {"txid":txid, "account" : "from1"} )
    check_array_result(nodes[1].listtransactions(),
                       {"category":"send","amount":Decimal("-0.44")},
                       {"txid":txid, "account" : ""} )
    check_array_result(nodes[1].listtransactions(),
                       {"category":"receive","amount":Decimal("0.44")},
                       {"txid":txid, "account" : "toself"} )
    

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
    try:
        print("Initializing test directory "+options.tmpdir)
        if not os.path.isdir(options.tmpdir):
            os.makedirs(options.tmpdir)
        initialize_chain(options.tmpdir)

        nodes = start_nodes(2, options.tmpdir)
        connect_nodes(nodes[1], 0)
        sync_blocks(nodes)
        run_test(nodes)

        success = True

    except AssertionError as e:
        print("Assertion failed: "+e.message)
    except Exception as e:
        print("Unexpected exception caught during testing: "+str(e))
        stack = traceback.extract_tb(sys.exc_info()[2])
        print(stack[-1])

    if not options.nocleanup:
        print("Cleaning up")
        stop_nodes()
        shutil.rmtree(options.tmpdir)

    if success:
        print("Tests successful")
        sys.exit(0)
    else:
        print("Failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
