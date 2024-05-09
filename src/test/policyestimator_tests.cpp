// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <policy/policy.h>
#include <kernel/mempool_entry.h>
#include <test/util/random.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/time.h>
#include <validationinterface.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(policyestimator_tests, ChainTestingSetup)

static inline CTransactionRef make_tx(const std::vector<COutPoint>& outpoints, int num_outputs = 1)
{
    CMutableTransaction tx;
    tx.vin.reserve(outpoints.size());
    tx.vout.resize(num_outputs);
    for (const auto& outpoint : outpoints) {
        tx.vin.emplace_back(outpoint);
    }
    for (int i = 0; i < num_outputs; ++i) {
        auto scriptPubKey = CScript() << OP_TRUE;
        tx.vout.emplace_back(COIN, scriptPubKey);
    }
    return MakeTransactionRef(tx);
}

void verifyTxsInclusion(const std::vector<Txid>& txs_vec, const std::set<Txid>& txs_set)
{
    BOOST_CHECK(txs_vec.size() == txs_set.size());
    for (const auto txid : txs_vec) {
        BOOST_CHECK(txs_set.find(txid) != txs_set.end());
    }
}

void validateTxRelation(const Txid& txid, const std::vector<Txid>& ancestors, const std::vector<Txid>& descendants, const TxAncestorsAndDescendants& txAncestorsAndDescendants)
{
    const auto iter = txAncestorsAndDescendants.find(txid);
    BOOST_CHECK(iter != txAncestorsAndDescendants.end());
    const auto calc_ancestors = iter->second.first;
    verifyTxsInclusion(ancestors, calc_ancestors);
    const auto calc_descendants = iter->second.second;
    verifyTxsInclusion(descendants, calc_descendants);
}

