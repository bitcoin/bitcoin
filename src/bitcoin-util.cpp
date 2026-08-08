// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/license_info.h>
#include <common/system.h>
#include <compat/compat.h>
#include <core_io.h>
#include <policy/policy.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <streams.h>
#include <univalue.h>
#include <util/check.h>
#include <util/exception.h>
#include <util/expected.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <thread>

static const int CONTINUE_EXECUTION=-1;

const TranslateFn G_TRANSLATION_FUN{nullptr};

static void SetupBitcoinUtilArgs(ArgsManager &argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    // evalscript options
    auto get_scriptflags = []() -> std::string {
        std::string r;
        for (const auto& [k, v] : ScriptFlagNamesToEnum()) {
            if (!r.empty()) r += ",";
            r += k;
        }
        return r;
    };

    argsman.AddArg("-sigversion", "Specify a script sigversion (base, witness_v0, tapscript; default: witness_v0).", ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);
    argsman.AddArg("-scriptflags", strprintf("Specify SCRIPT_VERIFY flags (NONE, MANDATORY, STANDARD, or a selection from %s; default: MANDATORY).", get_scriptflags()), ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);
    argsman.AddArg("-tx", "The tx (hex encoded). Required for meaningful validation of CHECKSIG, CHECKLOCKTIMEVERIFY and CHECKSEQUENCEVERIFY.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);
    argsman.AddArg("-input", "The index of the input being spent", ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);
    argsman.AddArg("-spentoutput", "The spent prevouts (hex encoded TxOut, ie nValue, scriptPubKey). May be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);

    argsman.AddCommand("grind", "Perform proof of work on hex header string");
    argsman.AddCommand("getchainparams", "Get hardcoded parameters for the selected chain");
    argsman.AddCommand("evalscript", "Interpret a bitcoin script", {"-sigversion", "-scriptflags", "-tx", "-input", "-spentoutput"});

    SetupChainParamsBaseOptions(argsman);
}

// This function returns either one of EXIT_ codes when it's expected to stop the process or
// CONTINUE_EXECUTION when it's expected to continue further.
static int AppInitUtil(ArgsManager& args, int argc, char* argv[])
{
    SetupBitcoinUtilArgs(args);
    std::string error;
    if (!args.ParseParameters(argc, argv, error)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error);
        return EXIT_FAILURE;
    }

    if (HelpRequested(args) || args.GetBoolArg("-version", false)) {
        // First part of help message is specific to this utility
        std::string strUsage = CLIENT_NAME " bitcoin-util utility version " + FormatFullVersion() + "\n";

        if (args.GetBoolArg("-version", false)) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n"
                "The bitcoin-util tool provides bitcoin related functionality that does not rely on the ability to access a running node. Available [commands] are listed below.\n"
                "\n"
                "Usage:  bitcoin-util [options] [command]\n"
                "or:     bitcoin-util [options] grind <hex-block-header>\n";
            strUsage += "\n" + args.GetHelpMessage();
        }

        tfm::format(std::cout, "%s", strUsage);

        if (argc < 2) {
            tfm::format(std::cerr, "Error: too few parameters\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // Check for chain settings (Params() calls are only valid after this clause)
    try {
        SelectParams(args.GetChainType());
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return CONTINUE_EXECUTION;
}

static void grind_task(uint32_t nBits, CBlockHeader header, uint32_t offset, uint32_t step, std::atomic<bool>& found, uint32_t& proposed_nonce)
{
    arith_uint256 target;
    bool neg, over;
    target.SetCompact(nBits, &neg, &over);
    if (target == 0 || neg || over) return;
    header.nNonce = offset;

    uint32_t finish = std::numeric_limits<uint32_t>::max() - step;
    finish = finish - (finish % step) + offset;

    while (!found && header.nNonce < finish) {
        const uint32_t next = (finish - header.nNonce < 5000*step) ? finish : header.nNonce + 5000*step;
        do {
            if (UintToArith256(header.GetHash()) <= target) {
                if (!found.exchange(true)) {
                    proposed_nonce = header.nNonce;
                }
                return;
            }
            header.nNonce += step;
        } while(header.nNonce != next);
    }
}

static int Grind(const std::vector<std::string>& args, std::string& strPrint)
{
    if (args.size() != 1) {
        strPrint = "Must specify block header to grind";
        return EXIT_FAILURE;
    }

    CBlockHeader header;
    if (!DecodeHexBlockHeader(header, args[0])) {
        strPrint = "Could not decode block header";
        return EXIT_FAILURE;
    }

    uint32_t nBits = header.nBits;
    std::atomic<bool> found{false};
    uint32_t proposed_nonce{};

    std::vector<std::thread> threads;
    int n_tasks = std::max(1u, std::thread::hardware_concurrency());
    threads.reserve(n_tasks);
    for (int i = 0; i < n_tasks; ++i) {
        threads.emplace_back(grind_task, nBits, header, i, n_tasks, std::ref(found), std::ref(proposed_nonce));
    }
    for (auto& t : threads) {
        t.join();
    }
    if (found) {
        header.nNonce = proposed_nonce;
    } else {
        strPrint = "Could not satisfy difficulty target";
        return EXIT_FAILURE;
    }

    DataStream ss{};
    ss << header;
    strPrint = HexStr(ss);
    return EXIT_SUCCESS;
}

static int GetChainParams(const std::vector<std::string>& args, std::string& strPrint)
{
    if (!args.empty()) {
        strPrint = "getchainparams does not take arguments";
        return EXIT_FAILURE;
    }

    const auto& params = Params();
    const auto& consensus = params.GetConsensus();

    UniValue result{UniValue::VOBJ};
    result.pushKV("chain", params.GetChainTypeString());
    result.pushKV("test_chain", params.IsTestChain());
    result.pushKV("genesis", HexStr(consensus.hashGenesisBlock));
    result.pushKV("subsidy_halving_interval", consensus.nSubsidyHalvingInterval);

    if (consensus.signet_blocks) {
        UniValue signet{UniValue::VOBJ};
        signet.pushKV("challenge", HexStr(consensus.signet_challenge));
        result.pushKV("signet", signet);
    }

    {
        UniValue pow{UniValue::VOBJ};
        pow.pushKV("limit", consensus.powLimit.ToString());
        if (!consensus.fPowNoRetargeting) {
            pow.pushKV("target_spacing", TicksSeconds(consensus.PowTargetSpacing()));
            pow.pushKV("difficulty_retarget_interval", consensus.DifficultyAdjustmentInterval());
            std::string mindiff_blocks = (consensus.fPowAllowMinDifficultyBlocks ?
                  (consensus.enforce_BIP94 ? "bip94" : "yes") : "no");
            pow.pushKV("mindiff_blocks", mindiff_blocks);
        }
        result.pushKV("pow", pow);
    }

    {
        UniValue net{UniValue::VOBJ};
        net.pushKV("default_port", params.GetDefaultPort());
        net.pushKV("magic", HexStr(params.MessageStart()));
        UniValue dns{UniValue::VARR};
        for (const auto& seed : params.DNSSeeds()) {
            dns.push_back(seed);
        }
        net.pushKV("dns_seeds", dns);
        result.pushKV("net", net);
    }

    {
        UniValue addr{UniValue::VOBJ};
        addr.pushKV("bech32_hrp", params.Bech32HRP());
        result.pushKV("addresses", addr);
    }

    strPrint = result.write(/*prettyIndent=*/2);
    return EXIT_SUCCESS;
}

static UniValue stack2uv(const std::vector<std::vector<unsigned char>>& stack)
{
    UniValue result{UniValue::VARR};
    for (const auto& v : stack) {
        result.push_back(HexStr(v));
    }
    return result;
}

static std::string sigver2str(SigVersion sigver)
{
    switch (sigver) {
    case SigVersion::BASE: return "base";
    case SigVersion::WITNESS_V0: return "witness_v0";
    case SigVersion::TAPROOT: return "taproot";
    case SigVersion::TAPSCRIPT: return "tapscript";
    }
    return "unknown";
}

static util::Expected<script_verify_flags, std::string> parse_verify_flags(const std::string& strFlags)
{
    if (strFlags.empty() || strFlags == "MANDATORY") return MANDATORY_SCRIPT_VERIFY_FLAGS;
    if (strFlags == "STANDARD") return STANDARD_SCRIPT_VERIFY_FLAGS;
    if (strFlags == "NONE") return SCRIPT_VERIFY_NONE;

    script_verify_flags flags{0};
    std::vector<std::string> words = util::SplitString(strFlags, ',');

    const auto& flag_names = ScriptFlagNamesToEnum();
    for (const std::string& word : words) {
        auto it = flag_names.find(word);
        if (it == flag_names.end()) {
            return util::Unexpected{strprintf("Unknown script flag: %s", word)};
        }
        flags |= it->second;
    }
    return flags;
}

static int EvalScript(const ArgsManager& argsman, const std::vector<std::string>& args, std::string& strPrint)
{
    UniValue result{UniValue::VOBJ};

    std::unique_ptr<const CTransaction> txTo;
    PrecomputedTransactionData txdata;
    ScriptExecutionData execdata;

    std::unique_ptr<BaseSignatureChecker> tx_checker; // for cleanup of checker if needed
    const BaseSignatureChecker* checker = &DUMMY_CHECKER;

    SigVersion sigversion = SigVersion::WITNESS_V0;

    if (const auto verstr = argsman.GetArg("-sigversion"); verstr.has_value()) {
        if (*verstr == "base") {
            sigversion = SigVersion::BASE;
        } else if (*verstr == "witness_v0") {
            sigversion = SigVersion::WITNESS_V0;
        } else if (*verstr == "tapscript") {
            sigversion = SigVersion::TAPSCRIPT;
        } else {
            strPrint = strprintf("Unknown -sigversion=%s", *verstr);
            return EXIT_FAILURE;
        }
    }

    auto exp_flags = parse_verify_flags(argsman.GetArg("-scriptflags", ""));
    if (!exp_flags) {
        strPrint = std::move(exp_flags).error();
        return EXIT_FAILURE;
    }
    const script_verify_flags flags = *exp_flags;

    CScript script{};
    std::vector<std::vector<unsigned char>> stack{};
    if (!args.empty()) {
        if (IsHex(args[0])) {
            auto h = ParseHex(args[0]);
            script = CScript(h.begin(), h.end());
        } else {
            script = ParseScript(args[0]);
        }

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].empty()) {
                stack.emplace_back();
            } else if (IsHex(args[i])) {
                stack.push_back(ParseHex(args[i]));
            } else {
                strPrint = strprintf("Initial stack element not valid hex: %s", args[i]);
                return EXIT_FAILURE;
            }
        }
    }

    if (const auto txhex = argsman.GetArg("-tx"); txhex.has_value()) {
        const int64_t input{argsman.GetIntArg("-input", 0)};
        const auto spent_outputs_hex{argsman.GetArgs("-spentoutput")};

        CMutableTransaction mut_tx;
        if (!DecodeHexTx(mut_tx, *txhex)) {
            strPrint = "Could not decode transaction from -tx argument";
            return EXIT_FAILURE;
        }
        txTo = std::make_unique<CTransaction>(mut_tx);

        if (spent_outputs_hex.size() != txTo->vin.size()) {
            strPrint = "When -tx is specified, must specify exactly one -spentoutput for each input";
            return EXIT_FAILURE;
        }

        std::vector<CTxOut> spent_outputs;
        for (const auto& outhex : spent_outputs_hex) {
            bool ok = false;
            if (IsHex(outhex)) {
                CTxOut txout;
                std::vector<unsigned char> out(ParseHex(outhex));
                DataStream ss(out);
                try {
                    ss >> txout;
                    if (ss.empty()) {
                        spent_outputs.push_back(txout);
                        ok = true;
                    }
                } catch (const std::exception&) {
                    // fall through
                }
            }
            if (!ok) {
                strPrint = strprintf("Could not parse -spentoutput=%s", outhex);
                return EXIT_FAILURE;
            }
        }

        const bool input_in_range = input >= 0 && static_cast<size_t>(input) < spent_outputs.size();
        if (!input_in_range) {
            strPrint = strprintf("Invalid -input=%d", input);
            return EXIT_FAILURE;
        }

        CAmount amount = spent_outputs.at(input).nValue;
        txdata.Init(*txTo, std::move(spent_outputs), /*force=*/true);
        tx_checker = std::make_unique<TransactionSignatureChecker>(txTo.get(), input, amount, txdata, MissingDataBehavior::ASSERT_FAIL);
        checker = tx_checker.get();

        if (sigversion == SigVersion::TAPSCRIPT) {
            const CTxIn& txin = txTo->vin.at(input);
            execdata.m_annex_present = false;
            if (txin.scriptWitness.stack.size() <= 1) {
                // either key path spend or no witness, so nothing to do here
            } else {
                const auto& top = txin.scriptWitness.stack.back();
                if (!top.empty() && top.at(0) == ANNEX_TAG) {
                    execdata.m_annex_hash = (HashWriter{} << top).GetSHA256();
                    execdata.m_annex_present = true;
                }
            }
            execdata.m_annex_init = true;
            execdata.m_tapleaf_hash = ComputeTapleafHash(TAPROOT_LEAF_TAPSCRIPT, script);
            execdata.m_tapleaf_hash_init = true;
            // we do not set execdata.m_validation_weight_left from the
            // tx here, because the provided tx data may be incomplete and
            // not actually match the script that we've been provided

        }
    }

    if (sigversion == SigVersion::TAPSCRIPT) {
        if (!execdata.m_annex_init) {
            execdata.m_annex_present = false;
            execdata.m_annex_init = true;
        }
        if (!execdata.m_tapleaf_hash_init) {
            execdata.m_tapleaf_hash = uint256::ZERO;
            execdata.m_tapleaf_hash_init = true;
        }
        assert(!execdata.m_validation_weight_left_init);
        // approximation of the consensus value; should include control block and annex
        execdata.m_validation_weight_left = ::GetSerializeSize(stack) + ::GetSerializeSize(script) + VALIDATION_WEIGHT_OFFSET;
        execdata.m_validation_weight_left_init = true;
    }

    ScriptError serror{};

    UniValue uv_flags{UniValue::VARR};
    for (const auto& el : GetScriptFlagNames(flags)) {
        uv_flags.push_back(el);
    }
    UniValue uv_script{UniValue::VOBJ};
    ScriptToUniv(script, uv_script);
    result.pushKV("script", uv_script);
    result.pushKV("sigversion", sigver2str(sigversion));
    result.pushKV("script_flags", uv_flags);
    result.pushKV("dummy_sig_checker", (checker == &DUMMY_CHECKER));

    std::optional<bool> opsuccess_check;
    if (sigversion == SigVersion::TAPSCRIPT) {
        opsuccess_check = CheckTapscriptOpSuccess(script, flags, &serror);

        bool opsuccess_found = opsuccess_check.has_value() && (*opsuccess_check || serror == SCRIPT_ERR_DISCOURAGE_OP_SUCCESS);
        result.pushKV("opsuccess_found", opsuccess_found);
    }

    bool success{true};
    if (opsuccess_check.has_value()) {
        success = *opsuccess_check;
    } else {
        if (sigversion == SigVersion::TAPSCRIPT && stack.size() > MAX_STACK_SIZE) {
            // Tapscript enforces initial stack size limits
            serror = SCRIPT_ERR_STACK_SIZE;
            success = false;
        } else if (sigversion == SigVersion::WITNESS_V0 || sigversion == SigVersion::TAPSCRIPT) {
            // Disallow stack item size > MAX_SCRIPT_ELEMENT_SIZE in witness stack
            for (const auto& elem : stack) {
                if (elem.size() > MAX_SCRIPT_ELEMENT_SIZE) {
                    serror = SCRIPT_ERR_PUSH_SIZE;
                    success = false;
                    break;
                }
            }
        }
        if (success) {
            success = EvalScript(stack, script, flags, *Assert(checker), sigversion, execdata, &serror);
        }
        if (success) {
            if (stack.size() != 1 && (sigversion == SigVersion::WITNESS_V0 || sigversion == SigVersion::TAPSCRIPT)) {
                serror = SCRIPT_ERR_CLEANSTACK;
                success = false;
            } else if (stack.empty() || !CastToBool(stack.back())) {
                serror = SCRIPT_ERR_EVAL_FALSE;
                success = false;
            } else if (stack.size() > 1 && (flags & SCRIPT_VERIFY_CLEANSTACK) != 0) {
                serror = SCRIPT_ERR_CLEANSTACK;
                success = false;
            }
        }
    }

    result.pushKV("stack_after", stack2uv(stack));

    auto get_sigop_cost = [&]() -> int64_t {
        switch (sigversion) {
        case SigVersion::TAPROOT:
        case SigVersion::TAPSCRIPT:
            return 0;
        case SigVersion::WITNESS_V0:
            return script.GetSigOpCount(true);
        case SigVersion::BASE:
            return script.GetSigOpCount(false) * WITNESS_SCALE_FACTOR;
        }
        Assume(false); // unreachable
        return 0;
    };
    result.pushKV("sigop_cost", get_sigop_cost());

    result.pushKV("success", success);
    if (!success) {
        result.pushKV("error", ScriptErrorString(serror));
    }

    strPrint = result.write(2);

    return EXIT_SUCCESS;
}

MAIN_FUNCTION
{
    ArgsManager& args = gArgs;
    SetupEnvironment();

    try {
        int ret = AppInitUtil(args, argc, argv);
        if (ret != CONTINUE_EXECUTION) {
            return ret;
        }
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInitUtil()");
        return EXIT_FAILURE;
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInitUtil()");
        return EXIT_FAILURE;
    }

    const auto cmd = args.GetCommand();
    if (!cmd) {
        tfm::format(std::cerr, "Error: must specify a command\n");
        return EXIT_FAILURE;
    }

    int ret = EXIT_FAILURE;
    std::string strPrint;
    try {
        if (cmd->command == "grind") {
            ret = Grind(cmd->args, strPrint);
        } else if (cmd->command == "getchainparams") {
            ret = GetChainParams(cmd->args, strPrint);
        } else if (cmd->command == "evalscript") {
            ret = EvalScript(args, cmd->args, strPrint);
        } else {
            assert(false); // unknown command should be caught earlier
        }
    } catch (const std::exception& e) {
        strPrint = std::string("error: ") + e.what();
    } catch (...) {
        strPrint = "unknown error";
    }

    if (strPrint != "") {
        tfm::format(ret == 0 ? std::cout : std::cerr, "%s\n", strPrint);
    }

    return ret;
}
