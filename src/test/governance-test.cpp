// Copyright (c) 2014-2018 Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "masternode-budget.h"
#include "masternodeman.h"

std::ostream& operator<<(std::ostream& os, uint256 value)
{
    return os << value.ToString();
}

std::ostream& operator<<(std::ostream& os, const CTxIn& value)
{
    return os << value.ToString();
}

std::ostream& operator<<(std::ostream& os, const CTxOut& value)
{
    return os << value.ToString();
}

std::ostream& operator<<(std::ostream& os, const CTxBudgetPayment& value)
{
    return os << "{" << value.nProposalHash.ToString() << ":" << value.nAmount << "@" << value.payee.ToString() << "}";
}

std::ostream& operator<<(std::ostream& os, const CBudgetProposal& value)
{
    return os << "{"
        << value.GetName() << ", "
        << value.GetURL() << ", "
        << "[" << value.GetBlockStart() << "," << value.GetBlockEnd() << "]" << ", "
        << value.GetAmount() << "->" << value.GetPayee().ToString()
        << "}";
}

bool operator == (const CBudgetProposal& a, const CBudgetProposal& b)
{
    return a.GetHash() == b.GetHash();
}

bool operator == (const CTxBudgetPayment& a, const CTxBudgetPayment& b)
{
    return a.nProposalHash == b.nProposalHash &&
           a.nAmount == b.nAmount &&
           a.payee == b.payee;
}

bool operator != (const CTxBudgetPayment& a, const CTxBudgetPayment& b)
{
    return !(a == b);
}

namespace
{
    CKey CreateKeyPair(const unsigned char privKey[32])
    {
        CKey keyPair;
        keyPair.Set(privKey, privKey + 32, true);

        return keyPair;
    }

    void FillBlock(CBlockIndex& block, /*const*/ CBlockIndex* prevBlock, const uint256& hash)
    {
        if (prevBlock)
        {
            block.nHeight = prevBlock->nHeight + 1;
            block.pprev = prevBlock;
        }
        else
        {
            block.nHeight = 0;
        }

        block.phashBlock = &hash;
        block.nTime = GetTime();
        block.BuildSkip();
    }

    void FillHash(uint256& hash, const arith_uint256& height)
    {
        hash = ArithToUint256(height);
    }

    void FillBlock(CBlockIndex& block, uint256& hash, /*const*/ CBlockIndex* prevBlock, size_t height)
    {
        FillHash(hash, height);
        FillBlock(block, height ? prevBlock : NULL, hash);

        assert(static_cast<int>(UintToArith256(block.GetBlockHash()).GetLow64()) == block.nHeight);
        assert(block.pprev == NULL || block.nHeight == block.pprev->nHeight + 1);

    }

    CScript PayToPublicKey(const CPubKey& pubKey)
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(pubKey.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    CMasternode CreateMasternode(CTxIn vin, const CKey* signKey = NULL)
    {
        CMasternode mn;
        mn.vin = vin;
        mn.activeState = CMasternode::MASTERNODE_ENABLED;
        if (signKey != NULL)
            mn.pubkey2 = signKey->GetPubKey();
        return mn;
    }

    CTxBudgetPayment GetPayment(const CBudgetProposal& proposal)
    {
        return CTxBudgetPayment(proposal.GetHash(), proposal.GetPayee(), proposal.GetAmount());
    }


    const unsigned char vchKey0[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    const unsigned char vchKey1[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0};
    const unsigned char vchKey2[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0};
    const unsigned char vchKey3[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0};

    struct FinalizedBudgetFixture
    {
        const std::string budgetName;
        const int blockStart;

        const CKey keyPairA;
        const CKey keyPairB;
        const CKey keyPairC;
        const CKey keyPairMn;

        const CBudgetProposal proposalA;
        const CBudgetProposal proposalB;
        const CBudgetProposal proposalC;

        const CMasternode mn1;
        const CMasternode mn2;
        const CMasternode mn3;
        const CMasternode mn4;
        const CMasternode mn5;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;
        std::string error;

        static boost::scoped_ptr<ECCVerifyHandle> globalVerifyHandle;

        FinalizedBudgetFixture()
            : budgetName("test")
            , blockStart(1296000)
            , keyPairA(CreateKeyPair(vchKey0))
            , keyPairB(CreateKeyPair(vchKey1))
            , keyPairC(CreateKeyPair(vchKey2))
            , keyPairMn(CreateKeyPair(vchKey3))
            , proposalA(CreateProposal("A", keyPairA, 42))
            , proposalB(CreateProposal("B", keyPairB, 404))
            , proposalC(CreateProposal("C", keyPairC, 42))
            , mn1(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN)), &keyPairMn))
            , mn2(CreateMasternode(CTxIn(COutPoint(ArithToUint256(2), 1 * COIN))))
            , mn3(CreateMasternode(CTxIn(COutPoint(ArithToUint256(3), 1 * COIN))))
            , mn4(CreateMasternode(CTxIn(COutPoint(ArithToUint256(4), 1 * COIN))))
            , mn5(CreateMasternode(CTxIn(COutPoint(ArithToUint256(5), 1 * COIN))))
            , hashes(1290500)
            , blocks(1290500)
        {
            globalVerifyHandle.reset(new ECCVerifyHandle());

            SetMockTime(GetTime());

            fMasterNode = true;

            // Build a main chain 100500 blocks long.
            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks.back());

