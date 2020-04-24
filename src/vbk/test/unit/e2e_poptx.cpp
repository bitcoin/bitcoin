// VeriBlock Blockchain Project
// Copyright 2017-2018 VeriBlock, Inc
// Copyright 2018-2019 Xenios SEZC
// All rights reserved.
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <boost/test/unit_test.hpp>

#include <bootstraps.h>
#include <chain.h>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/util.hpp>
#include <veriblock/alt-util.hpp>
#include <veriblock/mock_miner.hpp>

using altintegration::AltPayloads;
using altintegration::BtcBlock;
using altintegration::MockMiner;
using altintegration::PublicationData;
using altintegration::VbkBlock;
using altintegration::VTB;

struct E2eFixture : public TestChain100Setup {
    CScript cbKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    MockMiner popminer;
    altintegration::ValidationState state;
    VeriBlock::PopService* pop;

    E2eFixture()
    {
        pop = &VeriBlock::getService<VeriBlock::PopService>();
    }


    CBlock endorseAltBlock(uint256 hash, size_t generateVtbs = 0)
    {
        return endorseAltBlock(hash, ChainActive().Tip()->GetBlockHash(), generateVtbs);
    }

    CBlock endorseAltBlock(uint256 hash, uint256 prevBlock, size_t generateVtbs = 0)
    {
        CBlockIndex* endorsed = nullptr;
        {
            LOCK(cs_main);
            endorsed = LookupBlockIndex(hash);
            BOOST_CHECK(endorsed != nullptr);
        }

        std::vector<VTB> vtbs;
        vtbs.reserve(generateVtbs);
        std::generate_n(std::back_inserter(vtbs), generateVtbs, [&]() {
            return endorseVbkTip();
        });

        auto publicationdata = createPublicationData(endorsed);
        auto vbktx = popminer.endorseAltBlock(publicationdata);
        auto atv = popminer.generateATV(vbktx, getLastKnownVBKblock(), state);
        BOOST_CHECK(state.IsValid());

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

    PublicationData createPublicationData(CBlockIndex* endorsed)
    {
        PublicationData p;

        auto& config = VeriBlock::getService<VeriBlock::Config>();
        p.identifier = config.popconfig.alt->getIdentifier();
        // TODO: pass valid payout info
        p.payoutInfo = std::vector<uint8_t>{1, 2, 3, 4, 5};

        // serialize block header
        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << endorsed->GetBlockHeader();
        p.header = std::vector<uint8_t>{stream.begin(), stream.end()};

        return p;
    }
};

BOOST_AUTO_TEST_SUITE(e2e_poptx_tests)

BOOST_FIXTURE_TEST_CASE(ValidBlockIsAccepted, E2eFixture)
{
    // altintegration and popminer configured to use BTC/VBK/ALT regtest.
    auto tip = ChainActive().Tip();
    BOOST_CHECK(tip != nullptr);

    // endorse tip
    CBlock block = endorseAltBlock(tip->GetBlockHash(), 10);
    {
        BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == block.GetHash());
        auto btc = pop->getLastKnownBTCBlocks(1)[0];
        BOOST_CHECK(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = pop->getLastKnownVBKBlocks(1)[0];
        BOOST_CHECK(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }

    // endorse another tip
    block = endorseAltBlock(tip->GetBlockHash(), 1);
    auto lastHash = ChainActive().Tip()->GetBlockHash();
    {
        BOOST_CHECK(lastHash == block.GetHash());
        auto btc = pop->getLastKnownBTCBlocks(1)[0];
        BOOST_CHECK(btc == popminer.btc().getBestChain().tip()->getHash());
        auto vbk = pop->getLastKnownVBKBlocks(1)[0];
        BOOST_CHECK(vbk == popminer.vbk().getBestChain().tip()->getHash());
    }

    // create block that is not on main chain
    auto fork1tip = CreateAndProcessBlock({}, ChainActive().Tip()->pprev->pprev->GetBlockHash(), cbKey);

    // endorse block that is not on main chain
    block = endorseAltBlock(fork1tip.GetHash(), 1);
    BOOST_CHECK(ChainActive().Tip()->GetBlockHash() == lastHash);
}

BOOST_AUTO_TEST_SUITE_END()
