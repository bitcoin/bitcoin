#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Base class for RPC testing

import logging
import optparse
import os
import sys
import shutil
import tempfile
import traceback
from time import time, sleep

from .util import (
    initialize_chain,
    start_node,
    start_nodes,
    connect_nodes_bi,
    connect_nodes,
    sync_blocks,
    sync_mempools,
    sync_masternodes,
    stop_nodes,
    stop_node,
    enable_coverage,
    check_json_precision,
    initialize_chain_clean,
    PortSeed,
    set_cache_mocktime,
    set_genesis_mocktime,
    get_mocktime,
    set_mocktime,
    set_node_times,
    p2p_port,
    satoshi_round
)
from .authproxy import JSONRPCException


class BitcoinTestFramework(object):

    def __init__(self):
        self.num_nodes = 4
        self.setup_clean_chain = False
        self.nodes = None

    def run_test(self):
        raise NotImplementedError

    def add_options(self, parser):
        pass

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        if self.setup_clean_chain:
            initialize_chain_clean(self.options.tmpdir, self.num_nodes)
            set_genesis_mocktime()
        else:
            initialize_chain(self.options.tmpdir, self.num_nodes, self.options.cachedir)
            set_cache_mocktime()

    def stop_node(self, num_node):
        stop_node(self.nodes[num_node], num_node)

    def setup_nodes(self):
        return start_nodes(self.num_nodes, self.options.tmpdir)

    def setup_network(self, split = False):
        self.nodes = self.setup_nodes()

        # Connect the nodes as a "chain".  This allows us
        # to split the network between nodes 1 and 2 to get
        # two halves that can work on competing chains.

        # If we joined network halves, connect the nodes from the joint
        # on outward.  This ensures that chains are properly reorganised.
        if not split:
            connect_nodes_bi(self.nodes, 1, 2)
            sync_blocks(self.nodes[1:3])
            sync_mempools(self.nodes[1:3])

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 2, 3)
        self.is_network_split = split
        self.sync_all()

    def split_network(self):
        """
        Split the network of four nodes into nodes 0/1 and 2/3.
        """
        assert not self.is_network_split
        stop_nodes(self.nodes)
        self.setup_network(True)

    def sync_all(self):
        if self.is_network_split:
            sync_blocks(self.nodes[:2])
            sync_blocks(self.nodes[2:])
            sync_mempools(self.nodes[:2])
            sync_mempools(self.nodes[2:])
        else:
            sync_blocks(self.nodes)
            sync_mempools(self.nodes)

    def join_network(self):
        """
        Join the (previously split) network halves together.
        """
        assert self.is_network_split
        stop_nodes(self.nodes)
        self.setup_network(False)

    def main(self):

        parser = optparse.OptionParser(usage="%prog [options]")
        parser.add_option("--nocleanup", dest="nocleanup", default=False, action="store_true",
                          help="Leave dashds and test.* datadir on exit or error")
        parser.add_option("--noshutdown", dest="noshutdown", default=False, action="store_true",
                          help="Don't stop dashds after the test execution")
        parser.add_option("--srcdir", dest="srcdir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__))+"/../../../src"),
                          help="Source directory containing dashd/dash-cli (default: %default)")
        parser.add_option("--cachedir", dest="cachedir", default=os.path.normpath(os.path.dirname(os.path.realpath(__file__))+"/../../cache"),
                          help="Directory for caching pregenerated datadirs")
        parser.add_option("--tmpdir", dest="tmpdir", default=tempfile.mkdtemp(prefix="test"),
                          help="Root directory for datadirs")
        parser.add_option("--tracerpc", dest="trace_rpc", default=False, action="store_true",
                          help="Print out all RPC calls as they are made")
        parser.add_option("--portseed", dest="port_seed", default=os.getpid(), type='int',
                          help="The seed to use for assigning port numbers (default: current process id)")
        parser.add_option("--coveragedir", dest="coveragedir",
                          help="Write tested RPC commands into this directory")
        self.add_options(parser)
        (self.options, self.args) = parser.parse_args()

        # backup dir variable for removal at cleanup
        self.options.root, self.options.tmpdir = self.options.tmpdir, self.options.tmpdir + '/' + str(self.options.port_seed)

        if self.options.trace_rpc:
            logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)

        if self.options.coveragedir:
            enable_coverage(self.options.coveragedir)

        PortSeed.n = self.options.port_seed

        os.environ['PATH'] = self.options.srcdir+":"+self.options.srcdir+"/qt:"+os.environ['PATH']

        check_json_precision()

        success = False
        try:
            os.makedirs(self.options.tmpdir, exist_ok=False)
            self.setup_chain()
            self.setup_network()
            self.run_test()
            success = True
        except JSONRPCException as e:
            print("JSONRPC error: "+e.error['message'])
            traceback.print_tb(sys.exc_info()[2])
        except AssertionError as e:
            print("Assertion failed: " + str(e))
            traceback.print_tb(sys.exc_info()[2])
        except KeyError as e:
            print("key not found: "+ str(e))
            traceback.print_tb(sys.exc_info()[2])
        except Exception as e:
            print("Unexpected exception caught during testing: " + repr(e))
            traceback.print_tb(sys.exc_info()[2])
        except KeyboardInterrupt as e:
            print("Exiting after " + repr(e))

        if not self.options.noshutdown:
            print("Stopping nodes")
            stop_nodes(self.nodes)
        else:
            print("Note: dashds were not stopped and may still be running")

        if not self.options.nocleanup and not self.options.noshutdown and success:
            print("Cleaning up")
            shutil.rmtree(self.options.tmpdir)
            if not os.listdir(self.options.root):
                os.rmdir(self.options.root)
        else:
            print("Not cleaning up dir %s" % self.options.tmpdir)
            if os.getenv("PYTHON_DEBUG", ""):
                # Dump the end of the debug logs, to aid in debugging rare
                # travis failures.
                import glob
                filenames = glob.glob(self.options.tmpdir + "/node*/regtest/debug.log")
                MAX_LINES_TO_PRINT = 1000
                for f in filenames:
                    print("From" , f, ":")
                    from collections import deque
                    print("".join(deque(open(f), MAX_LINES_TO_PRINT)))
        if success:
            print("Tests successful")
            sys.exit(0)
        else:
            print("Failed")
            sys.exit(1)