            mnodeman.Add(mn1);
            mnodeman.Add(mn2);
            mnodeman.Add(mn3);
            mnodeman.Add(mn4);
            mnodeman.Add(mn5);
        }

        ~FinalizedBudgetFixture()
        {
            SetMockTime(0);

            mnodeman.Clear();
            budget.Clear();
            chainActive = CChain();
        }

        CBudgetProposal CreateProposal(std::string name, CKey payee, CAmount amount)
        {
            CBudgetProposal p(
                name,
                "",
                blockStart,
                blockStart + GetBudgetPaymentCycleBlocks() * 2,
                PayToPublicKey(payee.GetPubKey()),
                amount * COIN,
                uint256()
            );
            p.nTime = GetTime();
            return p;
        }

    };
    boost::scoped_ptr<ECCVerifyHandle> FinalizedBudgetFixture::globalVerifyHandle;
}


BOOST_FIXTURE_TEST_SUITE(FinalizedBudget, FinalizedBudgetFixture)

    BOOST_AUTO_TEST_CASE(CompareHash_Equal)
    {
        // Set Up
        std::vector<CTxBudgetPayment> payments1;
        payments1.push_back(GetPayment(proposalA));
        payments1.push_back(GetPayment(proposalB));
        payments1.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget1(
            budgetName, 
            blockStart, 
            payments1,
            ArithToUint256(1)
        );

        std::vector<CTxBudgetPayment> payments2;
        payments2.push_back(GetPayment(proposalA));
        payments2.push_back(GetPayment(proposalB));
        payments2.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget2(
            budgetName,
            blockStart,
            payments2,
            ArithToUint256(2)
        );

        // Call & Check
        BOOST_CHECK_EQUAL(budget1.GetHash(), budget2.GetHash());
    }

    BOOST_AUTO_TEST_CASE(CompareHash_DifferentName)
    {
        // Set Up
        std::vector<CTxBudgetPayment> payments1;
        payments1.push_back(GetPayment(proposalA));
        payments1.push_back(GetPayment(proposalB));
        payments1.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget1(
            budgetName,
            blockStart,
            payments1,
            ArithToUint256(1)
        );

        std::vector<CTxBudgetPayment> payments2;
        payments2.push_back(GetPayment(proposalA));
        payments2.push_back(GetPayment(proposalB));
        payments2.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget2(
            "he-who-must-not-be-named",
            blockStart,
            payments2,
            ArithToUint256(2)
        );

        // Call & Check
        BOOST_CHECK_EQUAL(budget1.GetHash(), budget2.GetHash());
    }

    BOOST_AUTO_TEST_CASE(CompareHash_DifferentSet)
    {
        // Set Up
        std::vector<CTxBudgetPayment> payments1;
        payments1.push_back(GetPayment(proposalA));
        payments1.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget1(
            budgetName,
            blockStart,
            payments1,
            ArithToUint256(1)
        );

        std::vector<CTxBudgetPayment> payments2;
        payments2.push_back(GetPayment(proposalA));
        payments2.push_back(GetPayment(proposalB));
        CFinalizedBudgetBroadcast budget2(
            budgetName,
            blockStart,
            payments2,
            ArithToUint256(2)
        );

        // Call & Check
        BOOST_CHECK(budget1.GetHash() != budget2.GetHash());
    }

    BOOST_AUTO_TEST_CASE(CompareHash_DifferentOrder)
    {
        // Set Up
        std::vector<CTxBudgetPayment> payments1;
        payments1.push_back(GetPayment(proposalA));
        payments1.push_back(GetPayment(proposalB));
        payments1.push_back(GetPayment(proposalC));
        CFinalizedBudgetBroadcast budget1(
            budgetName,
            blockStart,
            payments1,
            ArithToUint256(1)
        );

        std::vector<CTxBudgetPayment> payments2;
        payments2.push_back(GetPayment(proposalB));
        payments2.push_back(GetPayment(proposalC));
        payments2.push_back(GetPayment(proposalA));
        CFinalizedBudgetBroadcast budget2(
            budgetName,
            blockStart,
            payments2,
            ArithToUint256(2)
        );

        // Call & Check
        BOOST_CHECK_EQUAL(budget1.GetHash(), budget2.GetHash());
    }
    
    BOOST_AUTO_TEST_CASE(AutoCheck_OneProposal)
    {
        // Set Up
        // Submitting proposals
        budget.AddProposal(proposalA, false); // false = don't check collateral

        // Voting for proposals
        CBudgetVote vote1a(mn1.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote2a(mn2.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote3a(mn3.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote4a(mn4.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote5a(mn5.vin, proposalA.GetHash(), VOTE_YES);

        budget.SubmitProposalVote(vote1a, error);
        budget.SubmitProposalVote(vote2a, error);
        budget.SubmitProposalVote(vote3a, error);
        budget.SubmitProposalVote(vote4a, error);
        budget.SubmitProposalVote(vote5a, error);

        // Finalizing budget
        SetMockTime(GetTime() + 24 * 60 * 60 + 1); // 1 hour + 1 second has passed

        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        BOOST_REQUIRE(budget.fValid);
        BOOST_REQUIRE(budget.IsValid(false));
        BOOST_REQUIRE_EQUAL(budget.IsAutoChecked(), false);

        // Call & Check
        bool result = budget.AutoCheck();

        BOOST_CHECK_EQUAL(budget.IsAutoChecked(), true);
        BOOST_CHECK(result);
    }

    BOOST_AUTO_TEST_CASE(AutoCheck_ThreeProposals_ABC)
    {
        // Set Up
        // Submitting proposals
        budget.AddProposal(proposalA, false); // false = don't check collateral
        budget.AddProposal(proposalB, false);
        budget.AddProposal(proposalC, false);

        // Voting for proposals
        CBudgetVote vote1a(mn1.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote2a(mn2.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote3a(mn3.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote4a(mn4.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote5a(mn5.vin, proposalA.GetHash(), VOTE_YES);

        CBudgetVote vote1b(mn1.vin, proposalB.GetHash(), VOTE_YES);
        CBudgetVote vote2b(mn2.vin, proposalB.GetHash(), VOTE_YES);
        CBudgetVote vote3b(mn3.vin, proposalB.GetHash(), VOTE_YES);

        CBudgetVote vote1c(mn1.vin, proposalC.GetHash(), VOTE_YES);
        CBudgetVote vote2c(mn2.vin, proposalC.GetHash(), VOTE_YES);

        budget.SubmitProposalVote(vote1a, error);
        budget.SubmitProposalVote(vote2a, error);
        budget.SubmitProposalVote(vote3a, error);
        budget.SubmitProposalVote(vote4a, error);
        budget.SubmitProposalVote(vote5a, error);

        budget.SubmitProposalVote(vote1b, error);
        budget.SubmitProposalVote(vote2b, error);
        budget.SubmitProposalVote(vote3b, error);

        budget.SubmitProposalVote(vote1c, error);
        budget.SubmitProposalVote(vote2c, error);

        // Finalizing budget
        SetMockTime(GetTime() + 24 * 60 * 60 + 1); // 1 hour + 1 second has passed

        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));
        txBudgetPayments.push_back(GetPayment(proposalC));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        BOOST_REQUIRE(budget.fValid);
        BOOST_REQUIRE(budget.IsValid(false));
        BOOST_REQUIRE_EQUAL(budget.IsAutoChecked(), false);

        // Call & Check
        bool result = budget.AutoCheck();

        BOOST_CHECK_EQUAL(budget.IsAutoChecked(), true);
        BOOST_CHECK(result);
    }

    BOOST_AUTO_TEST_CASE(AutoCheck_ThreeProposals_CBA)
    {
        // Set Up
        // Submitting proposals
        budget.AddProposal(proposalA, false); // false = don't check collateral
        budget.AddProposal(proposalB, false);
        budget.AddProposal(proposalC, false);

        // Voting for proposals
        CBudgetVote vote1c(mn1.vin, proposalC.GetHash(), VOTE_YES);
        CBudgetVote vote2c(mn2.vin, proposalC.GetHash(), VOTE_YES);
        CBudgetVote vote3c(mn3.vin, proposalC.GetHash(), VOTE_YES);
        CBudgetVote vote4c(mn4.vin, proposalC.GetHash(), VOTE_YES);
        CBudgetVote vote5c(mn5.vin, proposalC.GetHash(), VOTE_YES);

        CBudgetVote vote1b(mn1.vin, proposalB.GetHash(), VOTE_YES);
        CBudgetVote vote2b(mn2.vin, proposalB.GetHash(), VOTE_YES);
        CBudgetVote vote3b(mn3.vin, proposalB.GetHash(), VOTE_YES);

        CBudgetVote vote1a(mn1.vin, proposalA.GetHash(), VOTE_YES);
        CBudgetVote vote2a(mn2.vin, proposalA.GetHash(), VOTE_YES);

        budget.SubmitProposalVote(vote1c, error);
        budget.SubmitProposalVote(vote2c, error);
        budget.SubmitProposalVote(vote3c, error);
        budget.SubmitProposalVote(vote4c, error);
        budget.SubmitProposalVote(vote5c, error);

        budget.SubmitProposalVote(vote1b, error);
        budget.SubmitProposalVote(vote2b, error);
        budget.SubmitProposalVote(vote3b, error);

        budget.SubmitProposalVote(vote1a, error);
        budget.SubmitProposalVote(vote2a, error);

        // Finalizing budget
        SetMockTime(GetTime() + 24 * 60 * 60 + 1); // 1 hour + 1 second has passed

        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalC));
        txBudgetPayments.push_back(GetPayment(proposalB));
        txBudgetPayments.push_back(GetPayment(proposalA));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        BOOST_REQUIRE(budget.fValid);
        BOOST_REQUIRE(budget.IsValid(false));
        BOOST_REQUIRE_EQUAL(budget.IsAutoChecked(), false);

        // Call & Check
        bool result = budget.AutoCheck();

        BOOST_CHECK_EQUAL(budget.IsAutoChecked(), true);
        BOOST_CHECK(result);
    }

    BOOST_AUTO_TEST_CASE(IsTransactionValid_Block0)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        CMutableTransaction expected;
        expected.vout.push_back(CTxOut(proposalB.GetAmount(), proposalB.GetPayee()));
        expected.vout.push_back(CTxOut(proposalA.GetAmount(), proposalA.GetPayee()));

        // Call & Check
        BOOST_CHECK(budget.IsTransactionValid(expected, blockStart));
    }

    BOOST_AUTO_TEST_CASE(IsTransactionValid_Block0_Invalid_MissingPayment)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        CMutableTransaction wrong1;
        wrong1.vout.push_back(CTxOut(proposalB.GetAmount(), proposalB.GetPayee()));

        CMutableTransaction wrong2;
        wrong2.vout.push_back(CTxOut(proposalA.GetAmount(), proposalA.GetPayee()));

        // Call & Check
        BOOST_CHECK(!budget.IsTransactionValid(wrong1, blockStart));
        BOOST_CHECK(!budget.IsTransactionValid(wrong2, blockStart));
    }

    BOOST_AUTO_TEST_CASE(IsTransactionValid_Block0_Invalid_WrongPayment)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        CMutableTransaction wrong1;
        wrong1.vout.push_back(CTxOut(proposalA.GetAmount(), proposalC.GetPayee()));

        CMutableTransaction wrong2;
        wrong2.vout.push_back(CTxOut(proposalB.GetAmount(), proposalA.GetPayee()));

        // Call & Check
        BOOST_CHECK(!budget.IsTransactionValid(wrong1, blockStart));
        BOOST_CHECK(!budget.IsTransactionValid(wrong2, blockStart));
    }

    BOOST_AUTO_TEST_CASE(IsTransactionValid_Block1)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        CMutableTransaction wrong1;
        wrong1.vout.push_back(CTxOut(proposalA.GetAmount(), proposalA.GetPayee()));

        CMutableTransaction wrong2;
        wrong2.vout.push_back(CTxOut(proposalB.GetAmount(), proposalB.GetPayee()));

        // Call & Check
        BOOST_CHECK(!budget.IsTransactionValid(wrong1, blockStart + 1));
        BOOST_CHECK(!budget.IsTransactionValid(wrong2, blockStart + 1));
    }

    BOOST_AUTO_TEST_CASE(GetBudgetPayments)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, ArithToUint256(42));

        std::vector<CTxBudgetPayment> expected;
        expected.push_back(GetPayment(proposalB));
        expected.push_back(GetPayment(proposalA));

        // Call & Check
        std::vector<CTxBudgetPayment> actual = budget.GetBudgetPayments();

        BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    }

    BOOST_AUTO_TEST_CASE(CreateFinalizedBudgetSignedByMasternode)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        BOOST_REQUIRE(keyPairMn.GetPubKey() == mn1.pubkey2);
        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        // Call & Check
        BOOST_CHECK(budget.IsValid());
        BOOST_CHECK_EQUAL(budget.MasternodeSubmittedId(), mn1.vin);
    }

    BOOST_AUTO_TEST_CASE(CreateFinalizedBudgetSignedWithWrongKey)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        BOOST_REQUIRE(keyPairMn.GetPubKey() != mn2.pubkey2);
        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, mn2.vin, keyPairMn);

        // Call & Check
        BOOST_CHECK(!budget.IsValid());
    }

    BOOST_AUTO_TEST_CASE(FinalizedBudgetSignatureValid)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        BOOST_REQUIRE(keyPairMn.GetPubKey() == mn1.pubkey2);
        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        // Call & Check
        BOOST_CHECK(budget.VerifySignature(mn1.pubkey2));
    }

    BOOST_AUTO_TEST_CASE(CopyFinalizedBudget)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        CFinalizedBudget copied = budget;

        BOOST_CHECK_EQUAL(budget.GetHash(), copied.GetHash());
        BOOST_CHECK_EQUAL(budget.GetFeeTxHash(), copied.GetFeeTxHash());
        BOOST_CHECK_EQUAL(budget.MasternodeSubmittedId(), copied.MasternodeSubmittedId());
        BOOST_CHECK(copied.VerifySignature(mn1.pubkey2));
    }

    BOOST_AUTO_TEST_CASE(FinalizedBudgetCanBeAdded)
    {
        BOOST_REQUIRE(budget.GetFinalizedBudgets().empty());
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        BOOST_REQUIRE(keyPairMn.GetPubKey() == mn1.pubkey2);
        CFinalizedBudgetBroadcast fb(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        BOOST_REQUIRE(fb.IsValid());
        BOOST_CHECK(budget.AddFinalizedBudget(fb));
    }

    BOOST_AUTO_TEST_CASE(FinalizedBudgetSerialized)
    {
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        CFinalizedBudgetBroadcast budget(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        BOOST_REQUIRE(budget.IsValid());

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream.reserve(1000);
        stream << budget;

        CFinalizedBudgetBroadcast restored;
        stream >> restored;

        BOOST_CHECK_EQUAL(restored.GetHash(), budget.GetHash());
        BOOST_CHECK_EQUAL(restored.MasternodeSubmittedId(), budget.MasternodeSubmittedId());
        BOOST_CHECK_EQUAL(restored.GetFeeTxHash(), budget.GetFeeTxHash());
        BOOST_CHECK(restored.VerifySignature(mn1.pubkey2));
    }

    BOOST_AUTO_TEST_CASE(FinalizedBudgetReceivedOverNetwork)
    {
        BOOST_REQUIRE(budget.GetFinalizedBudgets().empty());
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(GetPayment(proposalA));
        txBudgetPayments.push_back(GetPayment(proposalB));

        BOOST_REQUIRE(keyPairMn.GetPubKey() == mn1.pubkey2);
        CFinalizedBudgetBroadcast fb(budgetName, blockStart, txBudgetPayments, mn1.vin, keyPairMn);

        BOOST_REQUIRE(fb.IsValid());

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream.reserve(1000);
        stream << fb;

//        CFinalizedBudgetBroadcast actual;
//        stream >> actual;
//
//        BOOST_REQUIRE_EQUAL(actual.GetHash(), fb.GetHash());
//        BOOST_REQUIRE_EQUAL(actual.MasternodeSubmittedId(), fb.MasternodeSubmittedId());

        CNode dummy(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), NODE_NETWORK));

        budget.ProcessMessage(&dummy, "fbs", stream);

        BOOST_REQUIRE(!budget.GetFinalizedBudgets().empty());
        BOOST_CHECK_EQUAL(budget.GetFinalizedBudgets().front()->GetHash(), fb.GetHash());
    }

