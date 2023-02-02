#include <test/fuzz/proto/convert.h>

#include <primitives/transaction.h>
#include <txmempool.h>

#include <test/fuzz/proto/mempool_options.pb.h>
#include <test/fuzz/proto/transaction.pb.h>

CTransactionRef ConvertTransaction(const common_fuzz::Transaction& proto_tx)
{
    CMutableTransaction mtx;
    mtx.nLockTime = proto_tx.lock_time();
    mtx.nVersion = proto_tx.version();

    for (const auto& proto_input : proto_tx.inputs()) {
        CTxIn input;
        input.nSequence = proto_input.sequence();
        // scriptWitness
        for (const auto& witness : proto_input.witness_stack()) {
            input.scriptWitness.stack.push_back({witness.begin(), witness.end()});
        }
        // scriptSig
        std::vector<uint8_t> script{proto_input.script_sig().raw().begin(),
                                    proto_input.script_sig().raw().end()};
        input.scriptSig = {script.begin(), script.end()};
        // prevout
        auto hex = proto_input.prev_out().txid();
        hex.resize(64);
        input.prevout.hash = uint256S(hex);
        input.prevout.n = proto_input.prev_out().n();

        mtx.vin.emplace_back(std::move(input));
    }

    for (const auto& proto_output : proto_tx.outputs()) {
        CTxOut output;
        output.nValue = proto_output.value();
        std::vector<uint8_t> script{proto_output.script_pub_key().raw().begin(),
                                    proto_output.script_pub_key().raw().end()};
        output.scriptPubKey = {script.begin(), script.end()};
        mtx.vout.emplace_back(std::move(output));
    }

    return MakeTransactionRef(mtx);
}

CTxMemPool::Options ConvertMempoolOptions(const common_fuzz::MempoolOptions& mempool_options)
{
    // Take the default options for tests...
    CTxMemPool::Options mempool_opts;

    // ...override specific options for this specific fuzz suite
    mempool_opts.estimator = nullptr;
    mempool_opts.check_ratio = 1;

	// TODO clean up the naming
    auto& options = mempool_options;
    if (options.has_max_size_bytes())
        mempool_opts.require_standard = options.require_standard();
    if (options.has_expiry_hours())
        mempool_opts.expiry = std::chrono::hours{options.expiry_hours()};
    if (options.has_incremental_relay_feerate())
        mempool_opts.incremental_relay_feerate = CFeeRate{options.incremental_relay_feerate()};
    if (options.has_min_relay_feerate())
        mempool_opts.min_relay_feerate = CFeeRate{options.min_relay_feerate()};
    if (options.has_dust_relay_feerate())
        mempool_opts.dust_relay_feerate = CFeeRate{options.dust_relay_feerate()};
    if (options.has_max_datacarrier_bytes())
        mempool_opts.max_datacarrier_bytes = options.max_datacarrier_bytes();
    if (options.has_permit_bare_multisig())
        mempool_opts.permit_bare_multisig = options.permit_bare_multisig();
    if (options.has_require_standard())
        mempool_opts.require_standard = options.require_standard();
    if (options.has_full_rbf())
        mempool_opts.full_rbf = options.full_rbf();

    auto& limits = options.limits();
    auto& mempool_limits = mempool_opts.limits;
    if (limits.has_ancestor_count_limit())
        mempool_limits.ancestor_count = limits.ancestor_count_limit();
    if (limits.has_ancestor_size_limit())
        mempool_limits.ancestor_size_vbytes = limits.ancestor_size_limit();
    if (limits.has_descendant_count_limit())
        mempool_limits.descendant_count = limits.descendant_count_limit();
    if (limits.has_ancestor_size_limit())
        mempool_limits.descendant_size_vbytes = limits.descendant_size_limit();

    return mempool_opts;
}
