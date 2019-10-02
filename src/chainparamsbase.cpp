// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparamsbase.h>

#include <tinyformat.h>
#include <util/system.h>
#include <util/memory.h>

#include <assert.h>

const std::string CBaseChainParams::MAIN = "main";
const std::string CBaseChainParams::TESTNET = "test";
const std::string CBaseChainParams::REGTEST = "regtest";

void SetupChainParamsBaseOptions()
{
    gArgs.AddArg("-chain=<chain>", "Use the chain <chain> (default: main). Reserved values: main, test, regtest. With any other value, a custom chain is used. All regtest-only options are available in custom chains too.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-regtest", "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
                 "This is intended for regression testing tools and app development. Equivalent to -chain=regtest.", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-segwitheight=<n>", "Set the activation height of segwit. -1 to disable. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-testnet", "Use the test chain. Equivalent to -chain=test.", ArgsManager::ALLOW_ANY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-vbparams=deployment:start:end", "Use given start/end times for specified version bits deployment (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_nsubsidyhalvinginterval", "Number of blocks between one subsidy adjustment and the next one. Default: 150 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_bip16exception", "A block hash not to validate BIP16 on. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_bip34height", "Height from which BIP34 is enforced. Default: 500 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_bip34hash", "Hardcoded hash for BIP34 activation corresponding to the bip34height so that bip30 checks can be saved. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_bip65height", "Height from which BIP65 is enforced. Default: 1351 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_bip66height", "Height from which BIP66 is enforced. Default: 1251 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_csvheight", "Height from which CSV is enforced. Default: 432 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_minbip9warningheight", "Height from which BIP9 warnings are used. Default: 0 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_powlimit", "Maximum proof of work target. Default 7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_npowtargettimespan", "Proof of work retargetting interval in seconds. Default: 2 weeks (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_npowtargetspacing", "Proof of work target for interval between blocks in seconds. Default: 600 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_fpowallowmindifficultyblocks", "Whether the chain allows minimum difficulty blocks or not. Default: 1 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_fpownoretargeting", "Whether the chain skips proof of work retargetting or not. Default: 1 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_nminerconfirmationwindow", "Interval for BIP9 deployment activation. Default: 144 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_nrulechangeactivationthreshold", "Minimum blocks to signal readiness for a chain for BIP9 activation. Default 108 (ie 75%). (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_nminimumchainwork", "The best chain should have at least this much work. Default: 0 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-con_defaultassumevalid", "By default assume that the signatures in ancestors of this block are valid. Consider using -assumevalid instead. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-ndefaultport", "The port to listen for connections on by default. Consider using -port instead of changing the default.  Default: 18444 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-npruneafterheight", "Only start prunning after this height. Default: 1000 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-assumed_blockchain_size", "Estimated current blockchain size (in GB) for UI purposes. Default 0 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-assumed_chain_state_size", "Estimated current chain state size (in GB) for UI purposes. Default 0 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-fdefaultconsistencychecks", "Whether -checkblockindex and -checkmempool are active by default or not. Consider using those options instead. Default: 1 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-frequirestandard", "Whether standard policy rules are applied in the local mempool by default. Consider using -acceptnonstdtxn=0 instead of changing the default. Default: 0 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-is_test_chain", "Whether it's allowed to set -acceptnonstdtxn=0 for this chain or not. Default: 1 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-bech32_hrp", "Human readable part for bech32 addresses. See BIP173 for more info. Default: bcrt (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-pubkeyprefix", "Magic for base58 pubkeys. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-scriptprefix", "Magic for base58 scripts. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-secretprefix", "Magic for base58 secret keys. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-extpubkeyprefix", "Magic for base58 external pubkeys. Default: 043587CF (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-extprvkeyprefix", "Magic for base58 external secret keys. Default: 04358394 (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-pchmessagestart", "Magic for p2p protocol. Default: FABFB5DA (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
    gArgs.AddArg("-seednode=<ip>", "Use specified node as seed node. This option can be specified multiple times to connect to multiple nodes. (regtest-only)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CHAINPARAMS);
}

static std::unique_ptr<CBaseChainParams> globalChainBaseParams;

const CBaseChainParams& BaseParams()
{
    assert(globalChainBaseParams);
    return *globalChainBaseParams;
}

std::unique_ptr<CBaseChainParams> CreateBaseChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return MakeUnique<CBaseChainParams>("", 8332);
    else if (chain == CBaseChainParams::TESTNET)
        return MakeUnique<CBaseChainParams>("testnet3", 18332);
    else if (chain == CBaseChainParams::REGTEST)
        return MakeUnique<CBaseChainParams>("regtest", 18443);

    return MakeUnique<CBaseChainParams>(chain, 18553);
}

void SelectBaseParams(const std::string& chain)
{
    globalChainBaseParams = CreateBaseChainParams(chain);
    gArgs.SelectConfigNetwork(chain);
}