BOOST_AUTO_TEST_SUITE_END()

namespace
{
    struct BudgetProposalsFixture
    {
        const int nextSbStart;
        const int blockHeight;
        const CKey keyPair;
        const CMasternode mn;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;
        std::string error;

        BudgetProposalsFixture()
            : nextSbStart(1296000)
            , blockHeight(100500)
            , keyPair(CreateKeyPair(vchKey0))
            , hashes(static_cast<size_t>(blockHeight))
            , blocks(static_cast<size_t>(blockHeight))
        {
            // Build a main chain 100500 blocks long.
            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks[blockHeight-1]);
        }

        ~BudgetProposalsFixture()
        {
            SetMockTime(0);

            budget.Clear();
            chainActive = CChain();
        }

        CBudgetProposal CreateProposal(const std::string& name, int blockStart, int blockEnd, CKey payee, CAmount amount)
        {
            CBudgetProposal p(
                "test proposal",
                "",
                blockStart,
                blockEnd,
                PayToPublicKey(payee.GetPubKey()),
                amount * COIN,
                uint256()
            );
            p.nTime = GetTime();
            return p;
        }

        CBudgetProposal CreateProposal(const std::string& name, int blockStart, CKey payee)
        {
            return CreateProposal(name, blockStart, blockStart + GetBudgetPaymentCycleBlocks(), payee, 42);
        }
    };
}

