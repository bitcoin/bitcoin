#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test freeze."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

def rollback_chain(node, address):
        # Rolling back the chain to test reversing the last FREEZE tx
        blockcount = node.getblockcount()
        blockhash = node.getblockhash(blockcount)
        node.invalidateblock(blockhash)
        node.clearmempool()
        node.generatetoaddress(1, address)
        new_blockcount = node.getblockcount()
        new_blockhash = node.getblockhash(new_blockcount)

        # Checking the block count is the same as before the rollback...
        assert_equal(blockcount, new_blockcount)

        # Checking the block hash is different from before the rollback...
        if blockhash == new_blockhash:
            raise AssertionError("Block hashes should differ after reorg")

class OmniFreeze(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any']]

    def run_test(self):
        self.log.info("test freeze")

        node = self.nodes[0]

        # Obtaining addresses to work with
        address = node.getnewaddress()
        freeze_address = node.getnewaddress()
        coinbase_address = node.getnewaddress()

        # Preparing some mature Bitcoins
        node.generatetoaddress(105, coinbase_address)

        # Funding the addresses with some testnet BTC for fees
        node.sendmany("", {address: 5, freeze_address: 4})
        node.sendtoaddress(address, 6)
        node.sendtoaddress(address, 7)
        node.sendtoaddress(address, 8)
        node.sendtoaddress(address, 9)
        node.generatetoaddress(1, coinbase_address)

        # Creating a test (managed) property and granting 1000 tokens to the test address\
        node.omni_sendissuancemanaged(address, 1, 1, 0, "TestCat", "TestSubCat", "TestProperty", "TestURL", "TestData")
        node.generatetoaddress(1, coinbase_address)
        node.omni_sendgrant(address, freeze_address, 3, "1000")
        node.generatetoaddress(1, coinbase_address)

        # Running the test scenario...
        # Sending a 'freeze' transaction for the test address prior to enabling freezing
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Checking that freezing is currently disabled...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

        # Sending a 'enable freezing' transaction to ENABLE freezing
        txid = node.omni_sendenablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'enable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is now enabled...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], True)

        # Sending another 'freeze' transaction for the test address
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was now VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Testing a send from the test address (should now be frozen)
        txid = node.omni_send(freeze_address, address, 3, "50")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'send' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Checking the test address balance has not changed...
        balance = node.omni_getbalance(freeze_address, 3)['balance']
        assert_equal(balance, "1000")

        # Sending an 'unfreeze' transaction for the test address
        txid = node.omni_sendunfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'unfreeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Testing a send from the test address (should now be unfrozen)
        txid = node.omni_send(freeze_address, address, 3, "50")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'send' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking the test address balance has reduced by the amount of the send...
        balance = node.omni_getbalance(freeze_address, 3)['balance']
        assert_equal(balance, "950")

        # Sending another 'freeze' transaction for the test address
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending a 'disable freezing' transaction to DISABLE freezing
        txid = node.omni_senddisablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'disable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is now disabled...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

        # Testing a send from the test address (unfrozen when freezing was disabled)
        txid = node.omni_send(freeze_address, address, 3, "30")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'send' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking the test address balance has reduced by the amount of the send...
        balance = node.omni_getbalance(freeze_address, 3)['balance']
        assert_equal(balance, "920")

        # Sending a 'freeze' transaction for the test address to test that freezing is now disabled
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'unfreeze' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Sending a feature 14 activation to activate the notice period
        blockcount = node.getblockcount() + 8
        txid = node.omni_sendactivation(address, 14, blockcount, 999)
        node.generatetoaddress(1, coinbase_address)

        # Checking the activation transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        node.generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = node.omni_getactivations()['completedactivations']
        found = False
        for ids in featureid:
            if ids['featureid'] == 14:
                found = True
        assert_equal(found, True)

        # Sending a 'enable freezing' transaction to ENABLE freezing
        txid = node.omni_sendenablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'enable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is still disabled (due to wait period)...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

        # Sending a 'freeze' transaction for the test address before waiting period expiry
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Mining 10 blocks to forward past the waiting period
        node.generatetoaddress(10, coinbase_address)

        # Checking that freezing is now enabled...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], True)

        # Sending a 'freeze' transaction for the test address after waiting period expiry
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Testing a Send All from the test address (now frozen))
        txid = node.omni_sendall(freeze_address, address, 1)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'Send All' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Checking the test address balance has not changed...
        balance = node.omni_getbalance(freeze_address, 3)['balance']
        assert_equal(balance, "920")

        # Sending an 'unfreeze' transaction for the test address
        txid = node.omni_sendunfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'unfreeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        self.log.info("Running reorg test scenarios")

        # Rolling back the chain to test reversing the last UNFREEZE tx
        rollback_chain(node, coinbase_address)

        # Testing a send from the test address (should now be frozen again as the block that unfroze the address was dc'd)
        txid = node.omni_send(freeze_address, address, 3, "30")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'send' transaction was INVALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Sending an 'unfreeze' transaction for the test address
        txid = node.omni_sendunfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'unfreeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        node.generatetoaddress(3, coinbase_address)

        # Sending an 'freeze' transaction for the test address
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Rolling back the chain to test reversing the last FREEZE tx
        rollback_chain(node, coinbase_address)

        # Testing a send from the test address (should now be unfrozen again as the block that froze the address was dc'd)
        txid = node.omni_send(freeze_address, address, 3, "30")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'send' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending a 'freeze' transaction for the test address
        txid = node.omni_sendfreeze(address, freeze_address, 3, "1234")
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'freeze' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending a 'disable freezing' transaction to DISABLE freezing
        txid = node.omni_senddisablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'disable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is now disabled...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

        # Rolling back the chain to test reversing the last DISABLE FREEZEING tx\
        rollback_chain(node, coinbase_address)

        # Checking that freezing is now enabled (as the block that disabled it was dc'd)...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], True)

        # Sending a 'disable freezing' transaction to DISABLE freezing
        txid = node.omni_senddisablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'disable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is now disabled..
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)
        node.generatetoaddress(3, coinbase_address)

        # Sending a 'enable freezing' transaction to ENABLE freezing
        txid = node.omni_sendenablefreezing(address, 3)
        node.generatetoaddress(1, coinbase_address)

        # Checking the 'enable freezing' transaction was VALID...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking that freezing is still disabled (due to waiting period)...
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

        # Rolling back the chain to test reversing the last ENABLE FREEZEING tx
        rollback_chain(node, coinbase_address)

        # Mining past prior activation period and checking that freezing is still disabled...
        node.generatetoaddress(20, coinbase_address)
        result = node.omni_getproperty(3)
        assert_equal(result['freezingenabled'], False)

if __name__ == '__main__':
    OmniFreeze().main()
