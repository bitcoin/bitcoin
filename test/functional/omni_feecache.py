#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Omni fee cache."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class OmniFeeCache(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any', '-disabledevmsc=1']]

    def run_test(self):
        self.log.info("test fee cache")

        node = self.nodes[0]

        # Preparing some mature Bitcoins
        coinbase_address = node.getnewaddress()
        node.generatetoaddress(102, coinbase_address)

        # Obtaining a master address to work with
        address = node.getnewaddress()

        # Funding the address with some testnet BTC for fees
        node.sendtoaddress(address, 20)
        node.sendtoaddress(address, 20)
        node.sendtoaddress(address, 20)
        node.generatetoaddress(1, coinbase_address)

        # Participating in the Exodus crowdsale to obtain some OMNI
        txid = node.sendmany("", {"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP": 10, address: 4})
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid.
        result = node.gettransaction(txid)
        assert_equal(result['confirmations'], 1)

        # Creating an indivisible test property
        node.omni_sendissuancefixed(address, 1, 1, 0, "Z_TestCat", "Z_TestSubCat", "Z_IndivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        node.generatetoaddress(1, coinbase_address)

        # Creating a second indivisible test property
        node.omni_sendissuancefixed(address, 1, 1, 0, "Z_TestCat", "Z_TestSubCat", "Z_IndivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        node.generatetoaddress(1, coinbase_address)

        # Creating a divisible test property
        node.omni_sendissuancefixed(address, 1, 2, 0, "Z_TestCat", "Z_TestSubCat", "Z_DivisTestProperty", "Z_TestURL", "Z_TestData", "10000")
        node.generatetoaddress(1, coinbase_address)

        # Creating a second divisible test property
        node.omni_sendissuancefixed(address, 1, 2, 0, "Z_TestCat", "Z_TestSubCat", "Z_DivisTestProperty", "Z_TestURL", "Z_TestData", "10000")
        node.generatetoaddress(1, coinbase_address)

        # Creating an indivisible test property in the test ecosystem
        node.omni_sendissuancefixed(address, 2, 1, 0, "Z_TestCat", "Z_TestSubCat", "Z_IndivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        node.generatetoaddress(1, coinbase_address)

        # Creating a divisible test property in the test ecosystem
        node.omni_sendissuancefixed(address, 2, 2, 0, "Z_TestCat", "Z_TestSubCat", "Z_DivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        node.generatetoaddress(1, coinbase_address)

        # Generating addresses to use as fee recipients (OMN holders)
        addresses = []
        for _ in range(6):
            addresses.append(node.getnewaddress())

        # Seeding with 50.00 OMNI
        txid = node.omni_send(address, addresses[1], 1, "50")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Seeding with 100.00 OMNI
        txid = node.omni_send(address, addresses[2], 1, "100")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Seeding with 150.00 OMNI
        txid = node.omni_send(address, addresses[3], 1, "150")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Seeding with 200.00 OMNI
        txid = node.omni_send(address, addresses[4], 1, "200")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending the fee system activation & checking it was valid...
        activation_block = node.getblockcount() + 8
        txid = node.omni_sendactivation(address, 9, activation_block, 999)
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        node.generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = node.omni_getactivations()['completedactivations']
        found = False
        for ids in featureid:
            if ids['featureid'] == 9:
                found = True
        assert_equal(found, True)

        # Checking share of fees for recipients...
        # Checking 5 percent share of fees...
        feeshare = node.omni_getfeeshare(addresses[1])
        assert_equal(feeshare[0]['feeshare'], "5.0000%")

        # Checking 10 percent share of fees...
        feeshare = node.omni_getfeeshare(addresses[2])
        assert_equal(feeshare[0]['feeshare'], "10.0000%")

        # Checking 15 percent share of fees...
        feeshare = node.omni_getfeeshare(addresses[3])
        assert_equal(feeshare[0]['feeshare'], "15.0000%")

        # Checking 20 percent share of fees...
        feeshare = node.omni_getfeeshare(addresses[4])
        assert_equal(feeshare[0]['feeshare'], "20.0000%")

        # Checking 50 percent share of fees...
        feeshare = node.omni_getfeeshare(address)
        assert_equal(feeshare[0]['feeshare'], "50.0000%")

        # Checking 100 percent share of fees...
        feeshare = node.omni_getfeeshare(address, 2)
        assert_equal(feeshare[0]['feeshare'], "100.0000%")

        # Testing a trade against self where the first token is OMNI
        txida = node.omni_sendtrade(address, 3, "2000", 1, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Testing a trade against self where the first token is OMNI
        txidb = node.omni_sendtrade(address, 1, "1.0", 3, "2000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Checking the original trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txida)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000000")

        # Checking the new trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txidb)
        assert_equal(result['matches'][0]['tradingfee'], "0")

        # Checking the fee cache is empty for property 1...
        result = node.omni_getfeecache(1)
        assert_equal(result[0]['cachedfees'], "0.00000000")

        # Checking the fee cache is empty for property 3...
        result = node.omni_getfeecache(3)
        assert_equal(result[0]['cachedfees'], "0")

        # Checking the trading address didn't lose any #1 tokens after trade...
        result = node.omni_getbalance(address, 1)
        assert_equal(result['balance'], "500.00000000")

        # Checking the trading address didn't lose any #3 tokens after trade...
        result = node.omni_getbalance(address, 3)
        assert_equal(result['balance'], "10000000")

        # Sending the all pair activation & checking it was valid...
        activation_block = node.getblockcount() + 8
        txid = node.omni_sendactivation(address, 8, activation_block, 999)
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        node.generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = node.omni_getactivations()['completedactivations']
        found = False
        for ids in featureid:
            if ids['featureid'] == 8:
                found = True
        assert_equal(found, True)

        # Testing a trade against self that results in a 1 willet fee for property 3 (1.0 #5 for 2000 #3)
        txida = node.omni_sendtrade(address, 3, "2000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        txidb = node.omni_sendtrade(address, 5, "1.0", 3, "2000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txidb)
        assert_equal(result['valid'], True)

        # Checking the original trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txida)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000000")

        # Checking the new trade matches to confirm trading fee was 1...
        result = node.omni_gettrade(txidb)
        assert_equal(result['matches'][0]['tradingfee'], "1")

        # Checking the fee cache now has 1 fee cached for property 3...
        result = node.omni_getfeecache(3)
        assert_equal(result[0]['cachedfees'], "1")

        # Checking the trading address now owns 9999999 of property 3...
        result = node.omni_getbalance(address, 3)
        assert_equal(result['balance'], "9999999")

        # Testing another trade against self that results in a 5 willet fee for property 3 (1.0 #5 for 10000 #3)
        txida = node.omni_sendtrade(address, 3, "10000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        txidb = node.omni_sendtrade(address, 5, "1.0", 3, "10000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txidb)
        assert_equal(result['valid'], True)

        # Checking the original trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txida)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000000")

        # Checking the new trade matches to confirm trading fee was 5...
        result = node.omni_gettrade(txidb)
        assert_equal(result['matches'][0]['tradingfee'], "5")

        # Checking the fee cache now has 6 fee cached for property 3...
        result = node.omni_getfeecache(3)
        assert_equal(result[0]['cachedfees'], "6")

        # Checking the trading address now owns 9999994 of property 3...
        result = node.omni_getbalance(address, 3)
        assert_equal(result['balance'], "9999994")

        # Testing a trade against self that results in a 1 willet fee for property 6 (1.0 #5 for 0.00002 #6)
        txida = node.omni_sendtrade(address, 6, "0.00002000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        txidb = node.omni_sendtrade(address, 5, "1.0", 6, "0.00002000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txidb)
        assert_equal(result['valid'], True)

        # Checking the original trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txida)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000000")

        # Checking the new trade matches to confirm trading fee was 0.00000001...
        result = node.omni_gettrade(txidb)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000001")

        # Checking the fee cache now has 0.00000001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000001")

        # Checking the trading address now owns 9999.99999999 of property 6...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.99999999")

        # Testing a trade against self that results in a 5000 willet fee for property 6 (1.0 #5 for 0.1 #6)
        txida = node.omni_sendtrade(address, 6, "0.1", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        txidb = node.omni_sendtrade(address, 5, "1.0", 6, "0.1")
        node.generatetoaddress(1, coinbase_address)

        # Checking the original trade matches to confirm trading fee was 0...
        result = node.omni_gettrade(txida)
        assert_equal(result['matches'][0]['tradingfee'], "0.00000000")

        # Checking the new trade matches to confirm trading fee was 0.00005000...
        result = node.omni_gettrade(txidb)
        assert_equal(result['matches'][0]['tradingfee'], "0.00005000")

        # Checking the fee cache now has 0.00005001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00005001")

        # Checking the trading address now owns 9999.99994999 of property 3...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.99994999")

        # Increasing volume to get close to 10000000 fee trigger point for property 6
        for x in range(5):
            result = node.omni_sendtrade(address, 6, "39.96", 5, "1.0")
            node.generatetoaddress(1, coinbase_address)

            # Checking the transaction was valid...
            result= node.omni_gettransaction(result)
            assert_equal(result['valid'], True)

            result = node.omni_sendtrade(address, 5, "1.0", 6, "39.96")
            node.generatetoaddress(1, coinbase_address)

            # Checking the transaction was valid...
            result = node.omni_gettransaction(result)
            assert_equal(result['valid'], True)

        # Checking the fee cache now has 0.09995001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.09995001")

        # Checking the trading address now owns 9999.90004999 of property 3...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.90004999")

        # Performing a small trade to take fee cache to 0.1 and trigger distribution for property 6
        txida = node.omni_sendtrade(address, 6, "0.09999999", 5, "0.8")
        txidb = node.omni_sendtrade(address, 5, "0.8", 6, "0.09999999")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txidb)
        assert_equal(result['valid'], True)

        # Checking distribution was triggered and the fee cache is now empty for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000000")

        # Checking received 0.00500000 fee share...
        result = node.omni_getbalance(addresses[1], 6)
        assert_equal(result['balance'], "0.00500000")

        # Checking received 0.01000000 fee share...
        result = node.omni_getbalance(addresses[2], 6)
        assert_equal(result['balance'], "0.01000000")

        # Checking received 0.01500000 fee share...
        result = node.omni_getbalance(addresses[3], 6)
        assert_equal(result['balance'], "0.01500000")

        # Checking received 0.02000000 fee share...
        result = node.omni_getbalance(addresses[4], 6)
        assert_equal(result['balance'], "0.02000000")

        # Checking received 0.05000000 fee share...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.95000000")

        # Rolling back the chain to test ability to roll back a distribution during reorg (disconnecting 1 block from tip and mining a replacement)
        block = node.getblockcount()
        blockhash = node.getblockhash(block)
        node.invalidateblock(blockhash)
        prevblock = node.getblockcount()

        # Clearing the mempool
        node.clearmempool()

        # Checking the block count has been reduced by 1...
        expblock = block - 1
        assert_equal(expblock, prevblock)

        # Mining a replacement block
        node.generatetoaddress(1, coinbase_address)

        # Verifiying the results
        newblock = node.getblockcount()
        newblockhash = node.getblockhash(newblock)

        # Checking the block count is the same as before the rollback...
        assert_equal(block, newblock)

        # Checking the block hash is different from before the rollback...
        assert_equal(blockhash == newblockhash, False)

        # Checking the fee cache now again has 0.09995001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.09995001")

        # Checking balance has been rolled back to 0...
        result = node.omni_getbalance(addresses[1], 6)
        assert_equal(result['balance'], "0.00000000")

        # Checking balance has been rolled back to 0...
        result = node.omni_getbalance(addresses[2], 6)
        assert_equal(result['balance'], "0.00000000")

        # Checking balance has been rolled back to 0...
        result = node.omni_getbalance(addresses[3], 6)
        assert_equal(result['balance'], "0.00000000")

        # Checking balance has been rolled back to 0...
        result = node.omni_getbalance(addresses[4], 6)
        assert_equal(result['balance'], "0.00000000")

        # Checking balance has been rolled back to 9999.90004999...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.90004999")

        # Performing a small trade to take fee cache to 0.1 and retrigger distribution for property 6
        txida = node.omni_sendtrade(address, 6, "0.09999999", 5, "0.8")
        txidb = node.omni_sendtrade(address, 5, "0.8", 6, "0.09999999")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txida)
        assert_equal(result['valid'], True)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(txidb)
        assert_equal(result['valid'], True)

        # Checking received 0.00500000 fee share...
        result = node.omni_getbalance(addresses[1], 6)
        assert_equal(result['balance'], "0.00500000")

        # Checking received 0.01000000 fee share...
        result = node.omni_getbalance(addresses[2], 6)
        assert_equal(result['balance'], "0.01000000")

        # Checking received 0.01500000 fee share...
        result = node.omni_getbalance(addresses[3], 6)
        assert_equal(result['balance'], "0.01500000")

        # Checking received 0.02000000 fee share...
        result = node.omni_getbalance(addresses[4], 6)
        assert_equal(result['balance'], "0.02000000")

        # Checking received 0.05000000 fee share...
        result = node.omni_getbalance(address, 6)
        assert_equal(result['balance'], "9999.95000000")

        # Rolling back the chain to test ability to roll back a fee cache change during reorg
        # Testing a trade against self that results in a 1 willet fee for property 6 (1.0 #6 for 0.00002 #5)
        result = node.omni_sendtrade(address, 6, "0.00002000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        result = node.omni_sendtrade(address, 5, "1.0", 6, "0.00002000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        # Checking the fee cache now has 0.00000001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000001")

        # Testing another trade against self that results in a 1 willet fee for property 6 (1.0 #6 for 0.00002 #5)
        result = node.omni_sendtrade(address, 6, "0.00002000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        result = node.omni_sendtrade(address, 5, "1.0", 6, "0.00002000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        # Checking the fee cache now has 0.00000002 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000002")

        # Rolling back the chain to orphan a block (disconnecting 1 block from tip and mining a replacement)
        block = node.getblockcount()
        blockhash = node.getblockhash(block)
        node.invalidateblock(blockhash)
        prevblock = node.getblockcount()

        # Clearing the mempool
        node.clearmempool()

        # Checking the block count has been reduced by 1...
        expblock = block - 1
        assert_equal(expblock, prevblock)

        # Mining a replacement block
        node.generatetoaddress(1, coinbase_address)

        # Verifiying the results
        newblock = node.getblockcount()
        newblockhash = node.getblockhash(newblock)

        # Checking the block count is the same as before the rollback...
        assert_equal(block, newblock)

        # Checking the block hash is different from before the rollback...
        assert_equal(blockhash == newblockhash, False)

        # Checking the fee cache now again has 0.09995001 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000001")

        # Mining 51 blocks to test that fee cache is not affected by fee pruning
        node.generatetoaddress(51, coinbase_address)

        # Executing a trade to generate 1 willet fee
        result = node.omni_sendtrade(address, 6, "0.00002000", 5, "1.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        result = node.omni_sendtrade(address, 5, "1.0", 6, "0.00002000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        # Checking the fee cache now has 0.00000002 fee cached for property 6...
        result = node.omni_getfeecache(6)
        assert_equal(result[0]['cachedfees'], "0.00000002")

        # Adding some test ecosystem volume to trigger distribution
        for x in range(9):
            result = node.omni_sendtrade(address, 2147483651, "20000", 2147483652, "10.0")
            node.generatetoaddress(1, coinbase_address)

            # Checking the transaction was valid...
            result = node.omni_gettransaction(result)
            assert_equal(result['valid'], True)

            result = node.omni_sendtrade(address, 2147483652, "10.0", 2147483651, "20000")
            node.generatetoaddress(1, coinbase_address)

            # Checking the transaction was valid...
            result = node.omni_gettransaction(result)
            assert_equal(result['valid'], True)

        # Checking the fee cache now has 90 fee cached for property 2147483651...
        result = node.omni_getfeecache(2147483651)
        assert_equal(result[0]['cachedfees'], "90")

        # Checking the trading address now owns 9999910 of property 2147483651...
        result = node.omni_getbalance(address, 2147483651)
        assert_equal(result['balance'], "9999910")

        # Triggering distribution in the test ecosystem for property 2147483651
        result = node.omni_sendtrade(address, 2147483651, "20000", 2147483652, "10.0")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        result = node.omni_sendtrade(address, 2147483652, "10.0", 2147483651, "20000")
        node.generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = node.omni_gettransaction(result)
        assert_equal(result['valid'], True)

        # Checking distribution was triggered and the fee cache is now empty for property 2147483651...
        result = node.omni_getfeecache(2147483651)
        assert_equal(result[0]['cachedfees'], "0")

        # Checking received 100 fee share...
        result = node.omni_getbalance(address, 2147483651)
        assert_equal(result['balance'], "10000000")

if __name__ == '__main__':
    OmniFeeCache().main()
