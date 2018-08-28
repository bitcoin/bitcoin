#include <boost/test/unit_test.hpp>
#include <iostream>

#include "amount.h"
#include "arith_uint256.h"
#include "utiltime.h"

#include "mn-pos/kernel.h"
#include "mn-pos/stakeminer.h"


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

BOOST_AUTO_TEST_SUITE_END()