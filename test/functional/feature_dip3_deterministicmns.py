#!/usr/bin/env python3
# Copyright (c) 2015-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test deterministic masternodes
#

from decimal import Decimal
from typing import List

from test_framework.blocktools import create_block_with_mnpayments
from test_framework.messages import tx_from_hex
from test_framework.test_framework import (
    BitcoinTestFramework,
    MasternodeInfo,
)
from test_framework.util import assert_equal, force_finish_mnsync, p2p_port, softfork_active

class DIP3Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_initial_mn = 11 # Should be >= 11 to make sure quorums are not always the same MNs
        self.num_nodes = 1 + self.num_initial_mn + 2 # +1 for controller, +1 for mn-qt, +1 for mn created after dip3 activation
        self.setup_clean_chain = True
        self.disable_mocktime = True
        self.supports_cli = False

        self.extra_args = ["-deprecatedrpc=addresses"]
        self.extra_args += ["-budgetparams=10:10:10"]
        self.extra_args += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]
        self.extra_args += ["-dip3params=135:150"]


    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(1)
        self.start_controller_node()
        self.import_deterministic_coinbase_privkeys()

    def start_controller_node(self):
        self.log.info("starting controller node")
        self.start_node(0, extra_args=self.extra_args)
        for node in self.nodes[1:]:
            if node is not None and node.process is not None:
                self.connect_nodes(node.index, 0)

    def run_test(self):
        self.log.info("funding controller node")
        while self.nodes[0].getbalance() < (self.num_initial_mn + 3) * 1000:
            self.generate(self.nodes[0], 10, sync_fun=self.no_op) # generate enough for collaterals
        self.log.info("controller node has {} dash".format(self.nodes[0].getbalance()))

        # Make sure we're below block 135 (which activates dip3)
        self.log.info("testing rejection of ProTx before dip3 activation")
        assert self.nodes[0].getblockchaininfo()['blocks'] < 135

        mns: List[MasternodeInfo] = []

        # prepare mn which should still be accepted later when dip3 activates
        self.log.info("creating collateral for mn-before-dip3")
        before_dip3_mn: MasternodeInfo = self.prepare_mn(self.nodes[0], 1, 'mn-before-dip3')
        self.create_mn_collateral(self.nodes[0], before_dip3_mn)
        mns.append(before_dip3_mn)

        # block 150 starts enforcing DIP3 MN payments
        self.generate(self.nodes[0], 150 - self.nodes[0].getblockcount(), sync_fun=self.no_op)
        assert self.nodes[0].getblockcount() == 150

        self.log.info("mining final block for DIP3 activation")
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # We have hundreds of blocks to sync here, give it more time
        self.log.info("syncing blocks for all nodes")
        self.sync_blocks(timeout=120)

        # DIP3 is fully enforced here

        self.register_mn(self.nodes[0], before_dip3_mn)
        self.start_mn(before_dip3_mn)

        self.log.info("registering MNs")
        for i in range(self.num_initial_mn):
            mn: MasternodeInfo = self.prepare_mn(self.nodes[0], i + 2, "mn-%d" % i)
            mns.append(mn)

            # start a few MNs before they are registered and a few after they are registered
            start = (i % 3) == 0
            if start:
                self.start_mn(mn)

            # let a few of the protx MNs refer to the existing collaterals
            fund = (i % 2) == 0
            if fund:
                self.log.info(f"register_fund {mn.friendlyName}")
                self.register_fund_mn(self.nodes[0], mn)
            else:
                self.log.info(f"create_collateral {mn.friendlyName}")
                self.create_mn_collateral(self.nodes[0], mn)
                self.log.info(f"register {mn.friendlyName}")
                self.register_mn(self.nodes[0], mn)

            self.generate(self.nodes[0], 1, sync_fun=self.no_op)

            if not start:
                self.start_mn(mn)

            self.sync_all()
            self.assert_mnlists(mns)

        self.log.info("test that MNs disappear from the list when the ProTx collateral is spent")
        # also make sure "protx info" returns correct historical data for spent collaterals
        spend_mns_count = 3
        mns_tmp = [] + mns
        dummy_txins = []
        old_tip = self.nodes[0].getblockcount()
        old_listdiff = self.nodes[0].protx("listdiff", 1, old_tip)
        for i in range(spend_mns_count):
            old_protx_hash = mns[i].proTxHash
            old_collateral_address = mns[i].collateral_address
            old_blockhash = self.nodes[0].getbestblockhash()
            old_rpc_info = self.nodes[0].protx("info", old_protx_hash)
            rpc_collateral_address = old_rpc_info["collateralAddress"]
            assert_equal(rpc_collateral_address, old_collateral_address)
            dummy_txin = self.spend_mn_collateral(mns[i], with_dummy_input_output=True)
            dummy_txins.append(dummy_txin)
            self.generate(self.nodes[0], 1)
            mns_tmp.remove(mns[i])
            self.assert_mnlists(mns_tmp)
            new_rpc_info = self.nodes[0].protx("info", old_protx_hash, old_blockhash)
            del old_rpc_info["metaInfo"]
            del new_rpc_info["metaInfo"]
            assert_equal(new_rpc_info, old_rpc_info)
        new_listdiff = self.nodes[0].protx("listdiff", 1, old_tip)
        assert_equal(new_listdiff, old_listdiff)

        self.log.info("test that reverting the blockchain on a single node results in the mnlist to be reverted as well")
        for i in range(spend_mns_count):
            self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
            mns_tmp.append(mns[spend_mns_count - 1 - i])
            self.assert_mnlist(self.nodes[0], mns_tmp)

        self.log.info("cause a reorg with a double spend and check that mnlists are still correct on all nodes")
        self.mine_double_spend(mns, self.nodes[0], dummy_txins, self.nodes[0].getnewaddress())
        self.generate(self.nodes[0], spend_mns_count)
        self.assert_mnlists(mns_tmp)

        self.log.info("test mn payment enforcement with deterministic MNs")
        for i in range(20):
            node = self.nodes[i % len(self.nodes)]
            self.test_invalid_mn_payment(mns, node)
            self.generate(self.nodes[0], 1)

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
        for _ in range(len(mns)):
            bt = self.nodes[0].getblocktemplate()
            expected_payee = bt['masternode'][0]['payee']
            expected_amount = bt['masternode'][0]['amount']
            self.generate(self.nodes[0], 1)
            if expected_payee == multisig:
                block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
                cbtx = self.nodes[0].getrawtransaction(block['tx'][0], 1)
                for out in cbtx['vout']:
                    if 'address' in out['scriptPubKey']:
                        if expected_payee in out['scriptPubKey']['address'] and out['valueSat'] == expected_amount:
                            found_multisig_payee = True
        assert found_multisig_payee

        self.log.info("testing reusing of collaterals for replaced MNs")
        for i in range(5):
            mn = mns[i]
            # a few of these will actually refer to old ProRegTx internal collaterals,
            # which should work the same as external collaterals
            new_mn: MasternodeInfo = self.prepare_mn(self.nodes[0], mn.nodeIdx, mn.friendlyName)
            new_mn.collateral_address = mn.collateral_address
            new_mn.collateral_txid = mn.collateral_txid
            new_mn.collateral_vout = mn.collateral_vout

            self.register_mn(self.nodes[0], new_mn)
            mns[i] = new_mn
            self.generate(self.nodes[0], 1)
            self.assert_mnlists(mns)
            self.log.info(f"restarting MN {mn.friendlyName}")
            self.stop_node(new_mn.nodeIdx)
            self.start_mn(new_mn)
            self.sync_all()

        self.log.info("testing masternode status updates")
        # change voting address and see if changes are reflected in `masternode status` rpc output
        mn = mns[0]
        node = self.nodes[0]
        old_dmnState = mn.get_node(self).masternode("status")["dmnState"]
        old_voting_address = old_dmnState["votingAddress"]
        new_voting_address = node.getnewaddress()
        assert old_voting_address != new_voting_address
        # also check if funds from payout address are used when no fee source address is specified
        node.sendtoaddress(mn.rewards_address, 0.001)
        node.protx('update_registrar' if softfork_active(node, 'v19') else 'update_registrar_legacy', mn.proTxHash, "", new_voting_address, "")
        self.generate(node, 1)
        new_dmnState = mn.get_node(self).masternode("status")["dmnState"]
        new_voting_address_from_rpc = new_dmnState["votingAddress"]
        assert new_voting_address_from_rpc == new_voting_address
        # make sure payoutAddress is the same as before
        assert old_dmnState["payoutAddress"] == new_dmnState["payoutAddress"]

    def prepare_mn(self, node, idx, alias) -> MasternodeInfo:
        mn = MasternodeInfo(evo=False, legacy=(not softfork_active(node, 'v19')))
        mn.generate_addresses(node)
        mn.set_params(operator_reward=(idx % self.num_initial_mn), nodePort=p2p_port(idx))
        mn.set_node(idx, alias)
        return mn

    def create_mn_collateral(self, node, mn: MasternodeInfo):
        txid = node.sendtoaddress(mn.collateral_address, 1000)
        vout = None

        self.generate(node, 1, sync_fun=self.no_op)
        rawtx = node.getrawtransaction(txid, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                vout = txout['n']
                break
        assert vout is not None

        mn.set_params(collateral_txid=txid, collateral_vout=vout)

    # register a protx MN and also fund it (using collateral inside ProRegTx)
    def register_fund_mn(self, node, mn: MasternodeInfo):
        node.sendtoaddress(mn.fundsAddr, 1000.001)

        proTxHash = node.protx('register_fund' if softfork_active(node, 'v19') else 'register_fund_legacy', mn.collateral_address, '127.0.0.1:%d' % mn.nodePort, mn.ownerAddr, mn.pubKeyOperator, mn.votingAddr, mn.operator_reward, mn.rewards_address, mn.fundsAddr)
        vout = None

        rawtx = node.getrawtransaction(proTxHash, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                vout = txout['n']
                break
        assert vout is not None

        mn.set_params(proTxHash=proTxHash, collateral_txid=proTxHash, collateral_vout=vout)

    # create a protx MN which refers to an existing collateral
    def register_mn(self, node, mn: MasternodeInfo):
        node.sendtoaddress(mn.fundsAddr, 0.001)
        proTxHash = node.protx('register' if softfork_active(node, 'v19') else 'register_legacy', mn.collateral_txid, mn.collateral_vout, '127.0.0.1:%d' % mn.nodePort, mn.ownerAddr, mn.pubKeyOperator, mn.votingAddr, mn.operator_reward, mn.rewards_address, mn.fundsAddr)
        mn.set_params(proTxHash=proTxHash)
        self.generate(node, 1, sync_fun=self.no_op)

    def start_mn(self, mn: MasternodeInfo):
        assert mn.nodeIdx is not None, "nodeIdx must be set before starting masternode"
        if len(self.nodes) <= mn.nodeIdx:
            self.add_nodes(mn.nodeIdx - len(self.nodes) + 1)
            assert len(self.nodes) == mn.nodeIdx + 1
        self.start_node(mn.nodeIdx, extra_args = self.extra_args + ['-masternodeblsprivkey=%s' % mn.keyOperator])
        force_finish_mnsync(mn.get_node(self))
        self.connect_nodes(mn.nodeIdx, 0)
        self.sync_all()

    def spend_mn_collateral(self, mn: MasternodeInfo, with_dummy_input_output=False):
        return self.spend_input(mn.collateral_txid, mn.collateral_vout, 1000, with_dummy_input_output)

    def update_mn_payee(self, mn: MasternodeInfo, payee):
        self.nodes[0].sendtoaddress(mn.fundsAddr, 0.001)
        self.nodes[0].protx('update_registrar' if softfork_active(self.nodes[0], 'v19') else 'update_registrar_legacy', mn.proTxHash, '', '', payee, mn.fundsAddr)
        self.generate(self.nodes[0], 1)
        info = self.nodes[0].protx('info', mn.proTxHash)
        assert info['state']['payoutAddress'] == payee

    def test_protx_update_service(self, mn: MasternodeInfo):
        self.nodes[0].sendtoaddress(mn.fundsAddr, 0.001)
        self.nodes[0].protx('update_service', mn.proTxHash, '127.0.0.2:%d' % mn.nodePort, mn.keyOperator, "", mn.fundsAddr)
        self.generate(self.nodes[0], 1)
        for node in self.nodes:
            protx_info = node.protx('info', mn.proTxHash)
            mn_list = node.masternode('list')
            assert_equal(protx_info['state']['service'], '127.0.0.2:%d' % mn.nodePort)
            assert_equal(mn_list['%s-%d' % (mn.collateral_txid, mn.collateral_vout)]['address'], '127.0.0.2:%d' % mn.nodePort)

        # undo
        self.nodes[0].protx('update_service', mn.proTxHash, '127.0.0.1:%d' % mn.nodePort, mn.keyOperator, "", mn.fundsAddr)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

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
            if s not in mnlist:
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

    def mine_block(self, mns, node, vtx=None, mn_payee=None, mn_amount=None, expected_error=None):
        block = create_block_with_mnpayments(mns, node, vtx, mn_payee, mn_amount)
        result = node.submitblock(block.serialize().hex())
        if expected_error is not None and result != expected_error:
            raise AssertionError('mining the block should have failed with error %s, but submitblock returned %s' % (expected_error, result))
        elif expected_error is None and result is not None:
            raise AssertionError('submitblock returned %s' % (result))

    def mine_double_spend(self, mns, node, txins, target_address):
        amount = Decimal(0)
        for txin in txins:
            txout = node.gettxout(txin['txid'], txin['vout'], False)
            amount += txout['value']
        amount -= Decimal("0.001") # fee

        rawtx = node.createrawtransaction(txins, {target_address: amount})
        rawtx = node.signrawtransactionwithwallet(rawtx)['hex']
        tx = tx_from_hex(rawtx)

        self.mine_block(mns, node, [tx])

    def test_invalid_mn_payment(self, mns, node):
        mn_payee = self.nodes[0].getnewaddress()
        self.mine_block(mns, node, mn_payee=mn_payee, expected_error='bad-cb-payee')
        self.mine_block(mns, node, mn_amount=1, expected_error='bad-cb-payee')

if __name__ == '__main__':
    DIP3Test().main()
