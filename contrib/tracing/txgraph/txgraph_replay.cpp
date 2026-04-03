// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/** Standalone tool that replays a TxGraph binary trace file (TXGTRACE format)
 *  and reports per-entry-point timing statistics.
 *
 *  The trace file is produced by txgraph_trace_recorder.py (BCC/USDT) or any
 *  tool that emits the TXGTRACE binary format.
 *
 *  Usage: txgraph-replay <trace_file>
 */

#include <txgraph.h>
#include <util/translation.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

const TranslateFn G_TRANSLATION_FUN{nullptr};

// ---------------------------------------------------------------------------
// Trace opcodes (must match txgraph_trace_recorder.py)
// ---------------------------------------------------------------------------

enum Op : uint8_t {
    OP_INIT                      = 0x00,
    OP_ADD_TRANSACTION           = 0x01,
    OP_REMOVE_TRANSACTION        = 0x02,
    OP_ADD_DEPENDENCY            = 0x03,
    OP_SET_TRANSACTION_FEE       = 0x04,
    OP_UNLINK_REF                = 0x05,
    OP_GET_BLOCK_BUILDER         = 0x10,
    OP_DO_WORK                   = 0x11,
    OP_COMPARE_MAIN_ORDER        = 0x12,
    OP_GET_ANCESTORS             = 0x13,
    OP_GET_DESCENDANTS           = 0x14,
    OP_GET_ANCESTORS_UNION       = 0x15,
    OP_GET_DESCENDANTS_UNION     = 0x16,
    OP_GET_CLUSTER               = 0x17,
    OP_GET_MAIN_CHUNK_FEERATE    = 0x18,
    OP_COUNT_DISTINCT_CLUSTERS   = 0x19,
    OP_GET_MAIN_MEMORY_USAGE     = 0x1a,
    OP_GET_WORST_MAIN_CHUNK      = 0x1b,
    OP_GET_MAIN_STAGING_DIAGRAMS = 0x1c,
    OP_TRIM                      = 0x1d,
    OP_IS_OVERSIZED              = 0x1e,
    OP_EXISTS                    = 0x1f,
    OP_START_STAGING             = 0x20,
    OP_ABORT_STAGING             = 0x21,
    OP_COMMIT_STAGING            = 0x22,
    OP_GET_INDIVIDUAL_FEERATE    = 0x23,
    OP_GET_TRANSACTION_COUNT     = 0x24,
};

// ---------------------------------------------------------------------------
// I/O helpers (little-endian)
// ---------------------------------------------------------------------------

static bool ReadBytes(std::ifstream& f, void* buf, size_t n)
{
    return f.read(reinterpret_cast<char*>(buf), n).good();
}

template <typename T>
static bool ReadLE(std::ifstream& f, T& out)
{
    uint8_t buf[sizeof(T)];
    if (!ReadBytes(f, buf, sizeof(T))) return false;
    out = T{};
    for (size_t i = 0; i < sizeof(T); ++i) {
        out |= T(buf[i]) << (8 * i);
    }
    return true;
}

static bool ReadLE_i64(std::ifstream& f, int64_t& out)
{
    uint64_t u{};
    if (!ReadLE(f, u)) return false;
    memcpy(&out, &u, sizeof(out));
    return true;
}

static bool ReadLE_i32(std::ifstream& f, int32_t& out)
{
    uint32_t u{};
    if (!ReadLE(f, u)) return false;
    memcpy(&out, &u, sizeof(out));
    return true;
}

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

struct TimingStats {
    const char* name{nullptr};
    uint64_t calls{0};
    uint64_t total_us{0};
};

using Clock = std::chrono::steady_clock;

static uint64_t ElapsedUs(Clock::time_point start)
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
}

// ---------------------------------------------------------------------------
// Helper: read variable-length indices (count | level | idx[0..count-1])
// ---------------------------------------------------------------------------

struct VarLenArgs {
    std::vector<uint32_t> indices;
    uint8_t level;
};