BOOST_FIXTURE_TEST_SUITE(BudgetProposals, BudgetProposalsFixture)

    BOOST_AUTO_TEST_CASE(NoProposals)
    {
        // Set Up
        const CBudgetProposal dummy = CreateProposal("test proposal", nextSbStart, keyPair);

        // Call & Check
        BOOST_CHECK_EQUAL((void*)NULL, budget.FindProposal(dummy.GetHash()));
        BOOST_CHECK_EQUAL((void*)NULL, budget.FindProposal(dummy.GetName()));
    }

    BOOST_AUTO_TEST_CASE(ProposalIsFoundByHash)
    {
        // Set Up
        const CBudgetProposal expected = CreateProposal("test proposal", nextSbStart, keyPair);
        budget.AddProposal(expected, false); // false = don't check collateral

        // Call & Check
        const CBudgetProposal* actual = budget.FindProposal(expected.GetHash());
        BOOST_REQUIRE(actual != NULL);
        BOOST_CHECK_EQUAL(*actual, expected);
    }

    BOOST_AUTO_TEST_CASE(NoSuchHash)
    {
        // Set Up
        const CBudgetProposal expected = CreateProposal("test proposal", nextSbStart, keyPair);
        budget.AddProposal(expected, false); // false = don't check collateral

        // Call & Check
        BOOST_CHECK_EQUAL(budget.FindProposal(ArithToUint256(0xBADC0DE)), (void*)NULL);
    }

    BOOST_AUTO_TEST_CASE(ProposalIsFoundByName)
    {
        // Set Up
        const CBudgetProposal expected = CreateProposal("test proposal", nextSbStart, keyPair);
        budget.AddProposal(expected, false); // false = don't check collateral
        const CBudgetVote vote(mn.vin, expected.GetHash(), VOTE_YES);

        const bool isSubmitted = budget.SubmitProposalVote(vote, error);
        BOOST_REQUIRE(isSubmitted);

        // Call & Check
        const CBudgetProposal* actual = budget.FindProposal(expected.GetHash());
        BOOST_REQUIRE(actual != NULL);
        BOOST_CHECK_EQUAL(*actual, expected);
    }

    BOOST_AUTO_TEST_CASE(NoSuchName)
    {
        // Set Up
        const CBudgetProposal expected = CreateProposal("test proposal", nextSbStart, keyPair);
        budget.AddProposal(expected, false); // false = don't check collateral

        // Call & Check
        BOOST_CHECK_EQUAL(budget.FindProposal("not found"), (void*)NULL);
    }

