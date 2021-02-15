// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "params.hpp"
#include "util.hpp"
#include <boost/algorithm/string.hpp>
#include <chainparams.h>
#include <pow.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <vbk/bootstraps.h>
#include <vbk/pop_common.hpp>
#include <veriblock/bootstraps.hpp>


namespace VeriBlock {

bool AltChainParamsVBTC::checkBlockHeader(const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& root, altintegration::ValidationState& state) const noexcept
{
    const CChainParams& params = Params();

    try {
        // this throws
        auto header = VeriBlock::headerFromBytes(bytes);

        /* top level merkle `root` calculated by library is same as in endorsed header */
        auto actual = header.hashMerkleRoot.asVector();
        if(actual != root) {
            return state.Invalid("bad-merkle-root", strprintf("Expected %s, got %s", HexStr(root), HexStr(actual)));
        }

        /* and POW of endorsed header is valid */
        if(!CheckProofOfWork(header.GetHash(), header.nBits, params.GetConsensus())) {
            return state.Invalid("bad-pow", "Bad proof of work");
        }

        return true;
    } catch (...) {
        return state.Invalid("bad-header", "Can not parse block header");
    }
}


std::vector<uint8_t> AltChainParamsVBTC::getHash(const std::vector<uint8_t>& bytes) const noexcept
{
    try {
        return VeriBlock::headerFromBytes(bytes).GetHash().asVector();
    } catch (...) {
        // return empty hash, since we can't deserialize header
        return {};
    }
}

static std::vector<std::string> parseBlocks(const std::string& s)
{
    std::vector<std::string> strs;
    boost::split(strs, s, boost::is_any_of(","));
    return strs;
}

void printConfig(const altintegration::Config& config)
{
    std::string btclast = config.btc.blocks.empty() ? "<empty>" : config.btc.blocks.rbegin()->getHash().toHex();
    std::string btcfirst = config.btc.blocks.empty() ? "<empty>" : config.btc.blocks.begin()->getHash().toHex();
    std::string vbklast = config.vbk.blocks.empty() ? "<empty>" : config.vbk.blocks.rbegin()->getHash().toHex();
    std::string vbkfirst = config.vbk.blocks.empty() ? "<empty>" : config.vbk.blocks.begin()->getHash().toHex();


    assert(config.alt);

    LogPrintf(R"(Applied POP config:
 BTC:
  network     : %s
  startHeight : %d
  total blocks: %d
  first       : %s
  last        : %s

 VBK:
  network     : %s
  startHeight : %d
  total blocks: %d
  first       : %s
  last        : %s

 ALT:
  network     : %s
  block height: %d
  block hash  : %s
  chain id    : %lld
)",
        config.btc.params->networkName(), config.btc.startHeight, config.btc.blocks.size(), btcfirst, btclast,
        config.vbk.params->networkName(), config.vbk.startHeight, config.vbk.blocks.size(), vbkfirst, vbklast,

        Params().NetworkIDString(), config.alt->getBootstrapBlock().height,
        HexStr(config.alt->getBootstrapBlock().hash),
        config.alt->getIdentifier());
}

void selectPopConfig(
    const std::string& btcnet,
    const std::string& vbknet,
    bool popautoconfig,
    int btcstart,
    const std::string& btcblocks,
    int vbkstart,
    const std::string& vbkblocks)
{
    altintegration::Config popconfig;

    //! SET BTC
    if (btcnet == "test") {
        auto param = std::make_shared<altintegration::BtcChainParamsTest>();
        if (popautoconfig) {
            popconfig.setBTC(testnetBTCstartHeight, testnetBTCblocks, param);
        } else {
            popconfig.setBTC(btcstart, parseBlocks(btcblocks), param);
        }
    } else if (btcnet == "regtest") {
        auto param = std::make_shared<altintegration::BtcChainParamsRegTest>();
        if (popautoconfig) {
            popconfig.setBTC(0, {}, param);
        } else {
            popconfig.setBTC(btcstart, parseBlocks(btcblocks), param);
        }
    } else {
        throw std::invalid_argument("btcnet currently only supports test/regtest");
    }

    //! SET VBK
    if (vbknet == "test") {
        auto param = std::make_shared<altintegration::VbkChainParamsTest>();
        if (popautoconfig) {
            popconfig.setVBK(testnetVBKstartHeight, testnetVBKblocks, param);
        } else {
            popconfig.setVBK(vbkstart, parseBlocks(vbkblocks), param);
        }
    } else if (vbknet == "regtest") {
        auto param = std::make_shared<altintegration::VbkChainParamsRegTest>();
        if (popautoconfig) {
            popconfig.setVBK(0, {}, param);
        } else {
            popconfig.setVBK(vbkstart, parseBlocks(vbkblocks), param);
        }
    } else {
        throw std::invalid_argument("vbknet currently only supports test/regtest");
    }

    auto altparams = std::make_shared<VeriBlock::AltChainParamsVBTC>(Params().GenesisBlock());
    popconfig.alt = altparams;
    VeriBlock::SetPopConfig(popconfig);
    printConfig(popconfig);
}

void selectPopConfig(const ArgsManager& args, std::string vbk, std::string btc)
{
    std::string btcnet = args.GetArg("-popbtcnetwork", btc);
    std::string vbknet = args.GetArg("-popvbknetwork", vbk);
    int popautoconfig = args.GetArg("-popautoconfig", 1);
    int btcstart = args.GetArg("-popbtcstartheight", 0);
    std::string btcblocks = args.GetArg("-popbtcblocks", "");
    int vbkstart = args.GetArg("-popvbkstartheight", 0);
    std::string vbkblocks = args.GetArg("-popvbkblocks", "");

    selectPopConfig(btcnet, vbknet, (bool)popautoconfig, btcstart, btcblocks, vbkstart, vbkblocks);
}

} // namespace VeriBlock
