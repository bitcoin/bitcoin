#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test deterministic masternodes
#

from test_framework.blocktools import create_block, create_coinbase, get_masternode_payment
from test_framework.mininode import CTransaction, ToHex, FromHex, CTxOut, COIN, CCbTx
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class Masternode(object):
    pass

class DIP3Test(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_initial_mn = 11 # Should be >= 11 to make sure quorums are not always the same MNs
        self.num_nodes = 1 + self.num_initial_mn + 2 # +1 for controller, +1 for mn-qt, +1 for mn created after dip3 activation
        self.setup_clean_chain = True

        self.extra_args = ["-budgetparams=240:100:240"]
        self.extra_args += ["-sporkkey=cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"]

    def setup_network(self):
        disable_mocktime()
        self.start_controller_node()
        self.is_network_split = False

    def start_controller_node(self, extra_args=None):
        print("starting controller node")
        if self.nodes is None:
            self.nodes = [None]
        args = self.extra_args
        if extra_args is not None:
            args += extra_args
        self.nodes[0] = start_node(0, self.options.tmpdir, extra_args=args)
        for i in range(1, self.num_nodes):
            if i < len(self.nodes) and self.nodes[i] is not None:
                connect_nodes_bi(self.nodes, 0, i)

    def stop_controller_node(self):
        print("stopping controller node")
        stop_node(self.nodes[0], 0)

    def restart_controller_node(self):
        self.stop_controller_node()
        self.start_controller_node()

    def run_test(self):
        print("funding controller node")
        while self.nodes[0].getbalance() < (self.num_initial_mn + 3) * 1000:
            self.nodes[0].generate(1) # generate enough for collaterals
        print("controller node has {} dash".format(self.nodes[0].getbalance()))

        # Make sure we're below block 143 (which activates dip3)
        print("testing rejection of ProTx before dip3 activation")
        assert(self.nodes[0].getblockchaininfo()['blocks'] < 143)
        dip3_deployment = self.nodes[0].getblockchaininfo()['bip9_softforks']['dip0003']
        assert_equal(dip3_deployment['status'], 'defined')

        self.test_fail_create_protx(self.nodes[0])

        mns = []
        mn_idx = 1
        for i in range(self.num_initial_mn):
            mn = self.create_mn(self.nodes[0], mn_idx, 'mn-%d' % (mn_idx))
            mn_idx += 1
            mns.append(mn)

        # mature collaterals
        for i in range(3):
            self.nodes[0].generate(1)
            time.sleep(1)

        self.write_mnconf(mns)

        self.restart_controller_node()
        for mn in mns:
            self.start_mn(mn)
        self.sync_all()

        # force finishing of mnsync
        for node in self.nodes:
            self.force_finish_mnsync(node)

        # start MNs
        print("start mns")
        for mn in mns:
            self.start_alias(self.nodes[0], mn.alias)
        print("wait for MNs to appear in MN lists")
        self.wait_for_mnlists(mns, True, False)

        print("testing MN payment votes")
        self.test_mn_votes(10)

        print("testing instant send")
        self.test_instantsend(10, 5)

        print("testing rejection of ProTx before dip3 activation (in states defined, started and locked_in)")
        while self.nodes[0].getblockchaininfo()['bip9_softforks']['dip0003']['status'] == 'defined':
            self.nodes[0].generate(1)
        self.test_fail_create_protx(self.nodes[0])
        while self.nodes[0].getblockchaininfo()['bip9_softforks']['dip0003']['status'] == 'started':
            self.nodes[0].generate(1)
        self.test_fail_create_protx(self.nodes[0])

        # prepare mn which should still be accepted later when dip3 activates
        print("creating collateral for mn-before-dip3")
        before_dip3_mn = self.create_mn(self.nodes[0], mn_idx, 'mn-before-dip3')
        mn_idx += 1

        while self.nodes[0].getblockchaininfo()['bip9_softforks']['dip0003']['status'] == 'locked_in':
            self.nodes[0].generate(1)

        # We have hundreds of blocks to sync here, give it more time
        print("syncing blocks for all nodes")
        sync_blocks(self.nodes, timeout=120)

        # DIP3 has activated here

        print("testing rejection of ProTx right before dip3 activation")
        best_block = self.nodes[0].getbestblockhash()
        self.nodes[0].invalidateblock(best_block)
        self.test_fail_create_protx(self.nodes[0])
        self.nodes[0].reconsiderblock(best_block)

        # Now it should be possible to mine ProTx
        self.sync_all()
        self.test_success_create_protx(self.nodes[0])

        print("creating collateral for mn-after-dip3")
        after_dip3_mn = self.create_mn(self.nodes[0], mn_idx, 'mn-after-dip3')
        # mature collaterals
        for i in range(3):
            self.nodes[0].generate(1)
            time.sleep(1)

        print("testing if we can start a mn which was created before dip3 activation")
        self.write_mnconf(mns + [before_dip3_mn, after_dip3_mn])
        self.restart_controller_node()
        self.force_finish_mnsync(self.nodes[0])

        print("start MN %s" % before_dip3_mn.alias)
        mns.append(before_dip3_mn)
        self.start_mn(before_dip3_mn)
        self.wait_for_sporks()
        self.force_finish_mnsync_list(before_dip3_mn.node)
        self.start_alias(self.nodes[0], before_dip3_mn.alias)

        self.wait_for_mnlists(mns)
        self.wait_for_mnlists_same()

        # Test if nodes still allow creating new non-ProTx MNs now
        print("testing if MN start succeeds when using collateral which was created after dip3 activation")
        print("start MN %s" % after_dip3_mn.alias)
        mns.append(after_dip3_mn)
        self.start_mn(after_dip3_mn)
        self.wait_for_sporks()
        self.force_finish_mnsync_list(after_dip3_mn.node)
        self.start_alias(self.nodes[0], after_dip3_mn.alias)

        self.wait_for_mnlists(mns)
        self.wait_for_mnlists_same()

        first_upgrade_count = 5
        mns_after_upgrade = []
        mns_to_restart = []
        mns_to_restart_later = []
        mns_protx = []
        print("upgrading first %d MNs to use ProTx (but not deterministic MN lists)" % first_upgrade_count)
        for i in range(first_upgrade_count):
            # let a few of the protx MNs refer to the old collaterals
            fund = (i % 2) == 0
            mns[i] = self.upgrade_mn_protx(mns[i], fund)
            self.nodes[0].generate(1)

            if fund:
                # collateral has moved, so we need to start it again
                mns_to_restart.append(mns[i])
            else:
                # collateral has not moved, so it should still be in the masternode list even after upgrade
                mns_after_upgrade.append(mns[i])
                mns_to_restart_later.append(mns[i])
            mns_protx.append(mns[i])
        for i in range(first_upgrade_count, len(mns)):
            mns_after_upgrade.append(mns[i])
        self.write_mnconf(mns)

        print("wait for freshly funded and upgraded MNs to disappear from MN lists (their collateral was spent)")
        self.wait_for_mnlists(mns_after_upgrade, check=True)
        self.wait_for_mnlists_same()

        print("restarting controller and upgraded MNs")
        self.restart_controller_node()
        self.force_finish_mnsync_list(self.nodes[0])
        for mn in mns_to_restart:
            print("restarting MN %s" % mn.alias)
            self.stop_node(mn.idx)
            self.start_mn(mn)
            self.force_finish_mnsync_list(mn.node)
        print('start-alias on upgraded nodes')
        for mn in mns_to_restart:
            self.start_alias(self.nodes[0], mn.alias)

        print("wait for upgraded MNs to appear in MN list")
        self.wait_for_mnlists(mns)
        self.wait_for_mnlists_same()

        print("testing MN payment votes (with mixed ProTx and legacy nodes)")
        self.test_mn_votes(10, test_enforcement=True)

        print("testing instant send (with mixed ProTx and legacy nodes)")
        self.test_instantsend(10, 3)

        # We still need to restart them as otherwise they won't have the BLS operator key loaded
        print("restart upgraded nodes which refer to old collaterals")
        for mn in mns_to_restart_later:
            print("restarting MN %s" % mn.alias)
            self.stop_node(mn.idx)
            self.start_mn(mn)
            self.force_finish_mnsync_list(mn.node)

        print("activating spork15")
        height = self.nodes[0].getblockchaininfo()['blocks']
        spork15_offset = 10
        self.nodes[0].spork('SPORK_15_DETERMINISTIC_MNS_ENABLED', height + spork15_offset)
        self.wait_for_sporks()

        print("test that MN list does not change before final spork15 activation")
        for i in range(spork15_offset - 1):
            self.nodes[0].generate(1)
            self.sync_all()
            self.wait_for_mnlists(mns)
            self.wait_for_mnlists_same()

        print("mining final block which should switch network to deterministic lists")
        self.nodes[0].generate(1)
        self.sync_all()

        ##### WOW...we made it...we are in deterministic MN lists mode now.
        ##### From now on, we don't wait for mnlists to become correct anymore, we always assert that they are correct immediately

        print("assert that not upgraded MNs disappeared from MN list")
        self.assert_mnlists(mns_protx)

        # enable enforcement and keep it on from now on
        self.nodes[0].spork('SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT', 0)
        self.wait_for_sporks()

        print("test that MNs disappear from the list when the ProTx collateral is spent")
        spend_mns_count = 3
        mns_tmp = [] + mns_protx
        dummy_txins = []
        for i in range(spend_mns_count):
            dummy_txin = self.spend_mn_collateral(mns_protx[i], with_dummy_input_output=True)
            dummy_txins.append(dummy_txin)
            self.nodes[0].generate(1)
            self.sync_all()
            mns_tmp.remove(mns_protx[i])
            self.assert_mnlists(mns_tmp)

        print("test that reverting the blockchain on a single node results in the mnlist to be reverted as well")
        for i in range(spend_mns_count):
            self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
            mns_tmp.append(mns_protx[spend_mns_count - 1 - i])
            self.assert_mnlist(self.nodes[0], mns_tmp)

        print("cause a reorg with a double spend and check that mnlists are still correct on all nodes")
        self.mine_double_spend(self.nodes[0], dummy_txins, self.nodes[0].getnewaddress(), use_mnmerkleroot_from_tip=True)
        self.nodes[0].generate(spend_mns_count)
        self.sync_all()
        self.assert_mnlists(mns_tmp)

        print("upgrade remaining MNs to ProTx")
        for i in range(first_upgrade_count, len(mns)):
            mns[i] = self.upgrade_mn_protx(mns[i], True)
            mn = mns[i]
            self.nodes[0].generate(1)
            mns_protx.append(mn)
            print("restarting MN %s" % mn.alias)
            self.stop_node(mn.idx)
            self.start_mn(mn)
            self.sync_all()
            self.force_finish_mnsync(mn.node)
            self.assert_mnlists(mns_protx)

        self.assert_mnlists(mns_protx)

        print("test mn payment enforcement with deterministic MNs")
        for i in range(20):
            node = self.nodes[i % len(self.nodes)]
            self.test_invalid_mn_payment(node)
            node.generate(1)
            self.sync_all()

        print("testing instant send with deterministic MNs")
        self.test_instantsend(10, 5, timeout=20)

        print("testing ProUpServTx")
        for mn in mns_protx:
            self.test_protx_update_service(mn)

        print("testing P2SH/multisig for payee addresses")
        multisig = self.nodes[0].createmultisig(1, [self.nodes[0].getnewaddress(), self.nodes[0].getnewaddress()])['address']
        self.update_mn_payee(mns_protx[0], multisig)
        found_multisig_payee = False
        for i in range(len(mns_protx)):
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

        print("testing reusing of collaterals for replaced MNs")
        for i in range(0, 5):
            mn = mns_protx[i]
            # a few of these will actually refer to old ProRegTx internal collaterals,
            # which should work the same as external collaterals
            mn = self.create_mn_protx(self.nodes[0], mn.idx, 'mn-protx-%d' % mn.idx, mn.collateral_txid, mn.collateral_vout)
            mns_protx[i] = mn
            self.nodes[0].generate(1)
            self.sync_all()
            self.assert_mnlists(mns_protx)
            print("restarting MN %s" % mn.alias)
            self.stop_node(mn.idx)
            self.start_mn(mn)
            self.sync_all()

        print("testing instant send with replaced MNs")
        self.test_instantsend(10, 3, timeout=20)

        print("testing simple PoSe")
        self.assert_mnlists(mns_protx)
        self.nodes[0].spork('SPORK_17_QUORUM_DKG_ENABLED', 0)
        self.wait_for_sporks()

        height = self.nodes[0].getblockcount()
        skip_count = 24 - (height % 24)
        if skip_count != 0:
            self.nodes[0].generate(skip_count)

        for i in range(len(mns_protx), len(mns_protx) - 2, -1):
            mn = mns_protx[len(mns_protx) - 1]
            mns_protx.remove(mn)
            self.stop_node(mn.idx)
            self.nodes.remove(mn.node)

            punished = False
            banned = False
            t = time.time()
            while (not punished or not banned) and (time.time() - t) < 120:
                time.sleep(1)

                # 10 blocks until we can mine the dummy commitment
                for j in range(10):
                    self.nodes[0].generate(1)
                    self.sync_all()
                    time.sleep(0.5)

                info = self.nodes[0].protx('info', mn.protx_hash)
                if not punished:
                    if info['state']['PoSePenalty'] > 0:
                        punished = True
                if not banned:
                    if info['state']['PoSeBanHeight'] != -1:
                        banned = True

                # Fast-forward to next DKG session
                self.nodes[0].generate(24 - (self.nodes[0].getblockcount() % 24))
                self.sync_all()
            assert(punished and banned)

    def create_mn(self, node, idx, alias):
        mn = Masternode()
        mn.idx = idx
        mn.alias = alias
        mn.is_protx = False
        mn.p2p_port = p2p_port(mn.idx)

        blsKey = node.bls('generate')
        mn.legacyMnkey = node.masternode('genkey')
        mn.blsMnkey = blsKey['secret']
        mn.collateral_address = node.getnewaddress()
        mn.collateral_txid = node.sendtoaddress(mn.collateral_address, 1000)
        rawtx = node.getrawtransaction(mn.collateral_txid, 1)

        mn.collateral_vout = -1
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                mn.collateral_vout = txout['n']
                break
        assert(mn.collateral_vout != -1)

        lock = node.lockunspent(False, [{'txid': mn.collateral_txid, 'vout': mn.collateral_vout}])

        return mn

    def create_mn_protx_base(self, node, idx, alias, legacy_mn_key):
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
        mn.legacyMnkey = node.masternode('genkey') if legacy_mn_key is None else legacy_mn_key
        mn.blsMnkey = blsKey['secret']

        return mn

    # create a protx MN and also fund it (using collateral inside ProRegTx)
    def create_mn_protx_fund(self, node, idx, alias, legacy_mn_key=None):
        mn = self.create_mn_protx_base(node, idx, alias, legacy_mn_key=legacy_mn_key)
        node.sendtoaddress(mn.fundsAddr, 1000.001)

        mn.collateral_address = node.getnewaddress()

        mn.protx_hash = node.protx('register_fund', mn.collateral_address, '127.0.0.1:%d' % mn.p2p_port, mn.ownerAddr, mn.operatorAddr, mn.votingAddr, 0, mn.collateral_address, mn.fundsAddr)
        mn.collateral_txid = mn.protx_hash
        mn.collateral_vout = -1

        rawtx = node.getrawtransaction(mn.collateral_txid, 1)
        for txout in rawtx['vout']:
            if txout['value'] == Decimal(1000):
                mn.collateral_vout = txout['n']
                break
        assert(mn.collateral_vout != -1)

        return mn

    # create a protx MN which refers to an existing collateral
    def create_mn_protx(self, node, idx, alias, collateral_txid, collateral_vout, legacy_mn_key=None):
        mn = self.create_mn_protx_base(node, idx, alias, legacy_mn_key=legacy_mn_key)
        node.sendtoaddress(mn.fundsAddr, 0.001)

        mn.rewards_address = node.getnewaddress()

        mn.protx_hash = node.protx('register', collateral_txid, collateral_vout, '127.0.0.1:%d' % mn.p2p_port, mn.ownerAddr, mn.operatorAddr, mn.votingAddr, 0, mn.rewards_address, mn.fundsAddr)
        mn.collateral_txid = collateral_txid
        mn.collateral_vout = collateral_vout

        return mn

    def start_mn(self, mn):
        while len(self.nodes) <= mn.idx:
            self.nodes.append(None)
        extra_args = ['-masternode=1', '-masternodeprivkey=%s' % mn.legacyMnkey, '-masternodeblsprivkey=%s' % mn.blsMnkey]
        n = start_node(mn.idx, self.options.tmpdir, self.extra_args + extra_args, redirect_stderr=True)
        self.nodes[mn.idx] = n
        for i in range(0, self.num_nodes):
            if i < len(self.nodes) and self.nodes[i] is not None and i != mn.idx:
                connect_nodes_bi(self.nodes, mn.idx, i)
        mn.node = self.nodes[mn.idx]
        self.sync_all()

    def spend_mn_collateral(self, mn, with_dummy_input_output=False):
        return self.spend_input(mn.collateral_txid, mn.collateral_vout, 1000, with_dummy_input_output)

    def upgrade_mn_protx(self, mn, refund):
        if refund:
            self.spend_mn_collateral(mn)
            mn = self.create_mn_protx_fund(self.nodes[0], mn.idx, 'mn-protx-%d' % mn.idx, legacy_mn_key=mn.legacyMnkey)
        else:
            mn = self.create_mn_protx(self.nodes[0], mn.idx, 'mn-protx-%d' % mn.idx, mn.collateral_txid, mn.collateral_vout, legacy_mn_key=mn.legacyMnkey)
        return mn

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

    def force_finish_mnsync(self, node):
        while True:
            s = node.mnsync('next')
            if s == 'sync updated to MASTERNODE_SYNC_FINISHED':
                break
            time.sleep(0.1)

    def force_finish_mnsync_list(self, node):
        if node.mnsync('status')['AssetName'] == 'MASTERNODE_SYNC_WAITING':
            node.mnsync('next')

        while True:
            mnlist = node.masternode('list', 'status')
            if len(mnlist) != 0:
                time.sleep(0.5)
                self.force_finish_mnsync(node)
                return
            time.sleep(0.1)

    def write_mnconf_line(self, mn, f):
        conf_line = "%s %s:%d %s %s %d\n" % (mn.alias, '127.0.0.1', mn.p2p_port, mn.legacyMnkey, mn.collateral_txid, mn.collateral_vout)
        f.write(conf_line)

    def write_mnconf(self, mns):
        mnconf_file = os.path.join(self.options.tmpdir, "node0/regtest/masternode.conf")
        with open(mnconf_file, 'w') as f:
            for mn in mns:
                self.write_mnconf_line(mn, f)

    def start_alias(self, node, alias, should_fail=False):
        # When generating blocks very fast, the logic in miner.cpp:UpdateTime might result in block times ahead of the real time
        # This can easily accumulate to 30 seconds or more, which results in start-alias to fail as it expects the sigTime
        # to be less or equal to the confirmation block time
        # Solution is to sleep in this case.
        lastblocktime = node.getblock(node.getbestblockhash())['time']
        sleeptime = lastblocktime - time.time()
        if sleeptime > 0:
            time.sleep(sleeptime + 1) # +1 to be extra sure

        start_result = node.masternode('start-alias', alias)
        if not should_fail:
            assert_equal(start_result, {'result': 'successful', 'alias': alias})
        else:
            assert_equal(start_result, {'result': 'failed', 'alias': alias, 'errorMessage': 'Failed to verify MNB'})

    def generate_blocks_until_winners(self, node, count, timeout=60):
        # Winner lists are pretty much messed up when too many blocks were generated in a short time
        # To allow proper testing of winners list, we need to slowly generate a few blocks until the list stabilizes
        good_count = 0
        st = time.time()
        while time.time() < st + timeout:
            height = node.getblockchaininfo()['blocks'] + 10
            winners = node.masternode('winners')
            if str(height) in winners:
                if re.match('[0-9a-zA-Z]*:10', winners[str(height)]):
                    good_count += 1
                    if good_count >= count:
                        return
                else:
                    good_count = 0
            node.generate(1)
            self.sync_all()
            time.sleep(1)
        raise AssertionError("generate_blocks_until_winners timed out: {}".format(node.masternode('winners')))

    def test_mn_votes(self, block_count, test_enforcement=False):
        self.generate_blocks_until_winners(self.nodes[0], self.num_nodes)

        if test_enforcement:
            self.nodes[0].spork('SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT', 0)
            self.wait_for_sporks()
            self.test_invalid_mn_payment(self.nodes[0])

        cur_block = 0
        while cur_block < block_count:
            for n1 in self.nodes:
                if cur_block >= block_count:
                    break
                if n1 is None:
                    continue

                if test_enforcement:
                    self.test_invalid_mn_payment(n1)

                n1.generate(1)
                cur_block += 1
                self.sync_all()

                height = n1.getblockchaininfo()['blocks']
                winners = self.wait_for_winners(n1, height + 10)

                for n2 in self.nodes:
                    if n1 is n2 or n2 is None:
                        continue
                    winners2 = self.wait_for_winners(n2, height + 10)
                    if winners[str(height + 10)] != winners2[str(height + 10)]:
                        print("winner1: " + str(winners[str(height + 10)]))
                        print("winner2: " + str(winners2[str(height + 10)]))
                        raise AssertionError("winners did not match")

        if test_enforcement:
            self.nodes[0].spork('SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT', 4070908800)

    def test_instantsend(self, tx_count, repeat, timeout=20):
        self.nodes[0].spork('SPORK_2_INSTANTSEND_ENABLED', 0)
        self.wait_for_sporks()

        # give all nodes some coins first
        for i in range(tx_count):
            outputs = {}
            for node in self.nodes[1:]:
                outputs[node.getnewaddress()] = 1
            rawtx = self.nodes[0].createrawtransaction([], outputs)
            rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
            rawtx = self.nodes[0].signrawtransaction(rawtx)['hex']
            self.nodes[0].sendrawtransaction(rawtx)
            self.nodes[0].generate(1)
        self.sync_all()

        for j in range(repeat):
            for i in range(tx_count):
                while True:
                    from_node_idx = random.randint(0, len(self.nodes) - 1)
                    from_node = self.nodes[from_node_idx]
                    if from_node is not None:
                        break
                while True:
                    to_node_idx = random.randint(0, len(self.nodes) - 1)
                    to_node = self.nodes[to_node_idx]
                    if to_node is not None and from_node is not to_node:
                        break
                to_address = to_node.getnewaddress()
                txid = from_node.instantsendtoaddress(to_address, 0.01)
                for node in self.nodes:
                    if node is not None:
                        self.wait_for_instant_lock(node, to_node_idx, txid, timeout=timeout)
            self.nodes[0].generate(6)
            self.sync_all()

    def wait_for_instant_lock(self, node, node_idx, txid, timeout=10):
        st = time.time()
        while time.time() < st + timeout:
            try:
                tx = node.getrawtransaction(txid, 1)
            except:
                tx = None
            if tx is None:
                time.sleep(0.5)
                continue
            if tx['instantlock']:
                return
            time.sleep(0.5)
        raise AssertionError("wait_for_instant_lock timed out for: {} on node {}".format(txid, node_idx))

    def wait_for_winners(self, node, height, timeout=5):
        st = time.time()
        while time.time() < st + timeout:
            winners = node.masternode('winners')
            if str(height) in winners:
                if re.match('[0-9a-zA-Z]*:10', winners[str(height)]):
                    return winners
            time.sleep(0.5)
        raise AssertionError("wait_for_winners for height {} timed out: {}".format(height, node.masternode('winners')))

    def wait_for_mnlists(self, mns, timeout=30, check=False):
        for node in self.nodes:
            self.wait_for_mnlist(node, mns, timeout, check=check)

    def wait_for_mnlist(self, node, mns, timeout=30, check=False):
        st = time.time()
        while time.time() < st + timeout:
            if check:
                node.masternode('check')
            if self.compare_mnlist(node, mns):
                return
            time.sleep(0.5)
        raise AssertionError("wait_for_mnlist timed out")

    def assert_mnlists(self, mns):
        for node in self.nodes:
            self.assert_mnlist(node, mns)

    def assert_mnlist(self, node, mns):
        if not self.compare_mnlist(node, mns):
            expected = []
            for mn in mns:
                expected.append('%s-%d' % (mn.collateral_txid, mn.collateral_vout))
            print('mnlist: ' + str(node.masternode('list', 'status')))
            print('expected: ' + str(expected))
            raise AssertionError("mnlists does not match provided mns")

    def wait_for_sporks(self, timeout=30):
        st = time.time()
        while time.time() < st + timeout:
            if self.compare_sporks():
                return
            time.sleep(0.5)
        raise AssertionError("wait_for_sporks timed out")

    def compare_sporks(self):
        sporks = self.nodes[0].spork('show')
        for node in self.nodes[1:]:
            sporks2 = node.spork('show')
            if sporks != sporks2:
                return False
        return True

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

    def wait_for_mnlists_same(self, timeout=30):
        st = time.time()
        while time.time() < st + timeout:
            mnlist = self.nodes[0].masternode('list', 'status')
            all_match = True
            for node in self.nodes[1:]:
                mnlist2 = node.masternode('list', 'status')
                if mnlist != mnlist2:
                    all_match = False
                    break
            if all_match:
                return
            time.sleep(0.5)
        raise AssertionError("wait_for_mnlists_same timed out")

    def test_fail_create_protx(self, node):
        # Try to create ProTx (should still fail)
        fund_address = node.getnewaddress()
        address = node.getnewaddress()
        node.sendtoaddress(fund_address, 1000.001) # +0.001 for fees
        key = node.getnewaddress()
        blsKey = node.bls('generate')
        assert_raises_jsonrpc(None, "bad-tx-type", node.protx, 'register_fund', address, '127.0.0.1:10000', key, blsKey['public'], key, 0, address, fund_address)

    def test_success_create_protx(self, node):
        fund_address = node.getnewaddress()
        address = node.getnewaddress()
        txid = node.sendtoaddress(fund_address, 1000.001) # +0.001 for fees
        key = node.getnewaddress()
        blsKey = node.bls('generate')
        node.protx('register_fund', address, '127.0.0.1:10000', key, blsKey['public'], key, 0, address, fund_address)
        rawtx = node.getrawtransaction(txid, 1)
        self.mine_double_spend(node, rawtx['vin'], address, use_mnmerkleroot_from_tip=True)
        self.sync_all()

    def spend_input(self, txid, vout, amount, with_dummy_input_output=False):
        # with_dummy_input_output is useful if you want to test reorgs with double spends of the TX without touching the actual txid/vout
        address = self.nodes[0].getnewaddress()
        target = {address: amount}
        if with_dummy_input_output:
            dummyaddress = self.nodes[0].getnewaddress()
            target[dummyaddress] = 1
        rawtx = self.nodes[0].createrawtransaction([{'txid': txid, 'vout': vout}], target)
        rawtx = self.nodes[0].fundrawtransaction(rawtx)['hex']
        rawtx = self.nodes[0].signrawtransaction(rawtx)['hex']
        new_txid = self.nodes[0].sendrawtransaction(rawtx)

        if with_dummy_input_output:
            decoded = self.nodes[0].decoderawtransaction(rawtx)
            for i in range(len(decoded['vout'])):
                # make sure this one can only be spent when explicitely creating a rawtx with these outputs as inputs
                # this ensures that no other TX is chaining on top of this TX
                lock = self.nodes[0].lockunspent(False, [{'txid': new_txid, 'vout': i}])
            for txin in decoded['vin']:
                if txin['txid'] != txid or txin['vout'] != vout:
                    return txin
        return None

    def mine_block(self, node, vtx=[], miner_address=None, mn_payee=None, mn_amount=None, use_mnmerkleroot_from_tip=False, expected_error=None):
        bt = node.getblocktemplate()
        height = bt['height']
        tip_hash = bt['previousblockhash']

        tip_block = node.getblock(tip_hash)

        coinbasevalue = bt['coinbasevalue']
        if miner_address is None:
            miner_address = node.getnewaddress()
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
                txout = node.gettxout("%064x" % txin.prevout.hash, txin.prevout.n, False)
                in_value += int(txout['value'] * COIN)
            for txout in tx.vout:
                out_value += txout.nValue
            new_fees += in_value - out_value

        # fix fees
        coinbasevalue -= bt_fees
        coinbasevalue += new_fees

        if mn_amount is None:
            mn_amount = get_masternode_payment(height, coinbasevalue)
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
        rawtx = node.signrawtransaction(rawtx)['hex']
        tx = FromHex(CTransaction(), rawtx)

        self.mine_block(node, [tx], use_mnmerkleroot_from_tip=use_mnmerkleroot_from_tip)

    def test_invalid_mn_payment(self, node):
        mn_payee = self.nodes[0].getnewaddress()
        self.mine_block(node, mn_payee=mn_payee, expected_error='bad-cb-payee')
        self.mine_block(node, mn_amount=1, expected_error='bad-cb-payee')

if __name__ == '__main__':
    DIP3Test().main()
