// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * SignatureHashSchnorr from Bitcoin Core 23.0.
 *
 * Only change is using an assert directly instead of HandleMissingData.
 */
template<typename T>
bool OldSignatureHashSchnorr(uint256& hash_out, ScriptExecutionData& execdata, const T& tx_to, uint32_t in_pos, uint8_t hash_type, SigVersion sigversion, const PrecomputedTransactionData& cache, MissingDataBehavior mdb)
{
    uint8_t ext_flag, key_version;
    switch (sigversion) {
    case SigVersion::TAPROOT:
        ext_flag = 0;
        // key_version is not used and left uninitialized.
        break;
    case SigVersion::TAPSCRIPT:
        ext_flag = 1;
        // key_version must be 0 for now, representing the current version of
        // 32-byte public keys in the tapscript signature opcode execution.
        // An upgradable public key version (with a size not 32-byte) may
        // request a different key_version with a new sigversion.
        key_version = 0;
        break;
    default:
        assert(false);
    }
    assert(in_pos < tx_to.vin.size());
    assert(cache.m_bip341_taproot_ready && cache.m_spent_outputs_ready);

    HashWriter ss = HASHER_TAPSIGHASH;

    // Epoch
    static constexpr uint8_t EPOCH = 0;
    ss << EPOCH;

    // Hash type
    const uint8_t output_type = (hash_type == SIGHASH_DEFAULT) ? SIGHASH_ALL : (hash_type & SIGHASH_OUTPUT_MASK); // Default (no sighash byte) is equivalent to SIGHASH_ALL
    const uint8_t input_type = hash_type & SIGHASH_INPUT_MASK;
    if (!(hash_type <= 0x03 || (hash_type >= 0x81 && hash_type <= 0x83))) return false;
    ss << hash_type;

    // Transaction level data
    ss << tx_to.nVersion;
    ss << tx_to.nLockTime;
    if (input_type != SIGHASH_ANYONECANPAY) {
        ss << cache.m_prevouts_single_hash;
        ss << cache.m_spent_amounts_single_hash;
        ss << cache.m_spent_scripts_single_hash;
        ss << cache.m_sequences_single_hash;
    }
    if (output_type == SIGHASH_ALL) {
        ss << cache.m_outputs_single_hash;
    }

    // Data about the input/prevout being spent
    assert(execdata.m_annex_init);
    const bool have_annex = execdata.m_annex_present;
    const uint8_t spend_type = (ext_flag << 1) + (have_annex ? 1 : 0); // The low bit indicates whether an annex is present.
    ss << spend_type;
    if (input_type == SIGHASH_ANYONECANPAY) {
        ss << tx_to.vin[in_pos].prevout;
        ss << cache.m_spent_outputs[in_pos];
        ss << tx_to.vin[in_pos].nSequence;
    } else {
        ss << in_pos;
    }
    if (have_annex) {
        ss << execdata.m_annex_hash;
    }

    // Data about the output (if only one).
    if (output_type == SIGHASH_SINGLE) {
        if (in_pos >= tx_to.vout.size()) return false;
        if (!execdata.m_output_hash) {
            HashWriter sha_single_output{};
            sha_single_output << tx_to.vout[in_pos];
            execdata.m_output_hash = sha_single_output.GetSHA256();
        }
        ss << execdata.m_output_hash.value();
    }

    // Additional data for BIP 342 signatures
    if (sigversion == SigVersion::TAPSCRIPT) {
        assert(execdata.m_tapleaf_hash_init);
        ss << execdata.m_tapleaf_hash;
        ss << key_version;
        assert(execdata.m_codeseparator_pos_init);
        ss << execdata.m_codeseparator_pos;
    }

    hash_out = ss.GetSHA256();
    return true;
}

/** Test APO's SignatureHashSchnorr against Bitcoin 23.0's SignatureHashSchnorr.
 *
 * This makes sure the behaviour for non-APO keys was conserved with the introduction of the APO logic.
 */