BOOST_AUTO_TEST_SUITE_END()

namespace
{
    struct BudgetManagerVotingFixture
    {
        const int nextSbStart;
        const int blockHeight;
        const CKey keyPair;
        const CMasternode mn;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;
        std::string error;

        BudgetManagerVotingFixture()
            : nextSbStart(129600)
            , blockHeight(nextSbStart - 2880 + 1)
            , keyPair(CreateKeyPair(vchKey0))
            , mn(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))))
            , hashes(static_cast<size_t>(blockHeight))
            , blocks(static_cast<size_t>(blockHeight))
        {
            SetMockTime(GetTime());

            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks.back());

        }


        ~BudgetManagerVotingFixture()
        {
            SetMockTime(0);

            budget.Clear();
            chainActive = CChain();
        }

        CBudgetProposal CreateProposal(int blockStart, int blockEnd, CKey payee, CAmount amount)
        {
            CBudgetProposal p(
                "test proposal",
                "",
                blockStart,
                blockEnd,
                PayToPublicKey(payee.GetPubKey()),
                amount * COIN,
                uint256()
            );
            p.nTime = GetTime();
            return p;
        }

        CBudgetProposal CreateProposal(int blockStart, CKey payee, CAmount amount)
        {
            return CreateProposal(blockStart, blockStart + GetBudgetPaymentCycleBlocks(), payee, amount);
        }
    };
}


