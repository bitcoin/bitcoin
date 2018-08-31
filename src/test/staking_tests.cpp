#include <boost/test/unit_test.hpp>
#include <iostream>

#include "amount.h"
#include "arith_uint256.h"
#include "key.h"
#include "utiltime.h"
#include "primitives/block.h"

#include "mn-pos/kernel.h"
#include "mn-pos/stakeminer.h"
#include "mn-pos/stakevalidation.h"


BOOST_AUTO_TEST_SUITE(staking_tests)

BOOST_AUTO_TEST_CASE(stakeminer)
{
    //Make a kernel with some fake data
    std::pair<uint256, unsigned int> outpoint = std::make_pair(uint256S("99999"), 0);
    uint256 nModifier = uint256S("123456");
    unsigned int nTimeBlockFrom = 100000;
    unsigned int nTimeStake = nTimeBlockFrom + (60*60*48);
    uint64_t nAmount = 10000 * COIN;
    Kernel kernel(outpoint, nAmount, nModifier, nTimeBlockFrom, nTimeStake);

    //A moderately easy difficulty target to hit
    arith_uint256 aTarget;
    aTarget = ~aTarget;
    aTarget >>= 14;
    uint256 nTarget = ArithToUint256(aTarget);

    //Start searching from the current time and do not stop until a valid hash is found
    uint64_t nSearchStart = GetTime();
    bool found = false;
    while (!found)
    {
        uint64_t nSearchEnd = nSearchStart + 180;
        found = SearchTimeSpan(kernel, nSearchStart, nSearchStart + 180, nTarget);
        nSearchStart = nSearchEnd + 1;
    }

    BOOST_CHECK_MESSAGE(kernel.IsValidProof(nTarget), "did not find a valid kernel");
}

BOOST_AUTO_TEST_CASE(proof_validity)
{
    uint64_t nAmount = 10000 * COIN; //10,000 coins is the amount for a masternode

    arith_uint256 target;
    target = ~target;
    target >>= 8; //0x00F

    uint256 hashReq = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffff8463423000");
    arith_uint256 hash = UintToArith256(hashReq) - 1;
    target /= nAmount; //Need to compare the hash divided by the amount because it gets multiplied inside the check

    BOOST_CHECK_MESSAGE(Kernel::CheckProof(target, hash, nAmount), "failed to pass hash proof that is valid");
    BOOST_CHECK_MESSAGE(!Kernel::CheckProof(target, hash + 1, nAmount), "hash that is equal to target was considered valid proof hash");
    BOOST_CHECK_MESSAGE(!Kernel::CheckProof(target, hash + 2, nAmount), "hash that is greater than target was considered valid proof hash");
}

BOOST_AUTO_TEST_CASE(transaction_type)
{
    CTxIn txIn;
    txIn.scriptSig << OP_PROOFOFSTAKE;
    CMutableTransaction tx;
    tx.vin.emplace_back(txIn);
    tx.vout.emplace_back(CTxOut());

    CTransaction txCoinStake(tx);
    BOOST_CHECK_MESSAGE(txCoinStake.IsCoinStake(), "coinstake transaction not marked as coinstake");
    BOOST_CHECK_MESSAGE(!txCoinStake.IsCoinBase(), "coinstake transaction is marked as coinbase");

    tx.vin.clear();
    txCoinStake = tx;
    BOOST_CHECK_MESSAGE(!txCoinStake.IsCoinStake(), "tx with no inputs is marked as coinstake");

    tx.vin.emplace_back(CTxIn());
    txCoinStake = tx;
    BOOST_CHECK_MESSAGE(!txCoinStake.IsCoinStake(), "tx with null input but no PoS mark is marked as coinstake");

    tx.vin.clear();
    tx.vin.emplace_back(txIn);
    tx.vout.emplace_back(CTxOut());
    txCoinStake = tx;
    BOOST_CHECK_MESSAGE(!txCoinStake.IsCoinStake(), "tx with two outputs is marked as coinstake");

    tx.vout.clear();
    txCoinStake = tx;
    BOOST_CHECK_MESSAGE(!txCoinStake.IsCoinStake(), "tx with no outputs is marked as coinstake");
}

BOOST_AUTO_TEST_CASE(block_type)
{
    CBlock block;
    CMutableTransaction tx;
    CTxIn txIn;
    txIn.scriptSig << OP_PROOFOFSTAKE;
    tx.vin.emplace_back(txIn);
    tx.vout.emplace_back(CTxOut());
    block.vtx.emplace_back(CTransaction());
    block.vtx.emplace_back(tx);
    BOOST_CHECK_MESSAGE(block.IsProofOfStake(), "Proof of Stake block failed IsProofOfStake() test");

    CBlock blockPoW;
    BOOST_CHECK_MESSAGE(blockPoW.IsProofOfWork(), "Proof of Work block failed IsProofOfWork() test");
}

BOOST_AUTO_TEST_CASE(block_signature)
{
    //Dummy PoS Block
    CBlock block;
    CMutableTransaction tx;
    uint256 txid = uint256S("1");
    tx.vin.emplace_back(CTxIn(COutPoint(uint256(), -1), CScript()));
    tx.vin.emplace_back(CTxIn(COutPoint(txid, 0), CScript()));
    block.vtx.emplace_back(CTransaction());
    block.vtx.emplace_back(tx);

    CKey keyMasternode;
    keyMasternode.MakeNewKey(true);
    std::vector<unsigned char> vchSig;
    BOOST_CHECK_MESSAGE(keyMasternode.Sign(block.GetHash(), vchSig), "failed to sign block with key");

    //Correct signature
    block.vchBlockSig = vchSig;
    BOOST_CHECK_MESSAGE(CheckBlockSignature(block, keyMasternode.GetPubKey()), "block signature validation failed");

    //Sign with different key
    CKey keyOther;
    keyOther.MakeNewKey(true);
    BOOST_CHECK_MESSAGE(!CheckBlockSignature(block, keyOther.GetPubKey()), "considered block signature valid when it "
                                                                           "should not have");
    //Sign wrong hash
    vchSig.clear();
    BOOST_CHECK_MESSAGE(keyMasternode.Sign(uint256S("123456789"), vchSig), "failed to sign block with key");
    block.vchBlockSig = vchSig;
    BOOST_CHECK_MESSAGE(!CheckBlockSignature(block, keyMasternode.GetPubKey()), "Validated signature that was not valid");
}

BOOST_AUTO_TEST_SUITE_END()