/** Read the payload for variable-length ops.  Format: u32 count, u8 level, u32[count] indices. */
static bool ReadVarLen(std::ifstream& f, VarLenArgs& out)
{
    uint32_t count{};
    if (!ReadLE(f, count)) return false;
    if (!ReadBytes(f, &out.level, 1)) return false;
    out.indices.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!ReadLE(f, out.indices[i])) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <trace_file>" << std::endl;
        return 1;
    }

    std::ifstream f(argv[1], std::ios::binary);
    if (!f) {
        std::cerr << "Error: cannot open '" << argv[1] << "'" << std::endl;
        return 1;
    }

    // Validate header
    uint8_t magic[8];
    if (!ReadBytes(f, magic, 8)) {
        std::cerr << "Error: file too short for magic" << std::endl;
        f.close();
        return 1;
    }
    static const uint8_t expected_magic[8] = {'T','X','G','T','R','A','C','E'};
    if (memcmp(magic, expected_magic, 8) != 0) {
        std::cerr << "Error: invalid magic bytes" << std::endl;
        f.close();
        return 1;
    }
    uint32_t version{};
    if (!ReadLE(f, version) || version != 1) {
        std::cerr << "Error: unsupported trace version" << std::endl;
        f.close();
        return 1;
    }

    // Read INIT record
    uint8_t init_op{};
    if (!ReadBytes(f, &init_op, 1) || init_op != OP_INIT) {
        std::cerr << "Error: expected INIT record after header" << std::endl;
        f.close();
        return 1;
    }
    uint32_t max_cluster_count{};
    uint64_t max_cluster_size{};
    uint64_t acceptable_cost{};
    if (!ReadLE(f, max_cluster_count) || !ReadLE(f, max_cluster_size) || !ReadLE(f, acceptable_cost)) {
        std::cerr << "Error: truncated INIT record" << std::endl;
        f.close();
        return 1;
    }

    std::cout << "TxGraph parameters: max_cluster_count=" << max_cluster_count
              << ", max_cluster_size=" << max_cluster_size
              << ", acceptable_cost=" << acceptable_cost << std::endl;

    // Create TxGraph
    auto cmp = [](const TxGraph::Ref& a, const TxGraph::Ref& b) -> std::strong_ordering {
        return &a <=> &b;
    };
    auto graph = MakeTxGraph(max_cluster_count, max_cluster_size, acceptable_cost, cmp);

    // Map from recorded GraphIndex -> live Ref.
    std::unordered_map<uint32_t, std::unique_ptr<TxGraph::Ref>> refs;

    // Per-entry-point timing stats (indexed by opcode)
    static constexpr size_t NUM_OPS = 0x30;
    TimingStats stats[NUM_OPS]{};
    stats[OP_GET_BLOCK_BUILDER]         = {"GetBlockBuilder"};
    stats[OP_DO_WORK]                   = {"DoWork"};
    stats[OP_COMPARE_MAIN_ORDER]        = {"CompareMainOrder"};
    stats[OP_GET_ANCESTORS]             = {"GetAncestors"};
    stats[OP_GET_DESCENDANTS]           = {"GetDescendants"};
    stats[OP_GET_ANCESTORS_UNION]       = {"GetAncestorsUnion"};
    stats[OP_GET_DESCENDANTS_UNION]     = {"GetDescendantsUnion"};
    stats[OP_GET_CLUSTER]               = {"GetCluster"};
    stats[OP_GET_MAIN_CHUNK_FEERATE]    = {"GetMainChunkFeerate"};
    stats[OP_COUNT_DISTINCT_CLUSTERS]   = {"CountDistinctClusters"};
    stats[OP_GET_MAIN_MEMORY_USAGE]     = {"GetMainMemoryUsage"};
    stats[OP_GET_WORST_MAIN_CHUNK]      = {"GetWorstMainChunk"};
    stats[OP_GET_MAIN_STAGING_DIAGRAMS] = {"GetMainStagingDiagrams"};
    stats[OP_TRIM]                      = {"Trim"};
    stats[OP_IS_OVERSIZED]              = {"IsOversized"};
    stats[OP_EXISTS]                    = {"Exists"};
    stats[OP_GET_INDIVIDUAL_FEERATE]    = {"GetIndividualFeerate"};
    stats[OP_GET_TRANSACTION_COUNT]     = {"GetTransactionCount"};
    stats[OP_START_STAGING]             = {"StartStaging"};
    stats[OP_ABORT_STAGING]             = {"AbortStaging"};
    stats[OP_COMMIT_STAGING]            = {"CommitStaging"};

    // Mutation counters (not timed)
    uint64_t add_tx_count{0}, remove_tx_count{0}, add_dep_count{0};
    uint64_t set_fee_count{0}, unlink_ref_count{0};
    uint64_t total_ops{0};

    // -----------------------------------------------------------------
    // Replay loop
    // -----------------------------------------------------------------
    while (true) {
        uint8_t opbyte{};
        if (!ReadBytes(f, &opbyte, 1)) break; // EOF

        ++total_ops;

        switch (opbyte) {

        // ---- Mutations (execute, no timing) ----

        case OP_ADD_TRANSACTION: {
            ++add_tx_count;
            uint32_t graph_idx{};
            int64_t fee{};
            int32_t size{};
            if (!ReadLE(f, graph_idx) || !ReadLE_i64(f, fee) || !ReadLE_i32(f, size)) goto eof_error;
            refs[graph_idx] = std::make_unique<TxGraph::Ref>();
            graph->AddTransaction(*refs[graph_idx], FeePerWeight{fee, size});
            break;
        }

        case OP_REMOVE_TRANSACTION: {
            ++remove_tx_count;
            uint32_t graph_idx{};
            if (!ReadLE(f, graph_idx)) goto eof_error;
            {
                auto it = refs.find(graph_idx);
                if (it != refs.end() && it->second) {
                    graph->RemoveTransaction(*it->second);
                }
            }
            break;
        }

        case OP_ADD_DEPENDENCY: {
            ++add_dep_count;
            uint32_t parent{}, child{};
            if (!ReadLE(f, parent) || !ReadLE(f, child)) goto eof_error;
            {
                auto pit = refs.find(parent);
                auto cit = refs.find(child);
                if (pit != refs.end() && pit->second && cit != refs.end() && cit->second) {
                    graph->AddDependency(*pit->second, *cit->second);
                }
            }
            break;
        }

        case OP_SET_TRANSACTION_FEE: {
            ++set_fee_count;
            uint32_t graph_idx{};
            int64_t fee{};
            if (!ReadLE(f, graph_idx) || !ReadLE_i64(f, fee)) goto eof_error;
            {
                auto it = refs.find(graph_idx);
                if (it != refs.end() && it->second) {
                    graph->SetTransactionFee(*it->second, fee);
                }
            }
            break;
        }

        case OP_UNLINK_REF: {
            ++unlink_ref_count;
            uint32_t graph_idx{};
            if (!ReadLE(f, graph_idx)) goto eof_error;
            refs.erase(graph_idx);
            break;
        }

        // ---- Trigger operations (execute + time) ----

        case OP_GET_BLOCK_BUILDER: {
            auto t0 = Clock::now();
            auto builder = graph->GetBlockBuilder();
            while (builder->GetCurrentChunk()) {
                builder->Include();
            }
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_DO_WORK: {
            uint64_t max_cost{};
            if (!ReadLE(f, max_cost)) goto eof_error;
            auto t0 = Clock::now();
            graph->DoWork(max_cost);
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_COMPARE_MAIN_ORDER: {
            uint32_t idx_a{}, idx_b{};
            if (!ReadLE(f, idx_a) || !ReadLE(f, idx_b)) goto eof_error;
            {
                auto ait = refs.find(idx_a);
                auto bit = refs.find(idx_b);
                if (ait != refs.end() && ait->second && bit != refs.end() && bit->second) {
                    auto t0 = Clock::now();
                    (void)graph->CompareMainOrder(*ait->second, *bit->second);
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_ANCESTORS: {
            uint32_t idx{};
            uint8_t level_byte{};
            if (!ReadLE(f, idx) || !ReadBytes(f, &level_byte, 1)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->GetAncestors(*it->second, static_cast<TxGraph::Level>(level_byte));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_DESCENDANTS: {
            uint32_t idx{};
            uint8_t level_byte{};
            if (!ReadLE(f, idx) || !ReadBytes(f, &level_byte, 1)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->GetDescendants(*it->second, static_cast<TxGraph::Level>(level_byte));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_ANCESTORS_UNION: {
            VarLenArgs vl;
            if (!ReadVarLen(f, vl)) goto eof_error;
            {
                std::vector<const TxGraph::Ref*> ptrs;
                ptrs.reserve(vl.indices.size());
                for (uint32_t ridx : vl.indices) {
                    auto it = refs.find(ridx);
                    if (it != refs.end() && it->second) ptrs.push_back(it->second.get());
                }
                if (!ptrs.empty()) {
                    auto t0 = Clock::now();
                    (void)graph->GetAncestorsUnion(ptrs, static_cast<TxGraph::Level>(vl.level));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_DESCENDANTS_UNION: {
            VarLenArgs vl;
            if (!ReadVarLen(f, vl)) goto eof_error;
            {
                std::vector<const TxGraph::Ref*> ptrs;
                ptrs.reserve(vl.indices.size());
                for (uint32_t ridx : vl.indices) {
                    auto it = refs.find(ridx);
                    if (it != refs.end() && it->second) ptrs.push_back(it->second.get());
                }
                if (!ptrs.empty()) {
                    auto t0 = Clock::now();
                    (void)graph->GetDescendantsUnion(ptrs, static_cast<TxGraph::Level>(vl.level));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_CLUSTER: {
            uint32_t idx{};
            uint8_t level_byte{};
            if (!ReadLE(f, idx) || !ReadBytes(f, &level_byte, 1)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->GetCluster(*it->second, static_cast<TxGraph::Level>(level_byte));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_MAIN_CHUNK_FEERATE: {
            uint32_t idx{};
            if (!ReadLE(f, idx)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->GetMainChunkFeerate(*it->second);
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_COUNT_DISTINCT_CLUSTERS: {
            VarLenArgs vl;
            if (!ReadVarLen(f, vl)) goto eof_error;
            {
                std::vector<const TxGraph::Ref*> ptrs;
                ptrs.reserve(vl.indices.size());
                for (uint32_t ridx : vl.indices) {
                    auto it = refs.find(ridx);
                    if (it != refs.end() && it->second) ptrs.push_back(it->second.get());
                }
                if (!ptrs.empty()) {
                    auto t0 = Clock::now();
                    (void)graph->CountDistinctClusters(ptrs, static_cast<TxGraph::Level>(vl.level));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_MAIN_MEMORY_USAGE: {
            auto t0 = Clock::now();
            (void)graph->GetMainMemoryUsage();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_GET_WORST_MAIN_CHUNK: {
            auto t0 = Clock::now();
            (void)graph->GetWorstMainChunk();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_GET_MAIN_STAGING_DIAGRAMS: {
            auto t0 = Clock::now();
            (void)graph->GetMainStagingDiagrams();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_TRIM: {
            auto t0 = Clock::now();
            (void)graph->Trim();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_IS_OVERSIZED: {
            uint8_t level_byte{};
            if (!ReadBytes(f, &level_byte, 1)) goto eof_error;
            auto t0 = Clock::now();
            (void)graph->IsOversized(static_cast<TxGraph::Level>(level_byte));
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_EXISTS: {
            uint32_t idx{};
            uint8_t level_byte{};
            if (!ReadLE(f, idx) || !ReadBytes(f, &level_byte, 1)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->Exists(*it->second, static_cast<TxGraph::Level>(level_byte));
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_INDIVIDUAL_FEERATE: {
            uint32_t idx{};
            if (!ReadLE(f, idx)) goto eof_error;
            {
                auto it = refs.find(idx);
                if (it != refs.end() && it->second) {
                    auto t0 = Clock::now();
                    (void)graph->GetIndividualFeerate(*it->second);
                    stats[opbyte].total_us += ElapsedUs(t0);
                    ++stats[opbyte].calls;
                }
            }
            break;
        }

        case OP_GET_TRANSACTION_COUNT: {
            uint8_t level_byte{};
            if (!ReadBytes(f, &level_byte, 1)) goto eof_error;
            auto t0 = Clock::now();
            (void)graph->GetTransactionCount(static_cast<TxGraph::Level>(level_byte));
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        // ---- Staging (execute + time) ----

        case OP_START_STAGING: {
            auto t0 = Clock::now();
            graph->StartStaging();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_ABORT_STAGING: {
            auto t0 = Clock::now();
            graph->AbortStaging();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_COMMIT_STAGING: {
            auto t0 = Clock::now();
            graph->CommitStaging();
            stats[opbyte].total_us += ElapsedUs(t0);
            ++stats[opbyte].calls;
            break;
        }

        case OP_INIT:
            // Unexpected second INIT; skip its payload.
            {
                uint32_t c{}; uint64_t s{}, a{};
                if (!ReadLE(f, c) || !ReadLE(f, s) || !ReadLE(f, a)) goto eof_error;
            }
            break;

        default:
            std::cerr << "Warning: unknown opcode 0x"
                      << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(opbyte)
                      << std::dec << std::setfill(' ')
                      << " at op " << total_ops << ", stopping" << std::endl;
            goto done;
        }
    }
    goto done;

eof_error:
    std::cerr << "Warning: unexpected EOF while reading payload at op " << total_ops << std::endl;

done:
    f.close();

    // -----------------------------------------------------------------
    // Report
    // -----------------------------------------------------------------
    std::cout << "\n=== TxGraph Replay Summary ===" << std::endl;
    std::cout << "Total ops replayed: " << total_ops << "\n" << std::endl;

    std::cout << "Mutations (not timed):" << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "AddTransaction" << " " << std::right << std::setw(12) << add_tx_count << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "RemoveTransaction" << " " << std::right << std::setw(12) << remove_tx_count << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "AddDependency" << " " << std::right << std::setw(12) << add_dep_count << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "SetTransactionFee" << " " << std::right << std::setw(12) << set_fee_count << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "UnlinkRef" << " " << std::right << std::setw(12) << unlink_ref_count << std::endl;

    std::cout << "\nTimed entry points:" << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "Operation" << " " << std::right << std::setw(12) << "Calls" << " " << std::setw(14) << "Total (us)" << " " << std::setw(14) << "Avg (us)" << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "---" << " " << std::right << std::setw(12) << "---" << " " << std::setw(14) << "---" << " " << std::setw(14) << "---" << std::endl;

    uint64_t grand_total_us{0};
    uint64_t grand_total_calls{0};

    for (size_t i = 0; i < NUM_OPS; ++i) {
        if (!stats[i].name || stats[i].calls == 0) continue;
        double avg = static_cast<double>(stats[i].total_us) / static_cast<double>(stats[i].calls);
        std::cout << "  " << std::left << std::setw(28) << stats[i].name << " "
                  << std::right << std::setw(12) << stats[i].calls << " "
                  << std::setw(14) << stats[i].total_us << " "
                  << std::fixed << std::setprecision(2) << std::setw(14) << avg << std::endl;
        grand_total_us += stats[i].total_us;
        grand_total_calls += stats[i].calls;
    }

    std::cout << "  " << std::left << std::setw(28) << "" << " " << std::right << std::setw(12) << "---" << " " << std::setw(14) << "---" << std::endl;
    std::cout << "  " << std::left << std::setw(28) << "TOTAL" << " "
              << std::right << std::setw(12) << grand_total_calls << " "
              << std::setw(14) << grand_total_us << std::endl;

    return 0;
}