BOOST_FIXTURE_TEST_SUITE(BudgetVoting, BudgetManagerVotingFixture)

    BOOST_AUTO_TEST_CASE(SubmitVoteSuccess)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart + GetBudgetPaymentCycleBlocks(), keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(isSubmitted);
        BOOST_CHECK(error.empty());
        BOOST_CHECK_EQUAL(budget.mapProposals[proposal.GetHash()].mapVotes[vote.vin.prevout.GetHash()].vin, vote.vin);
    }

    BOOST_AUTO_TEST_CASE(SubmitVoteTooClose)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart, keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

    BOOST_AUTO_TEST_CASE(SubmitVoteTooCloseSecondPayment)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(
            nextSbStart - GetBudgetPaymentCycleBlocks(),
            nextSbStart + GetBudgetPaymentCycleBlocks(),
            keyPair,
            42
        );
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

    BOOST_AUTO_TEST_CASE(SubmitVoteProposalNotExists)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart + GetBudgetPaymentCycleBlocks(), keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetProposal wrongProposal = CreateProposal(nextSbStart + GetBudgetPaymentCycleBlocks(), keyPair, 43);
        const CBudgetVote vote(mn.vin, wrongProposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

    BOOST_AUTO_TEST_CASE(UpdateProposalSuccess)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart, keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        SetMockTime(100500 * 1000);

        const int superblockProjectedTime = GetTime() + (nextSbStart - chainActive.Tip()->nHeight) * Params().TargetSpacing();
        const int currentTime = superblockProjectedTime - 2161 * Params().TargetSpacing();

        SetMockTime(currentTime);

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        blocks.resize(nextSbStart - 1);
        hashes.resize(nextSbStart - 1);
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
        }
        chainActive.SetTip(&blocks.back());
        SetMockTime(superblockProjectedTime - 2 * Params().TargetSpacing());

        // Call & Check
        const bool isSubmitted = budget.ReceiveProposalVote(vote, NULL, error);

        BOOST_CHECK(isSubmitted);
        BOOST_CHECK(error.empty());
        BOOST_CHECK_EQUAL(budget.mapProposals[proposal.GetHash()].mapVotes[vote.vin.prevout.GetHash()].vin, vote.vin);
    }

    BOOST_AUTO_TEST_CASE(UpdateProposalNotExists)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart, keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        SetMockTime(100500 * 1000);

        const int superblockProjectedTime = GetTime() + (nextSbStart - chainActive.Tip()->nHeight) * Params().TargetSpacing();
        const int currentTime = superblockProjectedTime - 2160 * Params().TargetSpacing();

        SetMockTime(currentTime);

        const CBudgetProposal wrongProposal = CreateProposal(nextSbStart, keyPair, 43);
        const CBudgetVote vote(mn.vin, wrongProposal.GetHash(), VOTE_YES);

        blocks.resize(nextSbStart - 1);
        hashes.resize(nextSbStart - 1);
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
        }
        chainActive.SetTip(&blocks.back());
        SetMockTime(superblockProjectedTime - 2 * Params().TargetSpacing());

        // Call & Check
        const bool isSubmitted = budget.ReceiveProposalVote(vote, NULL, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

    BOOST_AUTO_TEST_CASE(UpdateProposalTooClose)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart, keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        SetMockTime(100500 * 1000);

        const int superblockProjectedTime = GetTime() + (nextSbStart - chainActive.Tip()->nHeight) * Params().TargetSpacing();
        const int currentTime = superblockProjectedTime - 2160 * Params().TargetSpacing();

        SetMockTime(currentTime);

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        blocks.resize(nextSbStart - 1);
        hashes.resize(nextSbStart - 1);
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
        }
        chainActive.SetTip(&blocks.back());
        SetMockTime(superblockProjectedTime - 2 * Params().TargetSpacing());

        // Call & Check
        const bool isSubmitted = budget.ReceiveProposalVote(vote, NULL, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

BOOST_AUTO_TEST_SUITE_END()

namespace
{
    struct TestnetBudgetManagerVotingFixture
    {
        const int nextSbStart;
        const int blockHeight;
        const CKey keyPair;
        const CMasternode mn;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;
        std::string error;

        const CBaseChainParams::Network prevParams = Params().NetworkID();

        TestnetBudgetManagerVotingFixture()
            : nextSbStart(10000)
            , blockHeight(nextSbStart - 9)
            , keyPair(CreateKeyPair(vchKey0))
            , mn(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))))
            , hashes(static_cast<size_t>(blockHeight))
            , blocks(static_cast<size_t>(blockHeight))
        {
            SelectParams(CBaseChainParams::TESTNET);
            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks.back());
        }

        ~TestnetBudgetManagerVotingFixture()
        {
            SelectParams(prevParams);
            SetMockTime(0);

            budget.Clear();
            chainActive = CChain();
        }

        CBudgetProposal CreateProposal(int blockStart, CKey payee, CAmount amount)
        {
            CBudgetProposal p(
                "test proposal",
                "",
                blockStart,
                blockStart + GetBudgetPaymentCycleBlocks(),
                PayToPublicKey(payee.GetPubKey()),
                amount * COIN,
                uint256()
            );
            p.nTime = GetTime();
            return p;
        }

    };
}