BOOST_AUTO_TEST_CASE(ComputingTxAncestorsAndDescendants)
{
    TestMemPoolEntryHelper entry;
    auto create_and_add_tx = [&](std::vector<RemovedMempoolTransactionInfo>& transactions, const std::vector<COutPoint>& outpoints, int num_outputs = 1) {
        const CTransactionRef tx = make_tx(outpoints, num_outputs);
        transactions.emplace_back(entry.FromTx(tx));
        return tx;
    };

    // Test 20 unique transactions
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(20);

        for (auto i = 0; i < 20; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints);
        }

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        for (auto& tx : transactions) {
            const Txid txid = tx.info.m_tx->GetHash();
            const auto txiter = txAncestorsAndDescendants.find(txid);
            BOOST_CHECK(txiter != txAncestorsAndDescendants.end());
            const auto ancestors = txiter->second.first;
            const auto descendants = txiter->second.second;
            BOOST_CHECK(ancestors.size() == 1);
            BOOST_CHECK(descendants.size() == 1);
        }
    }
    // Test 3 Linear transactions clusters
    /*
            Linear Packages
            A     B     C    D
            |     |     |    |
            E     H     J    K
            |     |
            F     I
            |
            G
    */
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(11);

        // Create transaction A B C D
        for (auto i = 0; i < 4; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints);
        }

        // Create cluster A children ---> E->F->G
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        for (auto i = 0; i < 3; ++i) {
            const CTransactionRef tx = create_and_add_tx(transactions, outpoints);
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster B children ---> H->I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        for (size_t i = 0; i < 2; ++i) {
            const CTransactionRef tx = create_and_add_tx(transactions, outpoints);
            outpoints = {COutPoint(tx->GetHash(), 0)};
        }

        // Create cluster C child ---> J
        outpoints = {COutPoint(transactions[2].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create cluster D child ---> K
        outpoints = {COutPoint(transactions[3].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            std::vector<Txid> descendants{txA_Id};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txA_Id, std::vector<Txid>{txA_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx G topology;
        {
            const Txid txG_Id = transactions[6].info.m_tx->GetHash();
            std::vector<Txid> ancestors{transactions[0].info.m_tx->GetHash()};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                ancestors.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txG_Id, ancestors, std::vector<Txid>{txG_Id}, txAncestorsAndDescendants);
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            std::vector<Txid> descendants{txB_Id};
            for (auto i = 7; i <= 8; i++) { // Add H, I
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txB_Id, std::vector<Txid>{txB_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx H topology;
        {
            const Txid txH_Id = transactions[7].info.m_tx->GetHash();
            const std::vector<Txid> ancestors{txH_Id, transactions[1].info.m_tx->GetHash()};   // H, B
            const std::vector<Txid> descendants{txH_Id, transactions[8].info.m_tx->GetHash()}; // H, I
            validateTxRelation(txH_Id, ancestors, descendants, txAncestorsAndDescendants);
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            const std::vector<Txid> descendants{txC_Id, transactions[9].info.m_tx->GetHash()}; // C, J
            validateTxRelation(txC_Id, std::vector<Txid>{txC_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx D topology;
        {
            const Txid txD_Id = transactions[3].info.m_tx->GetHash();
            const std::vector<Txid> descendants{txD_Id, transactions[10].info.m_tx->GetHash()}; // D, K
            validateTxRelation(txD_Id, std::vector<Txid>{txD_Id}, descendants, txAncestorsAndDescendants);
        }
    }

    /* Unique transactions with a cluster of transactions

           Cluster A                      Cluster B
              A                               B
            /   \                           /   \
           /     \                         /     \
          C       D                       I       J
        /   \     |                               |
       /     \    |                               |
      E       F   H                               K
      \       /
       \     /
          G
    */
    {
        std::vector<RemovedMempoolTransactionInfo> transactions;
        transactions.reserve(11);

        // Create transaction A and B
        for (auto i = 0; i < 2; ++i) {
            const std::vector<COutPoint> outpoints{COutPoint(Txid::FromUint256(m_rng.rand256()), 0)};
            create_and_add_tx(transactions, outpoints, /*num_outputs=*/2);
        }

        // Cluster 1 Topology
        // Create a child for A ---> C
        std::vector<COutPoint> outpoints{COutPoint(transactions[0].info.m_tx->GetHash(), 0)};
        const CTransactionRef tx_C = create_and_add_tx(transactions, outpoints, /*num_outputs=*/2);

        // Create a child for A ---> D
        outpoints = {COutPoint(transactions[0].info.m_tx->GetHash(), 1)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for C ---> E
        outpoints = {COutPoint(tx_C->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for C ---> F
        outpoints = {COutPoint(tx_C->GetHash(), 1)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for E and F  ---> G
        outpoints = {COutPoint(transactions[4].info.m_tx->GetHash(), 0), COutPoint(transactions[5].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for D ---> H
        outpoints = {COutPoint(transactions[3].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Cluster 2
        // Create a child for B ---> I
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        // Create a child for B ---> J
        outpoints = {COutPoint(transactions[1].info.m_tx->GetHash(), 1)};
        const CTransactionRef tx_J = create_and_add_tx(transactions, outpoints);

        // Create a child for J ---> K
        outpoints = {COutPoint(tx_J->GetHash(), 0)};
        create_and_add_tx(transactions, outpoints);

        const auto txAncestorsAndDescendants = GetTxAncestorsAndDescendants(transactions);
        BOOST_CHECK_EQUAL(txAncestorsAndDescendants.size(), transactions.size());

        // Check tx A topology;
        {
            const Txid txA_Id = transactions[0].info.m_tx->GetHash();
            std::vector<Txid> descendants{txA_Id};
            for (auto i = 2; i <= 7; i++) { // Add  C, D, E, F, G, H
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txA_Id, std::vector<Txid>{txA_Id}, descendants, txAncestorsAndDescendants);
        }
        // Check tx C topology;
        {
            const Txid txC_Id = transactions[2].info.m_tx->GetHash();
            std::vector<Txid> ancestors{txC_Id, transactions[0].info.m_tx->GetHash()}; // C, A
            std::vector<Txid> descendants{txC_Id};
            for (auto i = 4; i <= 6; i++) { // Add E, F, G
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txC_Id, ancestors, descendants, txAncestorsAndDescendants);
        }
        // Check tx B topology;
        {
            const Txid txB_Id = transactions[1].info.m_tx->GetHash();
            std::vector<Txid> descendants{txB_Id};
            for (auto i = 8; i <= 10; i++) { // Add I, J, K
                descendants.emplace_back(transactions[i].info.m_tx->GetHash());
            }
            validateTxRelation(txB_Id, std::vector<Txid>{txB_Id}, descendants, txAncestorsAndDescendants);
        }
    }
}

BOOST_AUTO_TEST_CASE(BlockPolicyEstimates)
{
    CBlockPolicyEstimator feeEst{FeeestPath(*m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    CTxMemPool& mpool = *Assert(m_node.mempool);
    m_node.validation_signals->RegisterValidationInterface(&feeEst);
    TestMemPoolEntryHelper entry;
    CAmount basefee(2000);
    CAmount deltaFee(100);
    std::vector<CAmount> feeV;
    feeV.reserve(10);

    // Populate vectors of increasing fees
    for (int j = 0; j < 10; j++) {
        feeV.push_back(basefee * (j+1));
    }

    // Store the hashes of transactions that have been
    // added to the mempool by their associate fee
    // txHashes[j] is populated with transactions either of
    // fee = basefee * (j+1)
    std::vector<Txid> txHashes[10];

    // Create a transaction template
    CScript garbage;
    for (unsigned int i = 0; i < 128; i++)
        garbage.push_back('X');
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = garbage;
    tx.vout.resize(1);
    tx.vout[0].nValue=0LL;
    CFeeRate baseRate(basefee, GetVirtualTransactionSize(CTransaction(tx)));

    // Create a fake block
    std::vector<CTransactionRef> block;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .9952 and 4 fee transactions per block
    // This makes the tx count about 2.5 per bucket, well above the 0.1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                {
                    LOCK2(cs_main, mpool.cs);
                    AddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not AddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                txHashes[j].push_back(tx.GetHash());
            }
        }
        //Create blocks where higher fee txs are included more often
        for (int h = 0; h <= blocknum%10; h++) {
            // 10/10 blocks add highest fee transactions
            // 9/10 blocks add 2nd highest and so on until ...
            // 1/10 blocks add lowest fee transactions
            while (txHashes[9-h].size()) {
                CTransactionRef ptx = mpool.get(txHashes[9-h].back());
                if (ptx)
                    block.push_back(ptx);
                txHashes[9-h].pop_back();
            }
        }

        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }

        block.clear();
        // Check after just a few txs that combining buckets works as expected
        if (blocknum == 3) {
            // Wait for fee estimator to catch up
            m_node.validation_signals->SyncWithValidationInterfaceQueue();
            // At this point we should need to combine 3 buckets to get enough data points
            // So estimateFee(1) should fail and estimateFee(2) should return somewhere around
            // 9*baserate.  estimateFee(2) %'s are 100,100,90 = average 97%
            BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() < 9*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() > 9*baseRate.GetFeePerK() - deltaFee);
        }
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    std::vector<CAmount> origFeeEst;
    // Highest feerate is 10*baseRate and gets in all blocks,
    // second highest feerate is 9*baseRate and gets in 9/10 blocks = 90%,
    // third highest feerate is 8*base rate, and gets in 8/10 blocks = 80%,
    // so estimateFee(1) would return 10*baseRate but is hardcoded to return failure
    // Second highest feerate has 100% chance of being included by 2 blocks,
    // so estimateFee(2) should return 9*baseRate etc...
    for (int i = 1; i < 10;i++) {
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
        if (i > 2) { // Fee estimates should be monotonically decreasing
            BOOST_CHECK(origFeeEst[i-1] <= origFeeEst[i-2]);
        }
        int mult = 11-i;
        if (i % 2 == 0) { //At scale 2, test logic is only correct for even targets
            BOOST_CHECK(origFeeEst[i-1] < mult*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(origFeeEst[i-1] > mult*baseRate.GetFeePerK() - deltaFee);
        }
    }
    // Fill out rest of the original estimates
    for (int i = 10; i <= 48; i++) {
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
    }

    // Mine 50 more blocks with no transactions happening, estimates shouldn't change
    // We haven't decayed the moving average enough so we still have enough data points in every bucket
    while (blocknum < 250) {
        LOCK(mpool.cs);
        mpool.removeForBlock(block, ++blocknum);
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] + deltaFee);
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }


    // Mine 15 more blocks with lots of transactions happening and not getting mined
    // Estimates should go up
    while (blocknum < 265) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                {
                    LOCK2(cs_main, mpool.cs);
                    AddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not AddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                txHashes[j].push_back(tx.GetHash());
            }
        }
        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine all those transactions
    // Estimates should still not be below original
    for (int j = 0; j < 10; j++) {
        while(txHashes[j].size()) {
            CTransactionRef ptx = mpool.get(txHashes[j].back());
            if (ptx)
                block.push_back(ptx);
            txHashes[j].pop_back();
        }
    }

    {
        LOCK(mpool.cs);
        mpool.removeForBlock(block, 266);
    }
    block.clear();

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine 400 more blocks where everything is mined every block
    // Estimates should be below original estimates
    while (blocknum < 665) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                {
                    LOCK2(cs_main, mpool.cs);
                    AddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not AddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                CTransactionRef ptx = mpool.get(tx.GetHash());
                if (ptx)
                    block.push_back(ptx);

            }
        }

        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }

        block.clear();
    }
    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 9; i++) { // At 9, the original estimate was already at the bottom (b/c scale = 2)
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
    }
}

BOOST_AUTO_TEST_SUITE_END()
