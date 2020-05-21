// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_TEST_UTIL_E2E_FIXTURE_HPP
#define BITCOIN_SRC_VBK_TEST_UTIL_E2E_FIXTURE_HPP

#include <boost/test/unit_test.hpp>

#include <bootstraps.h>
#include <chain.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/util.hpp>
#include <veriblock/alt-util.hpp>
#include <veriblock/mock_miner.hpp>
#include <vbk/log.hpp>

using altintegration::AltPayloads;
using altintegration::ATV;
using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

struct TestLogger : public altintegration::Logger {
    ~TestLogger() override = default;

    void log(altintegration::LogLevel lvl, const std::string& msg) override {
        fmt::printf("[pop] [%s]\t%s\n", altintegration::LevelToString(lvl), msg);
    }
};

struct E2eFixture : public TestChain100Setup {
    CScript cbKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    MockMiner popminer;
    altintegration::ValidationState state;
    VeriBlock::PopService* pop;
    std::vector<uint8_t> defaultPayoutInfo = {1, 2, 3, 4, 5};

    E2eFixture()
    {
        altintegration::SetLogger<TestLogger>();
        altintegration::GetLogger().level = altintegration::LogLevel::debug;
        pop = &VeriBlock::getService<VeriBlock::PopService>();
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
        auto atv = popminer.generateATV(vbktx, getLastKnownVBKblock(), state);
        BOOST_CHECK(state.IsValid());
        return atv;
    }

    ATV endorseAltBlock(uint256 hash, const std::vector<VTB>& vtbs)
    {
        return endorseAltBlock(hash, vtbs, defaultPayoutInfo);
    }

    CBlock endorseAltBlockAndMine(uint256 hash, size_t generateVtbs = 0)
    {
        return endorseAltBlockAndMine(hash, ChainActive().Tip()->GetBlockHash(), generateVtbs);
    }

    CBlock endorseAltBlockAndMine(uint256 hash, uint256 prevBlock, const std::vector<uint8_t>& payoutInfo, size_t generateVtbs = 0)
    {
        std::vector<VTB> vtbs;
        vtbs.reserve(generateVtbs);
        std::generate_n(std::back_inserter(vtbs), generateVtbs, [&]() {
            return endorseVbkTip();
        });
        auto atv = endorseAltBlock(hash, vtbs, payoutInfo);
        CScript sig;
        sig << atv.toVbkEncoding() << OP_CHECKATV;
        for (const auto& v : vtbs) {
            sig << v.toVbkEncoding() << OP_CHECKVTB;
        }
        sig << OP_CHECKPOP;

        auto tx = VeriBlock::MakePopTx(sig);
        bool isValid = false;
        return CreateAndProcessBlock({tx}, prevBlock, cbKey, &isValid);
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
        return endorseVbkBlock(tip->height);
    }

    VTB endorseVbkBlock(int height)
    {
        auto vbkbest = popminer.vbk().getBestChain();
        auto endorsed = vbkbest[height];
        if (!endorsed) {
            throw std::logic_error("can not find VBK block at height " + std::to_string(height));
        }

        auto btctx = popminer.createBtcTxEndorsingVbkBlock(*endorsed->header);
        auto* btccontaining = popminer.mineBtcBlocks(1);
        auto vbktx = popminer.createVbkPopTxEndorsingVbkBlock(*btccontaining->header, btctx, *endorsed->header, getLastKnownBTCblock());
        auto* vbkcontaining = popminer.mineVbkBlocks(1);

        auto vtbs = popminer.vbkPayloads[vbkcontaining->getHash()];
        BOOST_CHECK(vtbs.size() == 1);
        VTB& vtb = vtbs[0];

        // fill VTB context: from last known VBK block to containing
        auto* current = vbkcontaining;
        auto lastKnownVbk = getLastKnownVBKblock();
        while (current != nullptr && current->getHash() != lastKnownVbk) {
            vtb.context.push_back(*current->header);
            current = current->pprev;
        }
        std::reverse(vtb.context.begin(), vtb.context.end());

        return vtb;
    }

    BtcBlock::hash_t getLastKnownBTCblock()
    {
        auto blocks = pop->getLastKnownBTCBlocks(1);
        BOOST_CHECK(blocks.size() == 1);
        return blocks[0];
    }

    VbkBlock::hash_t getLastKnownVBKblock()
    {
        auto& pop = VeriBlock::getService<VeriBlock::PopService>();
        auto blocks = pop.getLastKnownVBKBlocks(1);
        BOOST_CHECK(blocks.size() == 1);
        return blocks[0];
    }

    PublicationData createPublicationData(CBlockIndex* endorsed, const std::vector<uint8_t>& payoutInfo)
    {
        PublicationData p;

        auto& config = VeriBlock::getService<VeriBlock::Config>();
        p.identifier = config.popconfig.alt->getIdentifier();
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