BOOST_FIXTURE_TEST_SUITE(TestnetBudgetVoting, TestnetBudgetManagerVotingFixture)

    BOOST_AUTO_TEST_CASE(SubmitVoteSuccess)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart + GetBudgetPaymentCycleBlocks(), keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(isSubmitted);
        BOOST_CHECK(error.empty());
        BOOST_CHECK_EQUAL(budget.mapProposals[proposal.GetHash()].mapVotes[vote.vin.prevout.GetHash()].vin, vote.vin);
    }

    BOOST_AUTO_TEST_CASE(SubmitVoteTooClose)
    {
        // Set Up
        const CBudgetProposal proposal = CreateProposal(nextSbStart, keyPair, 42);
        budget.AddProposal(proposal, false); // false = don't check collateral

        const CBudgetVote vote(mn.vin, proposal.GetHash(), VOTE_YES);

        // Call & Check
        const bool isSubmitted = budget.SubmitProposalVote(vote, error);

        BOOST_CHECK(!isSubmitted);
        BOOST_CHECK(!error.empty());
        BOOST_CHECK(budget.mapProposals[proposal.GetHash()].mapVotes.empty());
    }

BOOST_AUTO_TEST_SUITE_END()

namespace
{
    struct SuperblockPaymentFixture
    {
        const int blockHeight;

        const CKey keyPairA;
        const CKey keyPairB;
        const CKey keyPairC;

        const CMasternode mn1;
        const CMasternode mn2;
        const CMasternode mn3;
        const CMasternode mn4;
        const CMasternode mn5;

        std::vector<uint256> hashes;
        std::vector<CBlockIndex> blocks;

        const int fees;
        const uint256 finalBudgetCollateral;

        std::string error;

        SuperblockPaymentFixture()
            : blockHeight(1296000)
            , keyPairA(CreateKeyPair(vchKey0))
            , keyPairB(CreateKeyPair(vchKey1))
            , keyPairC(CreateKeyPair(vchKey2))
            , mn1(CreateMasternode(CTxIn(COutPoint(ArithToUint256(1), 1 * COIN))))
            , mn2(CreateMasternode(CTxIn(COutPoint(ArithToUint256(2), 1 * COIN))))
            , mn3(CreateMasternode(CTxIn(COutPoint(ArithToUint256(3), 1 * COIN))))
            , mn4(CreateMasternode(CTxIn(COutPoint(ArithToUint256(4), 1 * COIN))))
            , mn5(CreateMasternode(CTxIn(COutPoint(ArithToUint256(5), 1 * COIN))))
            , hashes(static_cast<size_t>(blockHeight + 1))
            , blocks(static_cast<size_t>(blockHeight + 1))
            , fees(0)
            , finalBudgetCollateral(ArithToUint256(1))
        {
            // Build a main chain 100500 blocks long.
            for (size_t i = 0; i < blocks.size(); ++i)
            {
                FillBlock(blocks[i], hashes[i], &blocks[i - 1], i);
            }
            chainActive.SetTip(&blocks[blockHeight-1]);

            SetMockTime(GetTime());

            mnodeman.Add(mn1);
            mnodeman.Add(mn2);
            mnodeman.Add(mn3);
            mnodeman.Add(mn4);
            mnodeman.Add(mn5);
        }

        ~SuperblockPaymentFixture()
        {
            SetMockTime(0);

            mnodeman.Clear();
            budget.Clear();
            chainActive = CChain();
        }

        CTxBudgetPayment CreatePayment(CKey payee, CAmount amount)
        {
            return CTxBudgetPayment(ArithToUint256(std::rand()), PayToPublicKey(payee.GetPubKey()), amount);
        }
    };
}

