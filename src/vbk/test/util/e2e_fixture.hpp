// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_TEST_UTIL_E2E_FIXTURE_HPP
#define BITCOIN_SRC_VBK_TEST_UTIL_E2E_FIXTURE_HPP

#include <boost/test/unit_test.hpp>

#include <bootstraps.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/log.hpp>
#include <vbk/util.hpp>
#include <veriblock/alt-util.hpp>
#include <veriblock/mempool.hpp>
#include <veriblock/mock_miner.hpp>

using altintegration::ATV;
using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

struct TestLogger : public altintegration::Logger {
    ~TestLogger() override = default;

    void log(altintegration::LogLevel lvl, const std::string& msg) override
    {
        fmt::printf("[pop] [%s]\t%s\n", altintegration::LevelToString(lvl), msg);
    }
};

struct E2eFixture : public TestChain100Setup {
    CScript cbKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    MockMiner popminer;
    altintegration::ValidationState state;
    altintegration::PopContext* pop;
    std::vector<uint8_t> defaultPayoutInfo = {1, 2, 3, 4, 5};

    E2eFixture()
    {
        altintegration::SetLogger<TestLogger>();
        altintegration::GetLogger().level = altintegration::LogLevel::warn;
        pop = &VeriBlock::GetPop();
    }

    void InvalidateTestBlock(CBlockIndex* pblock)
    {
        BlockValidationState state;
        InvalidateBlock(state, Params(), pblock);
        ActivateBestChain(state, Params());
        mempool.clear();
    }

    void ReconsiderTestBlock(CBlockIndex* pblock)
    {
        BlockValidationState state;

        {
            LOCK(cs_main);
            ResetBlockFailureFlags(pblock);
        }
        ActivateBestChain(state, Params(), std::shared_ptr<const CBlock>());
    }

    BtcBlock::hash_t getLastKnownBTCblock()
    {
        auto blocks = VeriBlock::getLastKnownBTCBlocks(1);
        BOOST_CHECK(blocks.size() == 1);
        return blocks[0];
    }

    VbkBlock::hash_t getLastKnownVBKblock()
    {
        auto blocks = VeriBlock::getLastKnownVBKBlocks(1);
        BOOST_CHECK(blocks.size() == 1);
        return blocks[0];
    }

    ATV endorseAltBlock(uint256 hash, const std::vector<VTB>& vtbs, const std::vector<uint8_t>& payoutInfo)
    {
        CBlockIndex* endorsed = nullptr;
        {
            LOCK(cs_main);
            endorsed = LookupBlockIndex(hash);
            BOOST_CHECK(endorsed != nullptr);
        }

        auto publicationdata = createPublicationData(endorsed, payoutInfo);
        auto vbktx = popminer.createVbkTxEndorsingAltBlock(publicationdata);
        auto atv = popminer.applyATV(vbktx, state);
        BOOST_CHECK(state.IsValid());
        return atv;
    }

    ATV endorseAltBlock(uint256 hash, const std::vector<VTB>& vtbs)
    {
        return endorseAltBlock(hash, vtbs, defaultPayoutInfo);
    }

    CBlock endorseAltBlockAndMine(const std::vector<uint256>& hashes, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(hashes, ChainActive().Tip()->GetBlockHash(), generateVtbs);
    }

    CBlock endorseAltBlockAndMine(const std::vector<uint256>& hashes, uint256 prevBlock, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(hashes, prevBlock, defaultPayoutInfo, generateVtbs);
    }

    CBlock endorseAltBlockAndMine(const std::vector<uint256>& hashes, uint256 prevBlock, const std::vector<uint8_t>& payoutInfo, size_t generateVtbs = 0)
    {
        std::vector<VTB> vtbs;
        vtbs.reserve(generateVtbs);
        std::generate_n(std::back_inserter(vtbs), generateVtbs, [&]() {
            return endorseVbkTip();
        });

        std::vector<ATV> atvs;
        atvs.reserve(hashes.size());
        std::transform(hashes.begin(), hashes.end(), std::back_inserter(atvs), [&](const uint256& hash) -> ATV {
            return endorseAltBlock(hash, {}, payoutInfo);
        });

        auto& pop_mempool = *pop->mempool;
        altintegration::ValidationState state;
        for (const auto& atv : atvs) {
            pop_mempool.submit(atv, state);
        }

        for (const auto& vtb : vtbs) {
            pop_mempool.submit(vtb, state);
        }

        bool isValid = false;
        return CreateAndProcessBlock({}, prevBlock, cbKey, &isValid);
    }

    CBlock endorseAltBlockAndMine(uint256 hash, uint256 prevBlock, const std::vector<uint8_t>& payoutInfo, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(std::vector<uint256>{hash}, prevBlock, payoutInfo, generateVtbs);
    }

    CBlock endorseAltBlockAndMine(uint256 hash, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(hash, ChainActive().Tip()->GetBlockHash(), generateVtbs);
    }

    CBlock endorseAltBlockAndMine(uint256 hash, uint256 prevBlock, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(hash, prevBlock, defaultPayoutInfo, generateVtbs);
    }

    VTB endorseVbkTip()
    {
        auto best = popminer.vbk().getBestChain();
        auto tip = best.tip();
        BOOST_CHECK(tip != nullptr);
        return endorseVbkBlock(tip->getHeight());
    }

    VTB endorseVbkBlock(int height)
    {
        auto vbkbest = popminer.vbk().getBestChain();
        auto endorsed = vbkbest[height];
        if (!endorsed) {
            throw std::logic_error("can not find VBK block at height " + std::to_string(height));
        }

        auto btctx = popminer.createBtcTxEndorsingVbkBlock(endorsed->getHeader());
        auto* btccontaining = popminer.mineBtcBlocks(1);
        auto vbktx = popminer.createVbkPopTxEndorsingVbkBlock(btccontaining->getHeader(), btctx, endorsed->getHeader(), getLastKnownBTCblock());
        auto* vbkcontaining = popminer.mineVbkBlocks(1);

        auto vtbs = popminer.vbkPayloads[vbkcontaining->getHash()];
        BOOST_CHECK(vtbs.size() == 1);
        return vtbs[0];
    }

    PublicationData createPublicationData(CBlockIndex* endorsed, const std::vector<uint8_t>& payoutInfo)
    {
        PublicationData p;

        auto& config = *VeriBlock::GetPop().config;
        p.identifier = config.alt->getIdentifier();
        p.payoutInfo = payoutInfo;

        // serialize block header
        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << endorsed->GetBlockHeader();
        p.header = std::vector<uint8_t>{stream.begin(), stream.end()};

        return p;
    }

    PublicationData createPublicationData(CBlockIndex* endorsed)
    {
        return createPublicationData(endorsed, defaultPayoutInfo);
    }
};

#endif //BITCOIN_SRC_VBK_TEST_UTIL_E2E_FIXTURE_HPP
