#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base class for RPC testing."""

import logging
import optparse
import os
import sys
import shutil
import tempfile
import traceback
from concurrent.futures import ThreadPoolExecutor
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
    satoshi_round,
    wait_to_sync,
    copy_datadir)
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
    def __init__(self, proTxHash, ownerAddr, votingAddr, pubKeyOperator, keyOperator, collateral_address, collateral_txid, collateral_vout):
        self.proTxHash = proTxHash
        self.ownerAddr = ownerAddr
        self.votingAddr = votingAddr
        self.pubKeyOperator = pubKeyOperator
        self.keyOperator = keyOperator
        self.collateral_address = collateral_address
        self.collateral_txid = collateral_txid
        self.collateral_vout = collateral_vout


class DashTestFramework(BitcoinTestFramework):
    def __init__(self, num_nodes, masterodes_count, extra_args, fast_dip3_enforcement=False):
        super().__init__()
        self.mn_count = masterodes_count
        self.num_nodes = num_nodes
        self.mninfo = []
        self.setup_clean_chain = True
        self.is_network_split = False
        # additional args
        self.extra_args = extra_args

        self.extra_args += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]

        self.fast_dip3_enforcement = fast_dip3_enforcement
        if fast_dip3_enforcement:
            self.extra_args += ["-bip9params=dip0003:0:999999999999:10:5", "-dip3enforcementheight=50"]

    def create_simple_node(self):
        idx = len(self.nodes)
        args = self.extra_args
        self.nodes.append(start_node(idx, self.options.tmpdir, args))
        for i in range(0, idx):
            connect_nodes(self.nodes[i], idx)

    def prepare_masternodes(self):
        for idx in range(0, self.mn_count):
            bls = self.nodes[0].bls('generate')
            address = self.nodes[0].getnewaddress()
            txid = self.nodes[0].sendtoaddress(address, MASTERNODE_COLLATERAL)

            txraw = self.nodes[0].getrawtransaction(txid, True)
            collateral_vout = 0
            for vout_idx in range(0, len(txraw["vout"])):
                vout = txraw["vout"][vout_idx]
                if vout["value"] == MASTERNODE_COLLATERAL:
                    collateral_vout = vout_idx
            self.nodes[0].lockunspent(False, [{'txid': txid, 'vout': collateral_vout}])

            # send to same address to reserve some funds for fees
            self.nodes[0].sendtoaddress(address, 0.001)

            ownerAddr = self.nodes[0].getnewaddress()
            votingAddr = self.nodes[0].getnewaddress()
            rewardsAddr = self.nodes[0].getnewaddress()

            port = p2p_port(len(self.nodes) + idx)
            if (idx % 2) == 0:
                self.nodes[0].lockunspent(True, [{'txid': txid, 'vout': collateral_vout}])
                proTxHash = self.nodes[0].protx('register_fund', address, '127.0.0.1:%d' % port, ownerAddr, bls['public'], votingAddr, 0, rewardsAddr, address)
            else:
                self.nodes[0].generate(1)
                proTxHash = self.nodes[0].protx('register', txid, collateral_vout, '127.0.0.1:%d' % port, ownerAddr, bls['public'], votingAddr, 0, rewardsAddr, address)
            self.nodes[0].generate(1)

            self.mninfo.append(MasternodeInfo(proTxHash, ownerAddr, votingAddr, bls['public'], bls['secret'], address, txid, collateral_vout))
        self.sync_all()

    def prepare_datadirs(self):
        # stop faucet node so that we can copy the datadir
        stop_node(self.nodes[0], 0)

        start_idx = len(self.nodes)
        for idx in range(0, self.mn_count):
            copy_datadir(0, idx + start_idx, self.options.tmpdir)

        # restart faucet node
        self.nodes[0] = start_node(0, self.options.tmpdir, self.extra_args)

    def start_masternodes(self):
        start_idx = len(self.nodes)

        for idx in range(0, self.mn_count):
            self.nodes.append(None)
        executor = ThreadPoolExecutor(max_workers=20)

        def do_start(idx):
            args = ['-masternode=1',
                    '-masternodeblsprivkey=%s' % self.mninfo[idx].keyOperator] + self.extra_args
            node = start_node(idx + start_idx, self.options.tmpdir, args)
            self.mninfo[idx].node = node
            self.nodes[idx + start_idx] = node
            wait_to_sync(node, True)

        def do_connect(idx):
            for i in range(0, idx + 1):
                connect_nodes(self.nodes[idx + start_idx], i)

        jobs = []

        # start up nodes in parallel
        for idx in range(0, self.mn_count):
            jobs.append(executor.submit(do_start, idx))

        # wait for all nodes to start up
        for job in jobs:
            job.result()
        jobs.clear()

        # connect nodes in parallel
        for idx in range(0, self.mn_count):
            jobs.append(executor.submit(do_connect, idx))

        # wait for all nodes to connect
        for job in jobs:
            job.result()
        jobs.clear()

        sync_masternodes(self.nodes, True)

        executor.shutdown()

    def setup_network(self):
        self.nodes = []
        # create faucet node for collateral and transactions
        self.nodes.append(start_node(0, self.options.tmpdir, self.extra_args))
        required_balance = MASTERNODE_COLLATERAL * self.mn_count + 1
        while self.nodes[0].getbalance() < required_balance:
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)
        # create connected simple nodes
        for i in range(0, self.num_nodes - self.mn_count - 1):
            self.create_simple_node()
        sync_masternodes(self.nodes, True)

        # activate DIP3
        if not self.fast_dip3_enforcement:
            while self.nodes[0].getblockcount() < 500:
                self.nodes[0].generate(10)
        self.sync_all()

        # create masternodes
        self.prepare_masternodes()
        self.prepare_datadirs()
        self.start_masternodes()

        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(1)
        # sync nodes
        self.sync_all()
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())

        mn_info = self.nodes[0].masternodelist("status")
        assert (len(mn_info) == self.mn_count)
        for status in mn_info.values():
            assert (status == 'ENABLED')

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

    def wait_for_sporks_same(self, timeout=30):
        st = time()
        while time() < st + timeout:
            if self.check_sporks_same():
                return
            sleep(0.5)
        raise AssertionError("wait_for_sporks_same timed out")

    def check_sporks_same(self):
        sporks = self.nodes[0].spork('show')
        for node in self.nodes[1:]:
            sporks2 = node.spork('show')
            if sporks != sporks2:
                return False
        return True

    def wait_for_quorum_phase(self, phase, check_received_messages, check_received_messages_count, timeout=15):
        t = time()
        while time() - t < timeout:
            all_ok = True
            for mn in self.mninfo:
                s = mn.node.quorum("dkgstatus")["session"]
                if "llmq_10" not in s:
                    all_ok = False
                    break
                s = s["llmq_10"]
                if "phase" not in s:
                    all_ok = False
                    break
                if s["phase"] != phase:
                    all_ok = False
                    break
                if check_received_messages is not None:
                    if s[check_received_messages] < check_received_messages_count:
                        all_ok = False
                        break
            if all_ok:
                return
            sleep(0.1)
        raise AssertionError("wait_for_quorum_phase timed out")

    def wait_for_quorum_commitment(self, timeout = 15):
        t = time()
        while time() - t < timeout:
            all_ok = True
            for node in self.nodes:
                s = node.quorum("dkgstatus")
                if "minableCommitments" not in s:
                    all_ok = False
                    break
                s = s["minableCommitments"]
                if "llmq_10" not in s:
                    all_ok = False
                    break
            if all_ok:
                return
            sleep(0.1)
        raise AssertionError("wait_for_quorum_commitment timed out")

    def mine_quorum(self, expected_valid_count=10):
        quorums = self.nodes[0].quorum("list")

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(skip_count)

        # Make sure all reached phase 1 (init)
        self.wait_for_quorum_phase(1, None, 0)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)

        # Make sure all reached phase 2 (contribute) and received all contributions
        self.wait_for_quorum_phase(2, "receivedContributions", expected_valid_count)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)

        # Make sure all reached phase 3 (complain) and received all complaints
        self.wait_for_quorum_phase(3, "receivedComplaints" if expected_valid_count != 10 else None, expected_valid_count)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)

        # Make sure all reached phase 4 (justify)
        self.wait_for_quorum_phase(4, None, 0)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)

        # Make sure all reached phase 5 (commit)
        self.wait_for_quorum_phase(5, "receivedPrematureCommitments", expected_valid_count)
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(2)

        # Make sure all reached phase 6 (mining)
        self.wait_for_quorum_phase(6, None, 0)

        # Wait for final commitment
        self.wait_for_quorum_commitment()

        # mine the final commitment
        set_mocktime(get_mocktime() + 1)
        set_node_times(self.nodes, get_mocktime())
        self.nodes[0].generate(1)
        while quorums == self.nodes[0].quorum("list"):
            sleep(2)
            set_mocktime(get_mocktime() + 1)
            set_node_times(self.nodes, get_mocktime())
            self.nodes[0].generate(1)

        sync_blocks(self.nodes)

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
                          default=os.getenv("BITCOIND", "dashd"),
                          help="dashd binary to test")
        parser.add_option("--refbinary", dest="refbinary",
                          default=os.getenv("BITCOIND", "dashd"),
                          help="dashd binary to use for reference nodes (if any)")

    def setup_network(self):
        self.nodes = start_nodes(
            self.num_nodes, self.options.tmpdir,
            extra_args=[['-debug', '-whitelist=127.0.0.1']] * self.num_nodes,
            binary=[self.options.testbinary] +
            [self.options.refbinary]*(self.num_nodes-1))
