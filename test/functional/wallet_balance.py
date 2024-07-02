#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet balance RPC methods."""
from decimal import Decimal
import time

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE as ADDRESS_WATCHONLY
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_is_hash_string,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import get_generate_key


def create_transactions(node, address, amt, fees):
    # Create and sign raw transactions from node to address for amt.
    # Creates a transaction for each fee and returns an array
    # of the raw transactions.
    utxos = [u for u in node.listunspent(0) if u['spendable']]

    # Create transactions
    inputs = []
    ins_total = 0
    for utxo in utxos:
        inputs.append({"txid": utxo["txid"], "vout": utxo["vout"]})
        ins_total += utxo['amount']
        if ins_total >= amt + max(fees):
            break
    # make sure there was enough utxos
    assert ins_total >= amt + max(fees)

    txs = []
    for fee in fees:
        outputs = {address: amt}
        # prevent 0 change output
        if ins_total > amt + fee:
            outputs[node.getrawchangeaddress()] = ins_total - amt - fee
        raw_tx = node.createrawtransaction(inputs, outputs, 0, True)
        raw_tx = node.signrawtransactionwithwallet(raw_tx)
        assert_equal(raw_tx['complete'], True)
        txs.append(raw_tx)

    return txs

class WalletTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [
            # Limit mempool descendants as a hack to have wallet txs rejected from the mempool.
            # Set walletrejectlongchains=0 so the wallet still creates the transactions.
            ['-limitdescendantcount=3', '-walletrejectlongchains=0'],
            [],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        if not self.options.descriptors:
            # Tests legacy watchonly behavior which is not present (and does not need to be tested) in descriptor wallets
            self.nodes[0].importaddress(ADDRESS_WATCHONLY)
            # Check that nodes don't own any UTXOs
            assert_equal(len(self.nodes[0].listunspent()), 0)
            assert_equal(len(self.nodes[1].listunspent()), 0)

            self.log.info("Check that only node 0 is watching an address")
            assert 'watchonly' in self.nodes[0].getbalances()
            assert 'watchonly' not in self.nodes[1].getbalances()

        self.log.info("Mining blocks ...")
        self.generate(self.nodes[0], 1)
        self.generate(self.nodes[1], 1)

        # Verify listunspent returns immature coinbase if 'include_immature_coinbase' is set
        assert_equal(len(self.nodes[0].listunspent(query_options={'include_immature_coinbase': True})), 1)
        assert_equal(len(self.nodes[0].listunspent(query_options={'include_immature_coinbase': False})), 0)

        self.generatetoaddress(self.nodes[1], COINBASE_MATURITY + 1, ADDRESS_WATCHONLY)

        # Verify listunspent returns all immature coinbases if 'include_immature_coinbase' is set
        # For now, only the legacy wallet will see the coinbases going to the imported 'ADDRESS_WATCHONLY'
        assert_equal(len(self.nodes[0].listunspent(query_options={'include_immature_coinbase': False})), 1 if self.options.descriptors else 2)
        assert_equal(len(self.nodes[0].listunspent(query_options={'include_immature_coinbase': True})), 1 if self.options.descriptors else COINBASE_MATURITY + 2)

        if not self.options.descriptors:
            # Tests legacy watchonly behavior which is not present (and does not need to be tested) in descriptor wallets
            assert_equal(self.nodes[0].getbalances()['mine']['trusted'], 50)
            assert_equal(self.nodes[0].getwalletinfo()['balance'], 50)
            assert_equal(self.nodes[1].getbalances()['mine']['trusted'], 50)

            assert_equal(self.nodes[0].getbalances()['watchonly']['immature'], 5000)
            assert 'watchonly' not in self.nodes[1].getbalances()

            assert_equal(self.nodes[0].getbalance(), 50)
            assert_equal(self.nodes[1].getbalance(), 50)

        self.log.info("Test getbalance with different arguments")
        assert_equal(self.nodes[0].getbalance("*"), 50)
        assert_equal(self.nodes[0].getbalance("*", 1), 50)
        assert_equal(self.nodes[0].getbalance(minconf=1), 50)
        if not self.options.descriptors:
            assert_equal(self.nodes[0].getbalance(minconf=0, include_watchonly=True), 100)
            assert_equal(self.nodes[0].getbalance("*", 1, True), 100)
        else:
            assert_equal(self.nodes[0].getbalance(minconf=0, include_watchonly=True), 50)
            assert_equal(self.nodes[0].getbalance("*", 1, True), 50)
        assert_equal(self.nodes[1].getbalance(minconf=0, include_watchonly=True), 50)

        # Send 40 BTC from 0 to 1 and 60 BTC from 1 to 0.
        txs = create_transactions(self.nodes[0], self.nodes[1].getnewaddress(), 40, [Decimal('0.01')])
        self.nodes[0].sendrawtransaction(txs[0]['hex'])
        self.nodes[1].sendrawtransaction(txs[0]['hex'])  # sending on both nodes is faster than waiting for propagation

        self.sync_all()
        txs = create_transactions(self.nodes[1], self.nodes[0].getnewaddress(), 60, [Decimal('0.01'), Decimal('0.02')])
        self.nodes[1].sendrawtransaction(txs[0]['hex'])
        self.nodes[0].sendrawtransaction(txs[0]['hex'])  # sending on both nodes is faster than waiting for propagation
        self.sync_all()

        # First argument of getbalance must be set to "*"
        assert_raises_rpc_error(-32, "dummy first argument must be excluded or set to \"*\"", self.nodes[1].getbalance, "")

        self.log.info("Test balances with unconfirmed inputs")

        # Before `test_balance()`, we have had two nodes with a balance of 50
        # each and then we:
        #
        # 1) Sent 40 from node A to node B with fee 0.01
        # 2) Sent 60 from node B to node A with fee 0.01
        #
        # Then we check the balances:
        #
        # 1) As is
        # 2) With transaction 2 from above with 2x the fee
        #
        # Prior to #16766, in this situation, the node would immediately report
        # a balance of 30 on node B as unconfirmed and trusted.
        #
        # After #16766, we show that balance as unconfirmed.
        #
        # The balance is indeed "trusted" and "confirmed" insofar as removing
        # the mempool transactions would return at least that much money. But
        # the algorithm after #16766 marks it as unconfirmed because the 'taint'
        # tracking of transaction trust for summing balances doesn't consider
        # which inputs belong to a user. In this case, the change output in
        # question could be "destroyed" by replace the 1st transaction above.
        #
        # The post #16766 behavior is correct; we shouldn't be treating those
        # funds as confirmed. If you want to rely on that specific UTXO existing
        # which has given you that balance, you cannot, as a third party
        # spending the other input would destroy that unconfirmed.
        #
        # For example, if the test transactions were:
        #
        # 1) Sent 40 from node A to node B with fee 0.01
        # 2) Sent 10 from node B to node A with fee 0.01
        #
        # Then our node would report a confirmed balance of 40 + 50 - 10 = 80
        # BTC, which is more than would be available if transaction 1 were
        # replaced.


        def test_balances(*, fee_node_1=0):
            # getbalances
            expected_balances_0 = {'mine':      {'immature':          Decimal('0E-8'),
                                                 'trusted':           Decimal('9.99'),  # change from node 0's send
                                                 'untrusted_pending': Decimal('60.0')},
                                   'watchonly': {'immature':          Decimal('5000'),
                                                 'trusted':           Decimal('50.0'),
                                                 'untrusted_pending': Decimal('0E-8')}}
            expected_balances_1 = {'mine':      {'immature':          Decimal('0E-8'),
                                                 'trusted':           Decimal('0E-8'),  # node 1's send had an unsafe input
                                                 'untrusted_pending': Decimal('30.0') - fee_node_1}}  # Doesn't include output of node 0's send since it was spent
            if self.options.descriptors:
                del expected_balances_0["watchonly"]
            balances_0 = self.nodes[0].getbalances()
            balances_1 = self.nodes[1].getbalances()
            # remove lastprocessedblock keys (they will be tested later)
            del balances_0['lastprocessedblock']
            del balances_1['lastprocessedblock']
            assert_equal(balances_0, expected_balances_0)
            assert_equal(balances_1, expected_balances_1)
            # getbalance without any arguments includes unconfirmed transactions, but not untrusted transactions
            assert_equal(self.nodes[0].getbalance(), Decimal('9.99'))  # change from node 0's send
            assert_equal(self.nodes[1].getbalance(), Decimal('0'))  # node 1's send had an unsafe input
            # Same with minconf=0
            assert_equal(self.nodes[0].getbalance(minconf=0), Decimal('9.99'))
            assert_equal(self.nodes[1].getbalance(minconf=0), Decimal('0'))
            # getbalance with a minconf incorrectly excludes coins that have been spent more recently than the minconf blocks ago
            # TODO: fix getbalance tracking of coin spentness depth
            assert_equal(self.nodes[0].getbalance(minconf=1), Decimal('0'))
            assert_equal(self.nodes[1].getbalance(minconf=1), Decimal('0'))
            # getunconfirmedbalance
            assert_equal(self.nodes[0].getunconfirmedbalance(), Decimal('60'))  # output of node 1's spend
            assert_equal(self.nodes[1].getunconfirmedbalance(), Decimal('30') - fee_node_1)  # Doesn't include output of node 0's send since it was spent
            # getwalletinfo.unconfirmed_balance
            assert_equal(self.nodes[0].getwalletinfo()["unconfirmed_balance"], Decimal('60'))
            assert_equal(self.nodes[1].getwalletinfo()["unconfirmed_balance"], Decimal('30') - fee_node_1)

        test_balances(fee_node_1=Decimal('0.01'))

        # Node 1 bumps the transaction fee and resends
        self.nodes[1].sendrawtransaction(txs[1]['hex'])
        self.nodes[0].sendrawtransaction(txs[1]['hex'])  # sending on both nodes is faster than waiting for propagation
        self.sync_all()

        self.log.info("Test getbalance and getbalances.mine.untrusted_pending with conflicted unconfirmed inputs")
        test_balances(fee_node_1=Decimal('0.02'))

        self.generatetoaddress(self.nodes[1], 1, ADDRESS_WATCHONLY)

        # balances are correct after the transactions are confirmed
        balance_node0 = Decimal('69.99')  # node 1's send plus change from node 0's send
        balance_node1 = Decimal('29.98')  # change from node 0's send
        assert_equal(self.nodes[0].getbalances()['mine']['trusted'], balance_node0)
        assert_equal(self.nodes[1].getbalances()['mine']['trusted'], balance_node1)
        assert_equal(self.nodes[0].getbalance(), balance_node0)
        assert_equal(self.nodes[1].getbalance(), balance_node1)

        # Send total balance away from node 1
        txs = create_transactions(self.nodes[1], self.nodes[0].getnewaddress(), Decimal('29.97'), [Decimal('0.01')])
        self.nodes[1].sendrawtransaction(txs[0]['hex'])
        self.generatetoaddress(self.nodes[1], 2, ADDRESS_WATCHONLY)

        # getbalance with a minconf incorrectly excludes coins that have been spent more recently than the minconf blocks ago
        # TODO: fix getbalance tracking of coin spentness depth
        # getbalance with minconf=3 should still show the old balance
        assert_equal(self.nodes[1].getbalance(minconf=3), Decimal('0'))

        # getbalance with minconf=2 will show the new balance.
        assert_equal(self.nodes[1].getbalance(minconf=2), Decimal('0'))

        # check mempool transactions count for wallet unconfirmed balance after
        # dynamically loading the wallet.
        before = self.nodes[1].getbalances()['mine']['untrusted_pending']
        dst = self.nodes[1].getnewaddress()
        self.nodes[1].unloadwallet(self.default_wallet_name)
        self.nodes[0].sendtoaddress(dst, 0.1)
        self.sync_all()
        self.nodes[1].loadwallet(self.default_wallet_name)
        after = self.nodes[1].getbalances()['mine']['untrusted_pending']
        assert_equal(before + Decimal('0.1'), after)

        # Create 3 more wallet txs, where the last is not accepted to the
        # mempool because it is the third descendant of the tx above
        for _ in range(3):
            # Set amount high enough such that all coins are spent by each tx
            txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 99)

        self.log.info('Check that wallet txs not in the mempool are untrusted')
        assert txid not in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].gettransaction(txid)['trusted'], False)
        assert_equal(self.nodes[0].getbalance(minconf=0), 0)

        self.log.info("Test replacement and reorg of non-mempool tx")
        tx_orig = self.nodes[0].gettransaction(txid)['hex']
        # Increase fee by 1 coin
        tx_replace = tx_orig.replace(
            (99 * 10**8).to_bytes(8, "little", signed=True).hex(),
            (98 * 10**8).to_bytes(8, "little", signed=True).hex(),
        )
        tx_replace = self.nodes[0].signrawtransactionwithwallet(tx_replace)['hex']
        # Total balance is given by the sum of outputs of the tx
        total_amount = sum([o['value'] for o in self.nodes[0].decoderawtransaction(tx_replace)['vout']])
        self.sync_all()
        self.nodes[1].sendrawtransaction(hexstring=tx_replace, maxfeerate=0)

        # Now confirm tx_replace
        block_reorg = self.generatetoaddress(self.nodes[1], 1, ADDRESS_WATCHONLY)[0]
        assert_equal(self.nodes[0].getbalance(minconf=0), total_amount)

        self.log.info('Put txs back into mempool of node 1 (not node 0)')
        self.nodes[0].invalidateblock(block_reorg)
        self.nodes[1].invalidateblock(block_reorg)
        assert_equal(self.nodes[0].getbalance(minconf=0), 0)  # wallet txs not in the mempool are untrusted
        self.generatetoaddress(self.nodes[0], 1, ADDRESS_WATCHONLY, sync_fun=self.no_op)

        # Now confirm tx_orig
        self.restart_node(1, ['-persistmempool=0'])
        self.connect_nodes(0, 1)
        self.sync_blocks()
        self.nodes[1].sendrawtransaction(tx_orig)
        self.generatetoaddress(self.nodes[1], 1, ADDRESS_WATCHONLY)
        assert_equal(self.nodes[0].getbalance(minconf=0), total_amount + 1)  # The reorg recovered our fee of 1 coin

        if not self.options.descriptors:
            self.log.info('Check if mempool is taken into account after import*')
            address = self.nodes[0].getnewaddress()
            privkey = self.nodes[0].dumpprivkey(address)
            self.nodes[0].sendtoaddress(address, 0.1)
            self.nodes[0].unloadwallet('')
            # check importaddress on fresh wallet
            self.nodes[0].createwallet('w1', False, True)
            self.nodes[0].importaddress(address)
            assert_equal(self.nodes[0].getbalances()['mine']['untrusted_pending'], 0)
            assert_equal(self.nodes[0].getbalances()['watchonly']['untrusted_pending'], Decimal('0.1'))
            self.nodes[0].importprivkey(privkey)
            assert_equal(self.nodes[0].getbalances()['mine']['untrusted_pending'], Decimal('0.1'))
            assert_equal(self.nodes[0].getbalances()['watchonly']['untrusted_pending'], 0)
            self.nodes[0].unloadwallet('w1')
            # check importprivkey on fresh wallet
            self.nodes[0].createwallet('w2', False, True)
            self.nodes[0].importprivkey(privkey)
            assert_equal(self.nodes[0].getbalances()['mine']['untrusted_pending'], Decimal('0.1'))
            self.nodes[0].unloadwallet("w2")

            self.log.info("Test that an import that makes something spendable updates \"mine\" balance")
            self.nodes[0].loadwallet(self.default_wallet_name)
            default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
            self.nodes[0].createwallet(wallet_name="legacyspendableupdate", descriptors=False)
            wallet = self.nodes[0].get_wallet_rpc("legacyspendableupdate")

            import_key1 = get_generate_key()
            import_key2 = get_generate_key()
            wallet.importaddress(import_key1.p2wpkh_addr)
            wallet.importaddress(import_key2.p2wpkh_addr)

            amount = Decimal(15)
            default.sendtoaddress(import_key1.p2wpkh_addr, amount)
            default.sendtoaddress(import_key2.p2wpkh_addr, amount)
            self.generate(self.nodes[0], 1)

            balances = wallet.getbalances()
            assert_equal(balances["mine"]["trusted"], 0)
            assert_equal(balances["watchonly"]["trusted"], amount * 2)

            # Rescanning should always update the txos by virtue of finding them again
            wallet.importprivkey(privkey=import_key1.privkey, rescan=True)
            balances = wallet.getbalances()
            assert_equal(balances["mine"]["trusted"], amount)
            assert_equal(balances["watchonly"]["trusted"], amount)

            # Don't rescan to make sure that the import updates the wallet txos
            wallet.importprivkey(privkey=import_key2.privkey, rescan=False)
            balances = wallet.getbalances()
            assert_equal(balances["mine"]["trusted"], amount * 2)
            assert_equal(balances["watchonly"]["trusted"], 0)
            self.nodes[0].unloadwallet("legacyspendableupdate")

        # Tests the lastprocessedblock JSON object in getbalances, getwalletinfo
        # and gettransaction by checking for valid hex strings and by comparing
        # the hashes & heights between generated blocks.
        self.log.info("Test getbalances returns expected lastprocessedblock json object")
        prev_hash = self.nodes[0].getbestblockhash()
        prev_height = self.nodes[0].getblock(prev_hash)['height']
        self.generatetoaddress(self.nodes[0], 5, self.nodes[0].get_deterministic_priv_key().address)
        lastblock = self.nodes[0].getbalances()['lastprocessedblock']
        assert_is_hash_string(lastblock['hash'])
        assert_equal((prev_hash == lastblock['hash']), False)
        assert_equal(lastblock['height'], prev_height + 5)

        prev_hash = self.nodes[0].getbestblockhash()
        prev_height = self.nodes[0].getblock(prev_hash)['height']
        self.log.info("Test getwalletinfo returns expected lastprocessedblock json object")
        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['lastprocessedblock']['height'], prev_height)
        assert_equal(walletinfo['lastprocessedblock']['hash'], prev_hash)

        self.log.info("Test gettransaction returns expected lastprocessedblock json object")
        txid = self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        tx_info = self.nodes[1].gettransaction(txid)
        assert_equal(tx_info['lastprocessedblock']['height'], prev_height)
        assert_equal(tx_info['lastprocessedblock']['hash'], prev_hash)

        self.log.info("Test that the balance is updated by an import that makes an untracked output in an existing tx \"mine\"")
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("importupdate")
        wallet = self.nodes[0].get_wallet_rpc("importupdate")

        import_key1 = get_generate_key()
        import_key2 = get_generate_key()
        wallet.importprivkey(import_key1.privkey)

        amount = 15
        default.send([{import_key1.p2wpkh_addr: amount},{import_key2.p2wpkh_addr: amount}])
        self.generate(self.nodes[0], 1)
        # Mock the time forward by 1 day so that "now" will exclude the block we just mined
        self.nodes[0].setmocktime(int(time.time()) + 86400)
        # Mine 11 blocks to move the MTP past the block we just mined
        self.generate(self.nodes[0], 11, sync_fun=self.no_op)

        balances = wallet.getbalances()
        assert_equal(balances["mine"]["trusted"], amount)

        # Don't rescan to make sure that the import updates the wallet txos
        wallet.importprivkey(privkey=import_key2.privkey, rescan=False)
        balances = wallet.getbalances()
        assert_equal(balances["mine"]["trusted"], amount * 2)

if __name__ == '__main__':
    WalletTest().main()