MASTERNODE_COLLATERAL = 1000


class MasternodeInfo:
    def __init__(self, key, blsKey, collateral_id, collateral_out):
        self.key = key
        self.blsKey = blsKey
        self.collateral_id = collateral_id
        self.collateral_out = collateral_out


class DashTestFramework(BitcoinTestFramework):
    def __init__(self, num_nodes, masterodes_count, extra_args):
        super().__init__()
        self.mn_count = masterodes_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        self.is_network_split = False
        # additional args
        self.extra_args = extra_args

    def create_simple_node(self):
        idx = len(self.nodes)
        args = ["-debug"] + self.extra_args
        self.nodes.append(start_node(idx, self.options.tmpdir,
                                     args))
        for i in range(0, idx):
            connect_nodes(self.nodes[i], idx)

    def get_mnconf_file(self):
        return os.path.join(self.options.tmpdir, "node0/regtest/masternode.conf")

    def prepare_masternodes(self):
        for idx in range(0, self.mn_count):
            key = self.nodes[0].masternode("genkey")
            blsKey = self.nodes[0].bls('generate')['secret']
            address = self.nodes[0].getnewaddress()
            txid = self.nodes[0].sendtoaddress(address, MASTERNODE_COLLATERAL)
            txrow = self.nodes[0].getrawtransaction(txid, True)
            collateral_vout = 0
            for vout_idx in range(0, len(txrow["vout"])):
                vout = txrow["vout"][vout_idx]
                if vout["value"] == MASTERNODE_COLLATERAL:
                    collateral_vout = vout_idx
            self.nodes[0].lockunspent(False,
                                      [{"txid": txid, "vout": collateral_vout}])
            self.mninfo.append(MasternodeInfo(key, blsKey, txid, collateral_vout))

    def write_mn_config(self):
        conf = self.get_mnconf_file()
        f = open(conf, 'a')
        for idx in range(0, self.mn_count):
            f.write("mn%d 127.0.0.1:%d %s %s %d\n" % (idx + 1, p2p_port(idx + 1),
                                                      self.mninfo[idx].key,
                                                      self.mninfo[idx].collateral_id,
                                                      self.mninfo[idx].collateral_out))
        f.close()

    def create_masternodes(self):
        for idx in range(0, self.mn_count):
            args = ['-debug=masternode', '-externalip=127.0.0.1', '-masternode=1',
                    '-masternodeprivkey=%s' % self.mninfo[idx].key,
                    '-masternodeblsprivkey=%s' % self.mninfo[idx].blsKey] + self.extra_args
            self.nodes.append(start_node(idx + 1, self.options.tmpdir, args))
            for i in range(0, idx + 1):
                connect_nodes(self.nodes[i], idx + 1)

    def setup_network(self):
        self.nodes = []
        # create faucet node for collateral and transactions
        args = ["-debug"] + self.extra_args
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        required_balance = MASTERNODE_COLLATERAL * self.mn_count + 1
        while self.nodes[0].getbalance() < required_balance:
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        # create masternodes
        self.prepare_masternodes()
        self.write_mn_config()
        stop_node(self.nodes[0], 0)
        args = ["-debug",
                "-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"] + \
               self.extra_args
        self.nodes[0] = start_node(0, self.options.tmpdir,
                                   args)
        self.create_masternodes()
        # create connected simple nodes
        for i in range(0, self.num_nodes - self.mn_count - 1):
            self.create_simple_node()
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        sync_masternodes(self.nodes)
        for i in range(1, self.mn_count + 1):
            res = self.nodes[0].masternode("start-alias", "mn%d" % i)
            assert (res["result"] == 'successful')
        sync_masternodes(self.nodes)
        mn_info = self.nodes[0].masternodelist("status")
        assert (len(mn_info) == self.mn_count)
        for status in mn_info.values():
            assert (status == 'ENABLED')

    def enforce_masternode_payments(self):
        self.nodes[0].spork('SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT', 0)

    def create_raw_trx(self, node_from, node_to, amount, min_inputs, max_inputs):
        assert (min_inputs <= max_inputs)
        # fill inputs
        inputs = []
        balances = node_from.listunspent()
        in_amount = 0.0
        last_amount = 0.0
        for tx in balances:
            if len(inputs) < min_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            elif in_amount > amount:
                break
            elif len(inputs) < max_inputs:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount += float(tx['amount'])
                inputs.append(input)
            else:
                input = {}
                input["txid"] = tx['txid']
                input['vout'] = tx['vout']
                in_amount -= last_amount
                in_amount += float(tx['amount'])
                inputs[-1] = input
            last_amount = float(tx['amount'])

        assert (len(inputs) > 0)
        assert (in_amount > amount)
        # fill outputs
        receiver_address = node_to.getnewaddress()
        change_address = node_from.getnewaddress()
        fee = 0.001
        outputs = {}
        outputs[receiver_address] = satoshi_round(amount)
        outputs[change_address] = satoshi_round(in_amount - amount - fee)
        rawtx = node_from.createrawtransaction(inputs, outputs)
        return node_from.signrawtransaction(rawtx)

    def wait_for_instantlock(self, txid, node):
        # wait for instantsend locks
        start = time()
        locked = False
        while True:
            is_trx = node.gettransaction(txid)
            if is_trx['instantlock']:
                locked = True
                break
            if time() > start + 10:
                break
            sleep(0.1)
        return locked


# Test framework for doing p2p comparison testing, which sets up some bitcoind
# binaries:
# 1 binary: test binary
# 2 binaries: 1 test binary, 1 ref binary
# n>2 binaries: 1 test binary, n-1 ref binaries

class ComparisonTestFramework(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("DASHD", "dashd"),
                          help="bitcoind binary to test")
        parser.add_option("--refbinary", dest="refbinary",
                          default=os.getenv("DASHD", "dashd"),
                          help="bitcoind binary to use for reference nodes (if any)")

    def setup_network(self):
        self.nodes = start_nodes(
            self.num_nodes, self.options.tmpdir,
            extra_args=[['-debug', '-whitelist=127.0.0.1']] * self.num_nodes,
            binary=[self.options.testbinary] +
            [self.options.refbinary]*(self.num_nodes-1))