BOOST_FIXTURE_TEST_SUITE(SuperblockPayment, SuperblockPaymentFixture)

    BOOST_AUTO_TEST_CASE(FinalizedBudgetIsSuccessfullyAdded)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));

        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        BOOST_REQUIRE(budget.GetFinalizedBudgets().empty());
        
        // Call & Check
        budget.AddFinalizedBudget(fb, false); // false = don't check collateral
        std::vector<CFinalizedBudget*> actual = budget.GetFinalizedBudgets();

        BOOST_REQUIRE_EQUAL(actual.size(), 1);
        BOOST_CHECK_EQUAL(fb.GetHash(), actual.front()->GetHash());
    }

    BOOST_AUTO_TEST_CASE(FinalizedBudgetHasCorrectStartAndEnd)
    {
        // Set Up
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));

        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        // Call & Check

        BOOST_CHECK_EQUAL(fb.GetBlockStart(), blockHeight);
        BOOST_CHECK_EQUAL(fb.GetBlockEnd(), blockHeight);
    }

    BOOST_AUTO_TEST_CASE(AllProposalsArePaidInOneBlock)
    {
        // Set Up

        // Create finalized budget
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));
        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        // Add finalized budget to budget manager
        budget.AddFinalizedBudget(fb, false); // false = don't check collateral

        // Vote for finalized buget
        CFinalizedBudgetVote vote1(mn1.vin, fb.GetHash());
        budget.UpdateFinalizedBudget(vote1, NULL, error);

        // Set expectations
        std::vector<CTxOut> expected;
        expected.push_back(CTxOut(9 * COIN, CScript()));
        expected.push_back(CTxOut(404, PayToPublicKey(keyPairB.GetPubKey()))); // Highest amount is #1
        expected.push_back(CTxOut(111, PayToPublicKey(keyPairC.GetPubKey())));
        expected.push_back(CTxOut(42, PayToPublicKey(keyPairA.GetPubKey()))); // Lowest amount is #31

        BOOST_REQUIRE_EQUAL(chainActive.Tip()->nHeight + 1, blockHeight);

        // Call & Check

        CMutableTransaction actual;
        actual.vout.push_back(CTxOut());

        budget.FillBlockPayee(actual, fees);

        BOOST_CHECK(budget.IsBudgetPaymentBlock(chainActive.Tip()->nHeight + 1));
        BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.vout.begin(), actual.vout.end());
    }

    BOOST_AUTO_TEST_CASE(NoProposalsArePaidAfterSuperblockIsPaid)
    {
        // Set Up

        // Create finalized budget
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));
        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        // Add finalized budget to budget manager
        budget.AddFinalizedBudget(fb, false); // false = don't check collateral

        // Vote for finalized buget
        CFinalizedBudgetVote vote1(mn1.vin, fb.GetHash());
        budget.UpdateFinalizedBudget(vote1, NULL, error);

        // Rewind 1 block forward
        chainActive.SetTip(&blocks[blockHeight]);
        BOOST_REQUIRE_EQUAL(chainActive.Tip()->nHeight + 1, blockHeight + 1);

        // Set expectations
        std::vector<CTxOut> expected;
        expected.push_back(CTxOut(9 * COIN, CScript()));

        // Call & Check

        CMutableTransaction actual;
        actual.vout.push_back(CTxOut());

        budget.FillBlockPayee(actual, fees);

        BOOST_CHECK(!budget.IsBudgetPaymentBlock(chainActive.Tip()->nHeight + 1));
        BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.vout.begin(), actual.vout.end());
    }

    BOOST_AUTO_TEST_CASE(BudgetWithZeroVotesIsNotPaid)
    {
        // Set Up

        // Create finalized budget
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));
        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        // Add finalized budget to budget manager
        budget.AddFinalizedBudget(fb, false); // false = don't check collateral

        // Set expectations
        std::vector<CTxOut> expected;
        expected.push_back(CTxOut(9 * COIN, CScript()));

        BOOST_REQUIRE_EQUAL(chainActive.Tip()->nHeight + 1, blockHeight);

        // Call & Check

        CMutableTransaction actual;
        actual.vout.push_back(CTxOut());

        budget.FillBlockPayee(actual, fees);

        BOOST_CHECK(!budget.IsBudgetPaymentBlock(chainActive.Tip()->nHeight + 1));
        BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.vout.begin(), actual.vout.end());
    }

    BOOST_AUTO_TEST_CASE(BudgetWithMoreVotesWins)
    {
        // Set Up

        // Create finalized budget #1
        std::vector<CTxBudgetPayment> txBudgetPayments1;
        txBudgetPayments1.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments1.push_back(CreatePayment(keyPairC, 111));
        CFinalizedBudgetBroadcast fb1("test", blockHeight, txBudgetPayments1, finalBudgetCollateral);

        // Create finalized budget #2
        std::vector<CTxBudgetPayment> txBudgetPayments2;
        txBudgetPayments2.push_back(CreatePayment(keyPairA, 42));
        CFinalizedBudgetBroadcast fb2("test", blockHeight, txBudgetPayments2, finalBudgetCollateral);

        // Add finalized budgets to budget manager
        budget.AddFinalizedBudget(fb1, false); // false = don't check collateral
        budget.AddFinalizedBudget(fb2, false); // false = don't check collateral

        // Vote for finalized bugets
        CFinalizedBudgetVote vote1(mn1.vin, fb1.GetHash());
        CFinalizedBudgetVote vote2(mn2.vin, fb1.GetHash());

        CFinalizedBudgetVote vote3(mn3.vin, fb2.GetHash());
        CFinalizedBudgetVote vote4(mn4.vin, fb2.GetHash());
        CFinalizedBudgetVote vote5(mn5.vin, fb2.GetHash());

        budget.UpdateFinalizedBudget(vote1, NULL, error);
        budget.UpdateFinalizedBudget(vote2, NULL, error);
        budget.UpdateFinalizedBudget(vote3, NULL, error);
        budget.UpdateFinalizedBudget(vote4, NULL, error);
        budget.UpdateFinalizedBudget(vote5, NULL, error);

        // Set expectations
        std::vector<CTxOut> expected;
        expected.push_back(CTxOut(9 * COIN, CScript()));
        expected.push_back(CTxOut(42, PayToPublicKey(keyPairA.GetPubKey()))); // Payment from budget #2

        // Call & Check

        CMutableTransaction actual;
        actual.vout.push_back(CTxOut());

        budget.FillBlockPayee(actual, fees);

        BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.vout.begin(), actual.vout.end());
    }

    BOOST_AUTO_TEST_CASE(TransactionWithBudgetPaymentsIsValid)
    {
        // Set Up

        // Create finalized budget
        std::vector<CTxBudgetPayment> txBudgetPayments;
        txBudgetPayments.push_back(CreatePayment(keyPairA, 42));
        txBudgetPayments.push_back(CreatePayment(keyPairB, 404));
        txBudgetPayments.push_back(CreatePayment(keyPairC, 111));
        CFinalizedBudgetBroadcast fb("test", blockHeight, txBudgetPayments, finalBudgetCollateral);

        // Add finalized budget to budget manager
        budget.AddFinalizedBudget(fb, false); // false = don't check collateral

        // Vote for finalized buget
        CFinalizedBudgetVote vote1(mn1.vin, fb.GetHash());
        budget.UpdateFinalizedBudget(vote1, NULL, error);

        BOOST_REQUIRE_EQUAL(chainActive.Tip()->nHeight + 1, blockHeight);

        // Call & Check

        CMutableTransaction actual;
        actual.vout.push_back(CTxOut());
        budget.FillBlockPayee(actual, fees);

        BOOST_CHECK(budget.IsTransactionValid(actual, blockHeight));
    }

BOOST_AUTO_TEST_SUITE_END()