FUZZ_TARGET(anyprevout_old_new_sighash)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    {
        // Get a transaction and pick the input we'll generate the sighash for from the fuzzer output.
        std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!mtx) return;
        const uint32_t in_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        if (in_pos >= mtx->vin.size()) return;

        // Make sure at least one of the transaction's inputs has a non-empty witness, as cache.Init()
        // below needs it. Then take a script code (we don't need it to be valid), a signature version
        // (which must be anything but APO), and a hash type to use from the fuzzer's output.
        if (mtx->vin[0].scriptWitness.IsNull()) mtx->vin[0].scriptWitness.stack.push_back({0});
        const CTransaction tx_to{*mtx};
        const CScript script_code = ConsumeScript(fuzzed_data_provider);
        const SigVersion sig_version{fuzzed_data_provider.PickValueInArray({SigVersion::TAPROOT, SigVersion::TAPSCRIPT})};
        const uint8_t hash_type{fuzzed_data_provider.ConsumeIntegral<uint8_t>()};

        // Populate the transaction cache with the transaction and dummy spent outputs. The scriptPubkKey
        // must match the Taproot template for Init() to populate the right fields.
        PrecomputedTransactionData cache;
        std::vector<CTxOut> spent_outputs;
        spent_outputs.reserve(tx_to.vin.size());
        for (unsigned i = 0; i < tx_to.vin.size(); ++i) {
            CScript script_pubkey;
            script_pubkey << OP_1 << fuzzed_data_provider.ConsumeBytes<uint8_t>(WITNESS_V1_TAPROOT_SIZE);
            if (script_pubkey.size() != 2 + WITNESS_V1_TAPROOT_SIZE) return;
            const CAmount value{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
            spent_outputs.emplace_back(value, std::move(script_pubkey));
        }
        cache.Init(tx_to, std::move(spent_outputs));
        assert(cache.m_spent_outputs_ready);
        assert(cache.m_bip341_taproot_ready);

        // Now populate the Script execution data. Annex needs always be populated and tapleaf hash
        // is only need for Script-path spends.
        ScriptExecutionData exec_data;
        exec_data.m_annex_present = false;
        if (fuzzed_data_provider.ConsumeBool()) {
            if (const auto annex_hash = ConsumeDeserializable<uint256>(fuzzed_data_provider)) {
                exec_data.m_annex_hash = *annex_hash;
                exec_data.m_annex_present = true;
            }
        }
        exec_data.m_annex_init = true;
        if (sig_version == SigVersion::TAPSCRIPT) {
            if (const auto tapleaf_hash = ConsumeDeserializable<uint256>(fuzzed_data_provider)) {
                exec_data.m_tapleaf_hash = *tapleaf_hash;
                exec_data.m_tapleaf_hash_init = true;
            } else {
                return;
            }
            exec_data.m_codeseparator_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            exec_data.m_codeseparator_pos_init = true;
        }

        // Assert the existing behaviour was conserved with the introduction of the APO logic for
        // the fully pseudo-random hashtype.
        uint256 old_sighash, new_sighash;
        const auto new_res = SignatureHashSchnorr(new_sighash, exec_data, tx_to, in_pos, hash_type, sig_version,
                                                  KeyVersion::TAPROOT, cache, MissingDataBehavior::ASSERT_FAIL);
        const auto old_res = OldSignatureHashSchnorr(old_sighash, exec_data, tx_to, in_pos, hash_type, sig_version,
                                                     cache, MissingDataBehavior::ASSERT_FAIL);
        assert(new_res == old_res);
        assert(new_sighash == old_sighash);
        const auto new_res_h = SignatureHashSchnorr(new_sighash, exec_data, tx_to, in_pos, hash_type, sig_version,
                                                    KeyVersion::TAPROOT, cache, MissingDataBehavior::ASSERT_FAIL);
        const auto old_res_h = OldSignatureHashSchnorr(old_sighash, exec_data, tx_to, in_pos, hash_type, sig_version,
                                                       cache, MissingDataBehavior::ASSERT_FAIL);
        assert(new_res_h == old_res_h);
        assert(new_sighash == old_sighash);
    }
}
