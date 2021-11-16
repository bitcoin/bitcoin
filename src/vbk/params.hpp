// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_BTCSQ_PARAMS_HPP
#define INTEGRATION_REFERENCE_BTC_BTCSQ_PARAMS_HPP

#include <limits>
#include <primitives/block.h>
#include <veriblock/pop.hpp>


class ArgsManager;

namespace VeriBlock {

struct AltChainParamsBTCSQ : public altintegration::AltChainParams {
    ~AltChainParamsBTCSQ() override = default;
    AltChainParamsBTCSQ() = default;

    explicit AltChainParamsBTCSQ(const CBlock& genesis)
    {
        // if block time changes, update altchain id. see altchain id comment.
        assert(altintegration::getMaxAtvsInVbkBlock(VeriBlock::ALT_CHAIN_ID) == 20);


        bootstrap.hash = genesis.GetHash().asVector();
        // intentionally leave prevHash empty
        bootstrap.height = 0;
        bootstrap.timestamp = genesis.GetBlockTime();

        // these parameters changed in comparison to default parameters
        this->mPopPayoutsParams->mPopPayoutDelay = 150;
        this->mPopPayoutsParams->mDifficultyAveragingInterval = 150;
        this->mEndorsementSettlementInterval = 150;
        this->mMaxVbkBlocksInAltBlock = 100;
        this->mMaxVTBsInAltBlock = 50;
        this->mMaxATVsInAltBlock = 100;
        this->mPreserveBlocksBehindFinal = mEndorsementSettlementInterval;
        this->mMaxReorgDistance = std::numeric_limits<int>::max(); // disable finalization for now

        //! copying all parameters here to make sure that
        //! if anyone changes them in alt-int-cpp, they
        //! won't be changed in BTCSQ.

        // pop payout params
        this->mPopPayoutsParams->mStartOfSlope = 1.0;
        this->mPopPayoutsParams->mSlopeNormal = 0.2;
        this->mPopPayoutsParams->mSlopeKeystone = 0.21325;
        this->mPopPayoutsParams->mKeystoneRound = 3;
        this->mPopPayoutsParams->mPayoutRounds = 4;
        this->mPopPayoutsParams->mFlatScoreRound = 2;
        this->mPopPayoutsParams->mUseFlatScoreRound = true;
        this->mPopPayoutsParams->mMaxScoreThresholdNormal = 2.0;
        this->mPopPayoutsParams->mMaxScoreThresholdKeystone = 3.0;
        this->mPopPayoutsParams->mRoundRatios = {0.97, 1.03, 1.07, 3.00};
        this->mPopPayoutsParams->mLookupTable = {
            1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000,
            1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000, 1.00000000,
            0.48296816, 0.31551694, 0.23325824, 0.18453616, 0.15238463, 0.12961255,
            0.11265630, 0.09955094, 0.08912509, 0.08063761, 0.07359692, 0.06766428,
            0.06259873, 0.05822428, 0.05440941, 0.05105386, 0.04807993, 0.04542644,
            0.04304458, 0.04089495, 0.03894540, 0.03716941, 0.03554497, 0.03405359,
            0.03267969, 0.03141000, 0.03023319, 0.02913950, 0.02812047, 0.02716878,
            0.02627801, 0.02544253, 0.02465739, 0.02391820, 0.02322107, 0.02256255,
            0.02193952, 0.02134922};

        // altchain params
        this->mMaxAltchainFutureBlockTime = 10 * 60; // 10 min
        this->mKeystoneInterval = 5;
        this->mFinalityDelay = 100;
        this->mMaxPopDataSize = altintegration::MAX_POPDATA_SIZE;
        this->mForkResolutionLookUpTable = {
            100, 100, 95, 89, 80, 69, 56, 40, 21};
    }

    altintegration::AltBlock getBootstrapBlock() const noexcept override
    {
        return bootstrap;
    }

    int64_t getIdentifier() const noexcept override
    {
        return VeriBlock::ALT_CHAIN_ID;
    }

    std::vector<uint8_t> getHash(const std::vector<uint8_t>& bytes) const noexcept override;

    // we should verify:
    // - check that 'bytes' can be deserialized to a CBlockHeader
    // - check that this CBlockHeader is valid (time, pow, version...)
    // - check that 'root' is equal to Merkle Root in CBlockHeader
    bool checkBlockHeader(
        const std::vector<uint8_t>& bytes,
        const std::vector<uint8_t>& root,
        altintegration::ValidationState& state) const noexcept override;

    altintegration::AltBlock bootstrap;
};

struct AltChainParamsBTCSQRegTest : public AltChainParamsBTCSQ {
    ~AltChainParamsBTCSQRegTest() override = default;

    explicit AltChainParamsBTCSQRegTest(const CBlock& genesis) : AltChainParamsBTCSQ(genesis)
    {
        this->mMaxReorgDistance = 1000;
        this->mMaxVbkBlocksInAltBlock = 200;
        this->mMaxVTBsInAltBlock = 200;
        this->mMaxATVsInAltBlock = 1000;
    }
};

struct AltChainParamsBTCSQDetRegTest : public AltChainParamsBTCSQ {
    ~AltChainParamsBTCSQDetRegTest() override = default;

    AltChainParamsBTCSQDetRegTest()
    {
        bootstrap.hash = uint256S("393e1fea789a3ac750921d6d9f6aa7e84df9e031ecd63fca22dc3adc0632025c").asVector();
        bootstrap.previousBlock = uint256S("42b16a400669ab1403410585e793cec350baa53ff3fddc2414be92f03f1b12f2").asVector();
        bootstrap.height = 1000;
        // intentionally leave timestamp empty
    }
};

void printConfig(const altintegration::Config& config);
void selectPopConfig(const std::string& network = "test");

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_BTCSQ_PARAMS_HPP