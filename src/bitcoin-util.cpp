// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <coins.h>
#include <common/args.h>
#include <common/license_info.h>
#include <common/system.h>
#include <compat/compat.h>
#include <core_io.h>
#include <crypto/muhash.h>
#include <crypto/sha256.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/exception.h>
#include <util/fs.h>
#include <util/overflow.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <utxo_snapshot.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

static const int CONTINUE_EXECUTION=-1;

const TranslateFn G_TRANSLATION_FUN{nullptr};

static void SetupBitcoinUtilArgs(ArgsManager &argsman)
{
    SetupHelpOptions(argsman);

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    argsman.AddCommand("grind", "Perform proof of work on hex header string");
    argsman.AddCommand("getchainparams", "Get hardcoded parameters for the selected chain");

    argsman.AddArg("-muhash", "Also compute the muhash, which costs a 3072 bit multiply per coin "
                              "rather than one hash (default: 1)",
                   ArgsManager::ALLOW_ANY, OptionsCategory::COMMAND_OPTIONS);
    argsman.AddCommand("txoutsetsummary", "Report what a UTXO set snapshot file holds, as JSON",
                       {"-muhash"});

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
    result.pushKV("genesis", consensus.hashGenesisBlock.GetHex());
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

/** Queue capacity, in batches, so the reader can get ahead of the hashers
 * if disk reads are bursty. A single batch is up to SNAPSHOT_ITEMS_PER_BATCH
 * coins or SNAPSHOT_BATCH_BYTES of decoded data.
 */
static constexpr size_t QUEUE_BATCHES{4};

/**
 * Report a summary of a serialized UTXO set snapshot (from dumptxoutset RPC).
 * Includes its base block hash, how many coins were read (counted,
 * not taken from the header), their total value, and the set commitment hashes.
 *
 * The snapshot is read from the given file, or from stdin if no file is given
 * or the file is "-". The muhash costs far more than the rest, so it can
 * be skipped: -muhash (default 1, -nomuhash to leave it out).
 */
static int TxoutSetSummary(const std::vector<std::string>& args, std::string& strPrint)
{
    if (args.size() > 1) {
        strPrint = "Must specify at most one UTXO set snapshot file";
        return EXIT_FAILURE;
    }

    const bool from_stdin{args.empty() || args[0] == "-"};
    // Wrapping stdin this way makes the destructor close it, which is harmless
    // for a command that exits immediately afterwards.
    AutoFile file{from_stdin ? stdin : fsbridge::fopen(fs::u8path(args[0]), "rb")};
    if (file.IsNull()) {
        strPrint = strprintf("Could not open %s", from_stdin ? "stdin" : args[0]);
        return EXIT_FAILURE;
    }

    SnapshotMetadata metadata;
    try {
        file >> metadata;
        // We accept any network magic here.
    } catch (const std::ios_base::failure& e) {
        strPrint = strprintf("Could not read snapshot metadata: %s", e.what());
        return EXIT_FAILURE;
    }
    const uint64_t coins_count{metadata.m_coins_count};
    const uint256& base_blockhash{metadata.m_base_blockhash};
    auto network{GetNetworkForMagic(metadata.m_network_magic)};
    const std::string network_name{network.has_value() ? ChainTypeToString(*network) : strprintf("magic=%s", HexStr(metadata.m_network_magic))};

    const bool want_muhash{gArgs.GetBoolArg("-muhash", true)};
    const size_t num_threads{std::max(1u, std::thread::hardware_concurrency())};
    SnapshotQueue muhash_queue{QUEUE_BATCHES + num_threads - 1};
    std::vector<MuHash3072> accumulators(num_threads);
    std::vector<std::thread> muhash_threads;
    if (want_muhash) {
        muhash_threads.reserve(num_threads);
        for (size_t i{0}; i < num_threads; ++i) {
            muhash_threads.emplace_back(MuhashSnapshotBatches, std::ref(muhash_queue), std::ref(accumulators[i]));
        }
    }

    // hash_serialized_3 is a stream hash, so it is taken by one consumer in file
    // order; the muhash is a product and can be split across the workers.
    SnapshotQueue hash_queue{QUEUE_BATCHES};
    uint256 hash_serialized_3;
    std::thread stream_hash_thread{HashSnapshotBatches, std::ref(hash_queue),
                                   std::ref(hash_serialized_3)};

    std::optional<CAmount> total_amount{0};
    static constexpr uint64_t PROGRESS_INTERVAL{10000000};
    uint64_t coins_done{0};
    uint64_t coins_reported{0};
    std::optional<bilingual_str> read_error;
    {
        const auto res{ReadSnapshotBatches(file, coins_count,
            [&](const COutPoint&, const Coin& coin, uint64_t) -> util::Result<void> {
                total_amount = CheckedAdd(*total_amount, coin.out.nValue);
                if (!total_amount) {
                    return util::Error{Untranslated("The total value of the coins overflows")};
                }
                return {};
            },
            [&](SnapshotBatch batch, uint64_t read) {
                hash_queue.Push(batch);
                if (want_muhash) muhash_queue.Push(batch);
                coins_done = read;
                if (coins_done - coins_reported >= PROGRESS_INTERVAL) {
                    coins_reported = coins_done;
                    tfm::format(std::cerr, "read %.1fM of %.1fM coins\n",
                                coins_done / 1000000.0, coins_count / 1000000.0);
                }
            })};
        if (!res) read_error = util::ErrorString(res);
    }
    hash_queue.Finish();
    stream_hash_thread.join();
    muhash_queue.Finish();
    for (auto& thread : muhash_threads) {
        thread.join();
    }
    if (read_error) {
        strPrint = read_error->original;
        return EXIT_FAILURE;
    }

    UniValue summary{UniValue::VOBJ};
    summary.pushKV("chain", network_name);
    summary.pushKV("bestblock", base_blockhash.GetHex());
    summary.pushKV("txouts", static_cast<int64_t>(coins_done));
    summary.pushKV("total_amount", ValueFromAmount(*total_amount));
    summary.pushKV("hash_serialized_3", hash_serialized_3.GetHex());
    if (want_muhash) {
        MuHash3072 muhash;
        for (const auto& accumulator : accumulators) {
            muhash *= accumulator;
        }
        uint256 muhash_result;
        muhash.Finalize(muhash_result);
        summary.pushKV("muhash", muhash_result.GetHex());
    }
    strPrint = summary.write(2);
    return EXIT_SUCCESS;
}

MAIN_FUNCTION
{
    ArgsManager& args = gArgs;
    SetupEnvironment();
    SHA256AutoDetect();

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
        } else if (cmd->command == "txoutsetsummary") {
            ret = TxoutSetSummary(cmd->args, strPrint);
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
