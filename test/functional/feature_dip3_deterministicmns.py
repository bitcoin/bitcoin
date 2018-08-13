#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test deterministic masternodes
#

from decimal import Decimal

from test_framework.blocktools import create_block, create_coinbase, get_masternode_payment
from test_framework.messages import CCbTx, COIN, CTransaction, FromHex, ToHex, uint256_to_string
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes, force_finish_mnsync, get_bip9_status, p2p_port

class Masternode(object):
    pass

class DIP3Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_initial_mn = 11 # Should be >= 11 to make sure quorums are not always the same MNs
        self.num_nodes = 1 + self.num_initial_mn + 2 # +1 for controller, +1 for mn-qt, +1 for mn created after dip3 activation
        self.setup_clean_chain = True

        self.extra_args = ["-budgetparams=10:10:10"]
        self.extra_args += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]
        self.extra_args += ["-dip3params=135:150"]


    def setup_network(self):
        self.disable_mocktime()
        self.add_nodes(1)
        self.start_controller_node()

    def start_controller_node(self):
        self.log.info("starting controller node")
        self.start_node(0, extra_args=self.extra_args)
        for i in range(1, self.num_nodes):
            if i < len(self.nodes) and self.nodes[i] is not None and self.nodes[i].process is not None:
                connect_nodes(self.nodes[i], 0)

    def stop_controller_node(self):
        self.log.info("stopping controller node")
        self.stop_node(0)

    def restart_controller_node(self):
        self.stop_controller_node()
        self.start_controller_node()

    def run_test(self):
        self.log.info("funding controller node")
        while self.nodes[0].getbalance() < (self.num_initial_mn + 3) * 1000:
            self.nodes[0].generate(10) # generate enough for collaterals
        self.log.info("controller node has {} dash".format(self.nodes[0].getbalance()))

        # Make sure we're below block 135 (which activates dip3)
        self.log.info("testing rejection of ProTx before dip3 activation")
        assert(self.nodes[0].getblockchaininfo()['blocks'] < 135)

        mns = []

        # prepare mn which should still be accepted later when dip3 activates
        self.log.info("creating collateral for mn-before-dip3")
        before_dip3_mn = self.prepare_mn(self.nodes[0], 1, 'mn-before-dip3')
        self.create_mn_collateral(self.nodes[0], before_dip3_mn)
        mns.append(before_dip3_mn)

        # block 150 starts enforcing DIP3 MN payments
        self.nodes[0].generate(150 - self.nodes[0].getblockcount())
        assert(self.nodes[0].getblockcount() == 150)

        self.log.info("mining final block for DIP3 activation")
        self.nodes[0].generate(1)

        # We have hundreds of blocks to sync here, give it more time
        self.log.info("syncing blocks for all nodes")
        self.sync_blocks(self.nodes, timeout=120)

        # DIP3 is fully enforced here

        self.register_mn(self.nodes[0], before_dip3_mn)
        self.start_mn(before_dip3_mn)

        self.log.info("registering MNs")
        for i in range(0, self.num_initial_mn):
            mn = self.prepare_mn(self.nodes[0], i + 2, "mn-%d" % i)
            mns.append(mn)

            # start a few MNs before they are registered and a few after they are registered
            start = (i % 3) == 0
            if start:
                self.start_mn(mn)

            # let a few of the protx MNs refer to the existing collaterals
            fund = (i % 2) == 0
            if fund:
                self.log.info("register_fund %s" % mn.alias)
                self.register_fund_mn(self.nodes[0], mn)
            else:
                self.log.info("create_collateral %s" % mn.alias)
                self.create_mn_collateral(self.nodes[0], mn)
                self.log.info("register %s" % mn.alias)
                self.register_mn(self.nodes[0], mn)

            self.nodes[0].generate(1)

            if not start:
                self.start_mn(mn)

            self.sync_all()
            self.assert_mnlists(mns)

        self.log.info("test that MNs disappear from the list when the ProTx collateral is spent")
        spend_mns_count = 3
        mns_tmp = [] + mns
        dummy_txins = []
        for i in range(spend_mns_count):
            dummy_txin = self.spend_mn_collateral(mns[i], with_dummy_input_output=True)
            dummy_txins.append(dummy_txin)
            self.nodes[0].generate(1)
            self.sync_all()
            mns_tmp.remove(mns[i])
            self.assert_mnlists(mns_tmp)

        self.log.info("test that reverting the blockchain on a single node results in the mnlist to be reverted as well")
        for i in range(spend_mns_count):
            self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
            mns_tmp.append(mns[spend_mns_count - 1 - i])
            self.assert_mnlist(self.nodes[0], mns_tmp)

        self.log.info("cause a reorg with a double spend and check that mnlists are still correct on all nodes")
        self.mine_double_spend(self.nodes[0], dummy_txins, self.nodes[0].getnewaddress(), use_mnmerkleroot_from_tip=True)
        self.nodes[0].generate(spend_mns_count)
        self.sync_all()
        self.assert_mnlists(mns_tmp)

        self.log.info("test mn payment enforcement with deterministic MNs")
        for i in range(20):
            node = self.nodes[i % len(self.nodes)]
            self.test_invalid_mn_payment(node)
            self.nodes[0].generate(1)
            self.sync_all()

        self.log.info("testing ProUpServTx")
        for mn in mns:
            self.test_protx_update_service(mn)

        self.log.info("testing P2SH/multisig for payee addresses")

        # Create 1 of 2 multisig
        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()

        addr1Obj = self.nodes[0].getaddressinfo(addr1)
        addr2Obj = self.nodes[0].getaddressinfo(addr2)

        multisig = self.nodes[0].createmultisig(1, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']
        self.update_mn_payee(mns[0], multisig)
        found_multisig_payee = False
        for i in range(len(mns)):
            bt = self.nodes[0].getblocktemplate()
            expected_payee = bt['masternode'][0]['payee']
            expected_amount = bt['masternode'][0]['amount']
            self.nodes[0].generate(1)
            self.sync_all()
            if expected_payee == multisig:
                block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
                cbtx = self.nodes[0].getrawtransaction(block['tx'][0], 1)
                for out in cbtx['vout']:
                    if 'addresses' in out['scriptPubKey']:
                        if expected_payee in out['scriptPubKey']['addresses'] and out['valueSat'] == expected_amount:
                            found_multisig_payee = True
        assert(found_multisig_payee)

        self.log.info("testing reusing of collaterals for replaced MNs")
        for i in range(0, 5):
            mn = mns[i]
            # a few of these will actually refer to old ProRegTx internal collaterals,
            # which should work the same as external collaterals
            new_mn = self.prepare_mn(self.nodes[0], mn.idx, mn.alias)
            new_mn.collateral_address = mn.collateral_address
            new_mn.collateral_txid = mn.collateral_txid
            new_mn.collateral_vout = mn.collateral_vout

            self.register_mn(self.nodes[0], new_mn)
            mns[i] = new_mn
            self.nodes[0].generate(1)
            self.sync_all()
            self.assert_mnlists(mns)
            self.log.info("restarting MN %s" % new_mn.alias)
            self.stop_node(new_mn.idx)
            self.start_mn(new_mn)
            self.sync_all()

        self.log.info("testing masternode status updates")
        # change voting address and see if changes are reflected in `masternode status` rpc output
        mn = mns[0]
        node = self.nodes[0]
        old_dmnState = mn.node.masternode("status")["dmnState"]
        old_voting_address = old_dmnState["votingAddress"]
        new_voting_address = node.getnewaddress()
        assert(old_voting_address != new_voting_address)
        # also check if funds from payout address are used when no fee source address is specified
        node.sendtoaddress(mn.rewards_address, 0.001)
        node.protx('update_registrar', mn.protx_hash, "", new_voting_address, "")
        node.generate(1)
        self.sync_all()
        new_dmnState = mn.node.masternode("status")["dmnState"]
        new_voting_address_from_rpc = new_dmnState["votingAddress"]
        assert(new_voting_address_from_rpc == new_voting_address)
        # make sure payoutAddress is the same as before
        assert(old_dmnState["payoutAddress"] == new_dmnState["payoutAddress"])

    def prepare_mn(self, node, idx, alias):
        mn = Masternode()
        mn.idx = idx
        mn.alias = alias
        mn.is_protx = True
        mn.p2p_port = p2p_port(mn.idx)

        blsKey = node.bls('generate')
        mn.fundsAddr = node.getnewaddress()
        mn.ownerAddr = node.getnewaddress()
        mn.operatorAddr = blsKey['public']
        mn.votingAddr = mn.ownerAddr
        mn.blsMnkey = blsKey['secret']

        return mn

    def create_mn_collateral(self, node, mn):
        mn.collateral_address = node.getnewaddress()
        mn.collateral_txid = node.sendtoaddress(mn.collateral_address, 1000)
        mn.collateral_vout = -1
        node.generate(1)

        rawtx = node.getrawtransaction(mn.collateral_txid, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                mn.collateral_vout = txout['n']
                break
        assert(mn.collateral_vout != -1)

    # register a protx MN and also fund it (using collateral inside ProRegTx)
    def register_fund_mn(self, node, mn):
        node.sendtoaddress(mn.fundsAddr, 1000.001)
        mn.collateral_address = node.getnewaddress()
        mn.rewards_address = node.getnewaddress()

        mn.protx_hash = node.protx('register_fund', mn.collateral_address, '127.0.0.1:%d' % mn.p2p_port, mn.ownerAddr, mn.operatorAddr, mn.votingAddr, 0, mn.rewards_address, mn.fundsAddr)
        mn.collateral_txid = mn.protx_hash
        mn.collateral_vout = -1

        rawtx = node.getrawtransaction(mn.collateral_txid, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                mn.collateral_vout = txout['n']
                break
        assert(mn.collateral_vout != -1)

    # create a protx MN which refers to an existing collateral
    def register_mn(self, node, mn):
        node.sendtoaddress(mn.fundsAddr, 0.001)
        mn.rewards_address = node.getnewaddress()

        mn.protx_hash = node.protx('register', mn.collateral_txid, mn.collateral_vout, '127.0.0.1:%d' % mn.p2p_port, mn.ownerAddr, mn.operatorAddr, mn.votingAddr, 0, mn.rewards_address, mn.fundsAddr)
        node.generate(1)

    def start_mn(self, mn):
        while len(self.nodes) <= mn.idx:
            self.add_nodes(1)
        extra_args = ['-masternodeblsprivkey=%s' % mn.blsMnkey]
        self.start_node(mn.idx, extra_args = self.extra_args + extra_args)
        force_finish_mnsync(self.nodes[mn.idx])
        mn.node = self.nodes[mn.idx]
        connect_nodes(mn.node, 0)
        self.sync_all()

    def spend_mn_collateral(self, mn, with_dummy_input_output=False):
        return self.spend_input(mn.collateral_txid, mn.collateral_vout, 1000, with_dummy_input_output)

    def update_mn_payee(self, mn, payee):
        self.nodes[0].sendtoaddress(mn.fundsAddr, 0.001)
        self.nodes[0].protx('update_registrar', mn.protx_hash, '', '', payee, mn.fundsAddr)
        self.nodes[0].generate(1)
        self.sync_all()
        info = self.nodes[0].protx('info', mn.protx_hash)
        assert(info['state']['payoutAddress'] == payee)

    def test_protx_update_service(self, mn):
        self.nodes[0].sendtoaddress(mn.fundsAddr, 0.001)
        self.nodes[0].protx('update_service', mn.protx_hash, '127.0.0.2:%d' % mn.p2p_port, mn.blsMnkey, "", mn.fundsAddr)
        self.nodes[0].generate(1)
        self.sync_all()
        for node in self.nodes:
            protx_info = node.protx('info', mn.protx_hash)
            mn_list = node.masternode('list')
            assert_equal(protx_info['state']['service'], '127.0.0.2:%d' % mn.p2p_port)
            assert_equal(mn_list['%s-%d' % (mn.collateral_txid, mn.collateral_vout)]['address'], '127.0.0.2:%d' % mn.p2p_port)

        # undo
        self.nodes[0].protx('update_service', mn.protx_hash, '127.0.0.1:%d' % mn.p2p_port, mn.blsMnkey, "", mn.fundsAddr)
        self.nodes[0].generate(1)

    def assert_mnlists(self, mns):
        for node in self.nodes:
            self.assert_mnlist(node, mns)

    def assert_mnlist(self, node, mns):
        if not self.compare_mnlist(node, mns):
            expected = []
            for mn in mns:
                expected.append('%s-%d' % (mn.collateral_txid, mn.collateral_vout))
            self.log.error('mnlist: ' + str(node.masternode('list', 'status')))
            self.log.error('expected: ' + str(expected))
            raise AssertionError("mnlists does not match provided mns")

    def compare_mnlist(self, node, mns):
        mnlist = node.masternode('list', 'status')
        for mn in mns:
            s = '%s-%d' % (mn.collateral_txid, mn.collateral_vout)
            in_list = s in mnlist
            if not in_list:
                return False
            mnlist.pop(s, None)
        if len(mnlist) != 0:
            return False
        return True

    def spend_input(self, txid, vout, amount, with_dummy_input_output=False):
        # with_dummy_input_output is useful if you want to test reorgs with double spends of the TX without touching the actual txid/vout
        address = self.nodes[0].getnewaddress()

        txins = [
            {'txid': txid, 'vout': vout}
        ]
        targets = {address: amount}

        dummy_txin = None
        if with_dummy_input_output:
            dummyaddress = self.nodes[0].getnewaddress()
            unspent = self.nodes[0].listunspent(110)
            for u in unspent:
                if u['amount'] > Decimal(1):
                    dummy_txin = {'txid': u['txid'], 'vout': u['vout']}
                    txins.append(dummy_txin)
                    targets[dummyaddress] = float(u['amount'] - Decimal(0.0001))
                    break

        rawtx = self.nodes[0].createrawtransaction(txins, targets)
        rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
        rawtx = self.nodes[0].signrawtransactionwithwallet(rawtx)['hex']
        self.nodes[0].sendrawtransaction(rawtx)

        return dummy_txin

    def mine_block(self, node, vtx=[], miner_address=None, mn_payee=None, mn_amount=None, use_mnmerkleroot_from_tip=False, expected_error=None):
        bt = node.getblocktemplate()
        height = bt['height']
        tip_hash = bt['previousblockhash']

        tip_block = node.getblock(tip_hash)

        coinbasevalue = bt['coinbasevalue']
        if miner_address is None:
            miner_address = self.nodes[0].getnewaddress()
        if mn_payee is None:
            if isinstance(bt['masternode'], list):
                mn_payee = bt['masternode'][0]['payee']
            else:
                mn_payee = bt['masternode']['payee']
        # we can't take the masternode payee amount from the template here as we might have additional fees in vtx

        # calculate fees that the block template included (we'll have to remove it from the coinbase as we won't
        # include the template's transactions
        bt_fees = 0
        for tx in bt['transactions']:
            bt_fees += tx['fee']

        new_fees = 0
        for tx in vtx:
            in_value = 0
            out_value = 0
            for txin in tx.vin:
                txout = node.gettxout(uint256_to_string(txin.prevout.hash), txin.prevout.n, False)
                in_value += int(txout['value'] * COIN)
            for txout in tx.vout:
                out_value += txout.nValue
            new_fees += in_value - out_value

        # fix fees
        coinbasevalue -= bt_fees
        coinbasevalue += new_fees

        if mn_amount is None:
            realloc_info = get_bip9_status(self.nodes[0], 'realloc')
            realloc_height = 99999999
            if realloc_info['status'] == 'active':
                realloc_height = realloc_info['since']
            mn_amount = get_masternode_payment(height, coinbasevalue, realloc_height)
        miner_amount = coinbasevalue - mn_amount

        outputs = {miner_address: str(Decimal(miner_amount) / COIN)}
        if mn_amount > 0:
            outputs[mn_payee] = str(Decimal(mn_amount) / COIN)

        coinbase = FromHex(CTransaction(), node.createrawtransaction([], outputs))
        coinbase.vin = create_coinbase(height).vin

        # We can't really use this one as it would result in invalid merkle roots for masternode lists
        if len(bt['coinbase_payload']) != 0:
            cbtx = FromHex(CCbTx(version=1), bt['coinbase_payload'])
            if use_mnmerkleroot_from_tip:
                if 'cbTx' in tip_block:
                    cbtx.merkleRootMNList = int(tip_block['cbTx']['merkleRootMNList'], 16)
                else:
                    cbtx.merkleRootMNList = 0
            coinbase.nVersion = 3
            coinbase.nType = 5 # CbTx
            coinbase.vExtraPayload = cbtx.serialize()

        coinbase.calc_sha256()

        block = create_block(int(tip_hash, 16), coinbase)
        block.vtx += vtx

        # Add quorum commitments from template
        for tx in bt['transactions']:
            tx2 = FromHex(CTransaction(), tx['data'])
            if tx2.nType == 6:
                block.vtx.append(tx2)

        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        result = node.submitblock(ToHex(block))
        if expected_error is not None and result != expected_error:
            raise AssertionError('mining the block should have failed with error %s, but submitblock returned %s' % (expected_error, result))
        elif expected_error is None and result is not None:
            raise AssertionError('submitblock returned %s' % (result))

    def mine_double_spend(self, node, txins, target_address, use_mnmerkleroot_from_tip=False):
        amount = Decimal(0)
        for txin in txins:
            txout = node.gettxout(txin['txid'], txin['vout'], False)
            amount += txout['value']
        amount -= Decimal("0.001") # fee

        rawtx = node.createrawtransaction(txins, {target_address: amount})
        rawtx = node.signrawtransactionwithwallet(rawtx)['hex']
        tx = FromHex(CTransaction(), rawtx)

        self.mine_block(node, [tx], use_mnmerkleroot_from_tip=use_mnmerkleroot_from_tip)

    def test_invalid_mn_payment(self, node):
        mn_payee = self.nodes[0].getnewaddress()
        self.mine_block(node, mn_payee=mn_payee, expected_error='bad-cb-payee')
        self.mine_block(node, mn_amount=1, expected_error='bad-cb-payee')

if __name__ == '__main__':
    DIP3Test().main()
