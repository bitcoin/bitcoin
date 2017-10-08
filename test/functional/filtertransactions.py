#!/usr/bin/env python3
# Copyright (c) 2017 The Particl Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_particl import ParticlTestFramework
from test_framework.test_particl import isclose
from test_framework.util import *
from test_framework.address import *

import json

class FilterTransactionsTest(ParticlTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [ [ '-debug' ] for i in range(self.num_nodes) ]

    def setup_network(self, split=False):
        self.enable_mocktime()
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)
        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        self.is_network_split = False
        self.sync_all()
    
    def run_test(self):
        nodes = self.nodes
        
        nodes[0].extkeyimportmaster('abandon baby cabbage dad eager fabric gadget habit ice kangaroo lab absorb')
        nodes[1].extkeyimportmaster('drip fog service village program equip minute dentist series hawk crop sphere olympic lazy garbage segment fox library good alley steak jazz force inmate')
        nodes[2].extkeyimportmaster('sección grito médula hecho pauta posada nueve ebrio bruto buceo baúl mitad')
        
        selfAddress    = nodes[0].getnewaddress()
        selfStealth    = nodes[0].getnewstealthaddress()
        targetAddress  = nodes[1].getnewaddress()
        targetStealth  = nodes[1].getnewstealthaddress()
        stakingAddress = nodes[2].getnewaddress()
        
        nodes[0].sendtoaddress(targetAddress, 10)
        
        nodes[0].sendparttoblind(
            selfStealth,          # address
            20,                   # amount
            '',                   # ?
            '',                   # ?
            False,                # substract fee
            'node0 -> node0 p->b' # narrative
        )
        
        nodes[0].sendparttoanon(
            selfStealth,          # address
            20,                   # amount
            '',                   # ?
            '',                   # ?
            False,                # substract fee
            'node0 -> node0 p->a' # narrative
        )
        
        self.sync_all()
        
        # without argument
        ro = nodes[0].filtertransactions()
        # assert(len(ro) == x)
        
        # too much arguments
        try:
            nodes[0].filtertransactions('foo', 'bar')
            assert(False)
        except JSONRPCException as e:
            assert('filtertransactions' in e.error['message'])
        
        print(json.dumps(ro, indent=4, default=self.jsonDecimal))
        # ro = nodes[1].filtertransactions()
        # print(json.dumps(ro, indent=4, default=self.jsonDecimal))
        
        #
        # count
        #
        
        # count: 0 => JSONRPCException
        try:
            nodes[0].filtertransactions({ 'count': 0 })
            assert(False)
        except JSONRPCException as e:
            assert('Invalid count' in e.error['message'])
        
        # count: -1 => JSONRPCException
        try:
            nodes[0].filtertransactions({ 'count': -1 })
            assert(False)
        except JSONRPCException as e:
            assert('Invalid count' in e.error['message'])
            
        # count: 1
        ro = nodes[0].filtertransactions({ 'count': 1 })
        assert(len(ro) == 1)
        
        #
        # skip
        #
        
        # skip: -1 => JSONRPCException
        try:
            nodes[0].filtertransactions({ 'skip': -1 })
            assert(False)
        except JSONRPCException as e:
            assert('Invalid skip' in e.error['message'])
        
        # skip = count => no entry
        ro = nodes[0].filtertransactions()
        ro = nodes[0].filtertransactions({ 'skip': len(ro) })
        assert(len(ro) == 0)
        
        # skip == count - 1 => one entry
        ro = nodes[0].filtertransactions()
        ro = nodes[0].filtertransactions({ 'skip': len(ro) - 1 })
        assert(len(ro) == 1)
        
        # TODO
        # skip: 1
        
        # include_watchonly
        # TODO
        # watchonly included
        # watchonly excluded
        
        # search
        # TODO
        # search amounts
        
        queries = [
            [targetAddress, 1],
            [selfStealth,   2],
            ['100',         2]
        ]
        
        for query in queries:
            ro = nodes[0].filtertransactions({ 'search': query[0] })
            # print(json.dumps(ro, indent=4, default=self.jsonDecimal))
            assert(len(ro) == query[1])
        
        # category
        # TODO
        # 'all' transactions
        # 'orphan' transactions
        # 'immature' transactions
        # 'orphaned_stake' transactions
        
        categories = [
            ['internal_transfer', 0],
            ['coinbase',          0],
            ['send',              0],
            ['receive',           0],
            ['stake',             0]
        ]
        
        for category in categories:
            ro = nodes[0].filtertransactions({ 'category': category[0] })
            for t in ro:
                assert(t['category'] == category[0])
            # TODO
            # assert(len(ro) == category[1])
        
        # invalid transaction category
        try:
            ro = nodes[0].filtertransactions({ 'category': 'invalid' })
            assert(False)
        except JSONRPCException as e:
            assert('Invalid category' in e.error['message'])
        
        # sort
        # TODO
        # sort by time
        # sort by address
        # sort by category
        # sort by amount
        # sort by confirmations
        # sort by txid
        # invalid sort
        try:
            ro = nodes[0].filtertransactions({ 'sort': 'invalid' })
            assert(False)
        except JSONRPCException as e:
            assert('Invalid sort' in e.error['message'])
        
        # ro = nodes[1].filtertransactions()
        # print(json.dumps(ro, indent=4, default=self.jsonDecimal))
    
if __name__ == '__main__':
    FilterTransactionsTest().main()
