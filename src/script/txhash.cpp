// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/txhash.h>
#include <script/interpreter.h>

#include <crypto/sha256.h>
#include <script/script.h>
#include <uint256.h>

/// Cached SHA-256 hash of the empty byte vector.
static uint256 SHA256_EMPTY = (HashWriter{}).GetSHA256();

signed int read_i7(uint8_t input) {
    uint8_t masked = input & 0x7f;
    if ((masked & 0x40) == 0) {
        return masked;
    } else {
        uint8_t neg = (~(masked-1)) & 0x7f;
        return 0 - neg;
    }
}

signed int read_i15(uint16_t input) {
    uint16_t masked = input & 0x7fff;
    if ((masked & 0x4000) == 0) {
        return masked;
    } else {
        uint16_t neg = (~(masked-1)) & 0x7fff;
        return 0 - neg;
    }
}

/// Validate a input/output range selector.
///
/// Returns the number of bytes used from the buffer.
bool parse_inout_selector(
    Span<const unsigned char>& bytes,
    unsigned int nb_items,
    bool& out_count,
    bool& out_all,
    bool& out_current,
    unsigned int& out_leading,
    std::vector<unsigned int>& out_individual,
    uint32_t in_pos,
    bool allow_empty
) {
    // zero out all outputs
    out_count = false;
    out_all = false;
    out_current = false;
    out_leading = 0;
    out_individual.clear();

    if (bytes.empty() && allow_empty) {
        return false; // unexpected EOF
    }
    unsigned char first = SpanPopFront(bytes);
    out_count = (first & TXFS_INOUT_NUMBER) != 0;

    unsigned int selection = first & (0xff ^ TXFS_INOUT_NUMBER);
    if (selection == TXFS_INOUT_SELECTION_NONE) {
        return true;
    } else if (selection == TXFS_INOUT_SELECTION_ALL) {
        out_all = true;
        return true;
    } else if (selection == TXFS_INOUT_SELECTION_CURRENT) {
        out_current = true;
        return true;
    } else if ((selection & TXFS_INOUT_SELECTION_MODE) == 0) {
        // leading mode
        unsigned int count;
        if ((selection & TXFS_INOUT_LEADING_SIZE) == 0) {
            count = selection & TXFS_INOUT_SELECTION_MASK;
        } else {
            if (bytes.empty()) {
                return false; // unexpected EOF
            }
            unsigned int next_byte = SpanPopFront(bytes);

            count = ((selection & TXFS_INOUT_SELECTION_MASK) << 8) + next_byte;
        }
        if (count > nb_items) {
            return false; // nb of leading in/outputs too high
        }
        out_leading = count;
        return true;
    } else {
        // individual mode
        bool absolute = (selection & TXFS_INOUT_INDIVIDUAL_MODE) == 0;
        unsigned int count = selection & TXFS_INOUT_SELECTION_MASK;

        signed int cur = in_pos; // cast to avoid warnings
        std::vector<unsigned int> indices = {};
        for (unsigned int i = 0; i < count; i++) {
            if (bytes.size() < 1) {
                return false; // not enough index bytes
            }
            unsigned int first = SpanPopFront(bytes);
            bool single_byte = (first & 1 << 7) == 0;

            unsigned int number = first;
            if (!single_byte) {
                if (bytes.size() < 1) {
                    return false; // not enough index bytes
                }
                unsigned int second = SpanPopFront(bytes);
                number = ((first & (1 << 7)) << 8) + second;
            }

            unsigned int idx = number;
            if (!absolute) {
                signed int rel = 0;
                if (single_byte) {
                    rel = read_i7(number);
                } else {
                    rel = read_i15(number);
                }

                if (rel < 0 && (-rel) > cur) {
                    return false; // underflow
                }
                idx = cur + rel;
            }

            if (idx > nb_items) {
                return false; // overflow
            }
            if (indices.size() > 0 && idx <= indices.back()) {
                return false; // not in increasing order
            }
            indices.push_back(idx);
        }
        out_individual = indices;
        return true;
    }

    return false; // should be unreachable
}

uint256 sha256_bytes(const std::vector<unsigned char>& bytes) {
    uint256 out;
    CSHA256().Write(&*bytes.begin(), bytes.size()).Finalize(out.begin());
    return out;
}

uint256 sha256_script(const CScript& script) {
    uint256 out;
    CSHA256().Write(&*script.begin(), script.size()).Finalize(out.begin());
    return out;
}

// The following hash calculation methods panic on index out of bounds.

uint256 script_sig_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, unsigned int idx) {
    if (cache.hashed_script_sigs.empty() || cache.hashed_script_sigs[idx].IsNull()) {
        LOCK(cache.mtx);
        cache.hashed_script_sigs.resize(inputs.size());
        cache.hashed_script_sigs[idx] = sha256_script(inputs[idx].scriptSig);
    }
    return cache.hashed_script_sigs[idx];
}

uint256 prevout_spk_hash(TxHashCache& cache, const std::vector<CTxOut> prev_outputs, unsigned int idx) {
    if (cache.hashed_prevout_spks.empty() || cache.hashed_prevout_spks[idx].IsNull()) {
        LOCK(cache.mtx);
        cache.hashed_prevout_spks.resize(prev_outputs.size());
        cache.hashed_prevout_spks[idx] = sha256_script(prev_outputs[idx].scriptPubKey);
    }
    return cache.hashed_prevout_spks[idx];
}

uint256 annex_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, const std::vector<CTxOut> prev_outputs, unsigned int idx) {
    if (cache.hashed_annexes.empty() || cache.hashed_annexes[idx].IsNull()) {
        LOCK(cache.mtx);
        cache.hashed_annexes.resize(inputs.size());

        // check if there is an annex
        uint256 annex_hash = SHA256_EMPTY;
        int witnessversion;
        std::vector<unsigned char> witnessprogram;
        if (prev_outputs[idx].scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
                assert(inputs[idx].scriptSig.size() == 0); //TODO(stevenroose) is this ok?

                const std::vector<std::vector<unsigned char>>& stack = inputs[idx].scriptWitness.stack;
                if (stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
                    annex_hash = sha256_bytes(stack.back());
                }
            }
        }
        cache.hashed_annexes[idx] = annex_hash;
    }
    return cache.hashed_annexes[idx];
}

uint256 script_pubkey_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs, unsigned int idx) {
    if (cache.hashed_script_pubkeys.empty() || cache.hashed_script_pubkeys[idx].IsNull()) {
        LOCK(cache.mtx);
        cache.hashed_script_pubkeys.resize(outputs.size());
        cache.hashed_script_pubkeys[idx] = sha256_script(outputs[idx].scriptPubKey);
    }
    return cache.hashed_script_pubkeys[idx];
}

uint256 leading_prevouts_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, unsigned int nb) {
    if (cache.leading_prevouts.empty()) {
        LOCK(cache.mtx);
        cache.leading_prevouts.reserve(inputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_prevouts.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_prevouts[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << inputs[cursor].prevout;
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_prevouts.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_sequences_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, unsigned int nb) {
    if (cache.leading_sequences.empty()) {
        LOCK(cache.mtx);
        cache.leading_sequences.reserve(inputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_sequences.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_sequences[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << inputs[cursor].nSequence;
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_sequences.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_script_sigs_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, unsigned int nb) {
    if (cache.leading_script_sigs.empty()) {
        LOCK(cache.mtx);
        cache.leading_script_sigs.reserve(inputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_script_sigs.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_script_sigs[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << script_sig_hash(cache, inputs, cursor);
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_script_sigs.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_prevout_spks_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs, unsigned int nb) {
    if (cache.leading_prevout_spks.empty()) {
        LOCK(cache.mtx);
        cache.leading_prevout_spks.reserve(prev_outputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_prevout_spks.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_prevout_spks[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << prevout_spk_hash(cache, prev_outputs, cursor);
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_prevout_spks.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_prevout_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs, unsigned int nb) {
    if (cache.leading_prevout_amounts.empty()) {
        LOCK(cache.mtx);
        cache.leading_prevout_amounts.reserve(prev_outputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_prevout_amounts.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_prevout_amounts[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << prev_outputs[cursor].nValue;
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_prevout_amounts.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_annexes_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, const std::vector<CTxOut>& prev_outputs, unsigned int nb) {
    if (cache.leading_annexes.empty()) {
        LOCK(cache.mtx);
        cache.leading_annexes.reserve(inputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_annexes.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_annexes[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << annex_hash(cache, inputs, prev_outputs, cursor);
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_annexes.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_script_pubkeys_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs, unsigned int nb) {
    if (cache.leading_script_pubkeys.empty()) {
        LOCK(cache.mtx);
        cache.leading_script_pubkeys.reserve(outputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_script_pubkeys.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_script_pubkeys[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << script_pubkey_hash(cache, outputs, cursor);
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_script_pubkeys.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 leading_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs, unsigned int nb) {
    if (cache.leading_amounts.empty()) {
        LOCK(cache.mtx);
        cache.leading_amounts.reserve(outputs.size() / LEADING_CACHE_INTERVAL);
    }

    unsigned int cache_cursor = (cache.leading_amounts.size() + 1) * LEADING_CACHE_INTERVAL;
    unsigned int cursor = std::min(cache_cursor, nb / LEADING_CACHE_INTERVAL);

    HashWriter ss;
    if (cursor != 0) {
        ss = HashWriter(cache.leading_amounts[cursor / LEADING_CACHE_INTERVAL]);
    } else {
        ss = HashWriter{};
    }

    while (cursor < nb) {
        ss << outputs[cursor].nValue;
        if (cursor % LEADING_CACHE_INTERVAL == 0) {
            LOCK(cache.mtx);
            cache.leading_amounts.push_back(ss.GetHashCtx());
        }
        cursor++;
    }

    return ss.GetSHA256();
}

uint256 all_prevouts_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs) {
    if (cache.all_prevouts.IsNull()) {
        LOCK(cache.mtx);
        cache.all_prevouts = leading_prevouts_hash(cache, inputs, inputs.size());
    }
    return cache.all_prevouts;
}

uint256 all_sequences_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs) {
    if (cache.all_sequences.IsNull()) {
        LOCK(cache.mtx);
        cache.all_sequences = leading_sequences_hash(cache, inputs, inputs.size());
    }
    return cache.all_sequences;
}

uint256 all_script_sigs_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs) {
    if (cache.all_script_sigs.IsNull()) {
        LOCK(cache.mtx);
        cache.all_script_sigs = leading_script_sigs_hash(cache, inputs, inputs.size());
    }
    return cache.all_script_sigs;
}

uint256 all_prevout_spks_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs) {
    if (cache.all_prevout_spks.IsNull()) {
        LOCK(cache.mtx);
        cache.all_prevout_spks = leading_prevout_spks_hash(cache, prev_outputs, prev_outputs.size());
    }
    return cache.all_prevout_spks;
}

uint256 all_prevout_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs) {
    if (cache.all_prevout_amounts.IsNull()) {
        LOCK(cache.mtx);
        cache.all_prevout_amounts = leading_prevout_amounts_hash(cache, prev_outputs, prev_outputs.size());
    }
    return cache.all_prevout_amounts;
}

uint256 all_annexes_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, const std::vector<CTxOut>& prev_outputs) {
    if (cache.all_annexes.IsNull()) {
        LOCK(cache.mtx);
        cache.all_annexes = leading_annexes_hash(cache, inputs, prev_outputs, inputs.size());
    }
    return cache.all_annexes;
}

uint256 all_script_pubkeys_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs) {
    if (cache.all_script_pubkeys.IsNull()) {
        LOCK(cache.mtx);
        cache.all_script_pubkeys = leading_script_pubkeys_hash(cache, outputs, outputs.size());
    }
    return cache.all_script_pubkeys;
}

uint256 all_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs) {
    if (cache.all_amounts.IsNull()) {
        LOCK(cache.mtx);
        cache.all_amounts = leading_amounts_hash(cache, outputs, outputs.size());
    }
    return cache.all_amounts;
}

uint256 selected_prevouts_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << inputs[idx].prevout;
    }
    return ss.GetSHA256();
}

uint256 selected_sequences_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << inputs[idx].nSequence;
    }
    return ss.GetSHA256();
}

uint256 selected_script_sigs_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << script_sig_hash(cache, inputs, idx);
    }
    return ss.GetSHA256();
}

uint256 selected_prevout_spks_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << prevout_spk_hash(cache, prev_outputs, idx);
    }
    return ss.GetSHA256();
}

uint256 selected_prevout_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& prev_outputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << prev_outputs[idx].nValue;
    }
    return ss.GetSHA256();
}

uint256 selected_annexes_hash(TxHashCache& cache, const std::vector<CTxIn>& inputs, const std::vector<CTxOut>& prev_outputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << annex_hash(cache, inputs, prev_outputs, idx);
    }
    return ss.GetSHA256();
}

uint256 selected_script_pubkeys_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << script_pubkey_hash(cache, outputs, idx);
    }
    return ss.GetSHA256();
}

uint256 selected_amounts_hash(TxHashCache& cache, const std::vector<CTxOut>& outputs, std::vector<unsigned int>& indices) {
    HashWriter ss{};
    for (unsigned int i = 0; i < indices.size(); i++) {
        unsigned int idx = indices[i];
        ss << outputs[idx].nValue;
    }
    return ss.GetSHA256();
}

template <class T>
bool calculate_txhash(
    uint256& hash_out,
    Span<const unsigned char> field_selector,
    TxHashCache& cache,
    const T& tx,
    const std::vector<CTxOut>& prev_outputs,
    uint32_t codeseparator_pos,
    uint32_t in_pos
) {
    assert(tx.vin.size() == prev_outputs.size());
    assert(in_pos < tx.vin.size());

    if (field_selector.empty()) {
        field_selector = Span<const unsigned char> { TXFS_SPECIAL_TEMPLATE };
    } else if (field_selector.size() == 1 && field_selector[0] == 0) {
        field_selector = Span<const unsigned char> { TXFS_SPECIAL_ALL };
    }

    HashWriter ss{};

    unsigned char global = field_selector[0];

    if ((global & TXFS_CONTROL) != 0) {
        ss << field_selector;
    }

    if ((global & TXFS_VERSION) != 0) {
        ss << tx.nVersion;
    }

    if ((global & TXFS_LOCKTIME) != 0) {
        ss << tx.nLockTime;
    }

    if ((global & TXFS_CURRENT_INPUT_IDX) != 0) {
        ss << in_pos;
    }

    if ((global & TXFS_CURRENT_INPUT_SPENTSCRIPT) != 0) {
        if (cache.hashed_spentscripts.empty() || cache.hashed_spentscripts[in_pos].IsNull()) {
            LOCK(cache.mtx);
            cache.hashed_spentscripts.resize(tx.vin.size());

            // extract spentScript:
            // - for p2sh: redeemScript from scriptSig
            // - for p2wsh: witnessScript from witness
            // - for p2tr: tapscript from control block
            uint256 ss_hash = SHA256_EMPTY;

            int witver;
            std::vector<unsigned char> witprog;
            CScript scriptSig = tx.vin[in_pos].scriptSig;
            if (prev_outputs[in_pos].scriptPubKey.IsPayToScriptHash() && scriptSig.IsPushOnly()) {
                // We need to convert the scriptSig to a stack.
                std::vector<std::vector<unsigned char>> stack;
                unsigned int flags = SCRIPT_VERIFY_NONE;
                BaseSignatureChecker checker;
                ScriptError serror;
                if (!EvalScript(stack, scriptSig, flags, checker, SigVersion::BASE, &serror)) {
                    return false;
                }
                const std::vector<unsigned char>& redeemScriptBytes = stack.back();
                ss_hash = sha256_bytes(redeemScriptBytes);
                // CScript redeemScript(redeemScriptBytes.begin(), redeemScriptBytes.end());
            } else if (prev_outputs[in_pos].scriptPubKey.IsPayToWitnessScriptHash()) {
                if (tx.vin[in_pos].scriptWitness.stack.back().size() > 0) {
                    ss_hash = sha256_bytes(tx.vin[in_pos].scriptWitness.stack.back());
                }
            } else if (prev_outputs[in_pos].scriptPubKey.IsWitnessProgram(witver, witprog)) {
                if (witver == 1 && witprog.size() == WITNESS_V1_TAPROOT_SIZE) {
                    const std::vector<std::vector<unsigned char>>& stack = tx.vin[in_pos].scriptWitness.stack;
                    if (stack.size() >= 3 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
                        ss_hash = sha256_bytes(stack[stack.size() - 3]);
                    } else if (stack.size() >= 2) {
                        ss_hash = sha256_bytes(stack[stack.size() - 2]);
                    }
                }
            }
            cache.hashed_spentscripts[in_pos] = ss_hash;
        }
        ss << cache.hashed_spentscripts[in_pos];
    }

    if ((global & TXFS_CURRENT_INPUT_CONTROL_BLOCK) != 0) {
        if (cache.hashed_control_blocks.empty() || cache.hashed_control_blocks[in_pos].IsNull()) {
            LOCK(cache.mtx);
            cache.hashed_control_blocks.resize(tx.vin.size());

            // extract control block
            uint256 cb_hash = SHA256_EMPTY;
            int witnessversion;
            std::vector<unsigned char> witnessprogram;
            if (prev_outputs[in_pos].scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
                if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
                    const std::vector<std::vector<unsigned char>>& stack = tx.vin[in_pos].scriptWitness.stack;
                    if (stack.size() >= 3 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
                        cb_hash = sha256_bytes(stack[stack.size() - 2]);
                    } else if (stack.size() >= 2) {
                        cb_hash = sha256_bytes(stack.back());
                    }
                }
            }
            cache.hashed_control_blocks[in_pos] = cb_hash;
        }
        ss << cache.hashed_control_blocks[in_pos];
    }

    if ((global & TXFS_CURRENT_INPUT_LAST_CODESEPARATOR_POS) != 0) {
        ss << codeseparator_pos;
    }


    Span<const unsigned char> bytes = field_selector.subspan(1);
    if (bytes.size() == 0) {
        hash_out = ss.GetSHA256();
        return true;
    }
    if (field_selector.size() == 1) {
        return false;
    }
    unsigned char inout_fields = SpanPopFront(bytes);

    // INPUTS
    bool count = false;
    bool all = false;
    bool current = false;
    unsigned int leading = 0;
    std::vector<unsigned int> individual = {};
    if (!parse_inout_selector(bytes, tx.vin.size(), count, all, current, leading, individual, in_pos, true)) {
        return false;
    }

    if (count) {
        uint32_t len32 = tx.vin.size();
        ss << len32;
    }

    if ((inout_fields & TXFS_INPUTS_PREVOUTS) != 0) {
        if (all) {
            ss << all_prevouts_hash(cache, tx.vin);
        }
        if (current) {
            ss << (HashWriter{} << tx.vin[in_pos].prevout).GetSHA256();
        }
        if (leading) {
            ss << leading_prevouts_hash(cache, tx.vin, leading);
        }
        if (!individual.empty()) {
            ss << selected_prevouts_hash(cache, tx.vin, individual);
        }
    }

    if ((inout_fields & TXFS_INPUTS_SEQUENCES) != 0) {
        if (all) {
            ss << all_sequences_hash(cache, tx.vin);
        }
        if (current) {
            ss << (HashWriter{} << tx.vin[in_pos].nSequence).GetSHA256();
        }
        if (leading) {
            ss << leading_sequences_hash(cache, tx.vin, leading);
        }
        if (!individual.empty()) {
            ss << selected_sequences_hash(cache, tx.vin, individual);
        }
    }

    if ((inout_fields & TXFS_INPUTS_SCRIPTSIGS) != 0) {
        if (all) {
            ss << all_script_sigs_hash(cache, tx.vin);
        }
        if (current) {
            ss << (HashWriter{} << script_sig_hash(cache, tx.vin, in_pos)).GetSHA256();
        }
        if (leading) {
            ss << leading_script_sigs_hash(cache, tx.vin, leading);
        }
        if (!individual.empty()) {
            ss << selected_script_sigs_hash(cache, tx.vin, individual);
        }
    }

    if ((inout_fields & TXFS_INPUTS_PREVOUT_SCRIPTPUBKEYS) != 0) {
        if (all) {
            ss << all_prevout_spks_hash(cache, prev_outputs);
        }
        if (current) {
            ss << (HashWriter{} << prevout_spk_hash(cache, prev_outputs, in_pos)).GetSHA256();
        }
        if (leading) {
            ss << leading_prevout_spks_hash(cache, prev_outputs, leading);
        }
        if (!individual.empty()) {
            ss << selected_prevout_spks_hash(cache, prev_outputs, individual);
        }
    }

    if ((inout_fields & TXFS_INPUTS_PREVOUT_VALUES) != 0) {
        if (all) {
            ss << all_prevout_amounts_hash(cache, prev_outputs);
        }
        if (current) {
            ss << (HashWriter{} << prev_outputs[in_pos].nValue).GetSHA256();
        }
        if (leading) {
            ss << leading_prevout_amounts_hash(cache, prev_outputs, leading);
        }
        if (!individual.empty()) {
            ss << selected_prevout_amounts_hash(cache, prev_outputs, individual);
        }
    }

    if ((inout_fields & TXFS_INPUTS_TAPROOT_ANNEXES) != 0) {
        if (all) {
            ss << all_annexes_hash(cache, tx.vin, prev_outputs);
        }
        if (current) {
            ss << (HashWriter{} << annex_hash(cache, tx.vin, prev_outputs, in_pos)).GetSHA256();
        }
        if (leading) {
            ss << leading_annexes_hash(cache, tx.vin, prev_outputs, leading);
        }
        if (!individual.empty()) {
            ss << selected_annexes_hash(cache, tx.vin, prev_outputs, individual);
        }
    }

    // OUTPUTS
    if (bytes.size() == 0) {
        hash_out = ss.GetSHA256();
        return true;
    }

    count = false;
    all = false;
    current = false;
    leading = 0;
    individual = {};
    if (!parse_inout_selector(bytes, tx.vout.size(), count, all, current, leading, individual, in_pos, false)) {
        return false;
    }

    if (current && in_pos >= tx.vout.size()) {
        // current input index out of bounds
        return false;
    }

    if (count) {
        uint32_t len32 = tx.vout.size();
        ss << len32;
    }

    if ((inout_fields & TXFS_OUTPUTS_SCRIPTPUBKEYS) != 0) {
        if (all) {
            ss << all_script_pubkeys_hash(cache, tx.vout);
        }
        if (current) {
            ss << (HashWriter{} << script_pubkey_hash(cache, tx.vout, in_pos)).GetSHA256();
        }
        if (leading) {
            ss << leading_script_pubkeys_hash(cache, tx.vout, leading);
        }
        if (!individual.empty()) {
            ss << selected_script_pubkeys_hash(cache, tx.vout, individual);
        }
    }

    if ((inout_fields & TXFS_OUTPUTS_VALUES) != 0) {
        if (all) {
            ss << all_amounts_hash(cache, tx.vout);
        }
        if (current) {
            ss << (HashWriter{} << tx.vout[in_pos].nValue).GetSHA256();
        }
        if (leading) {
            ss << leading_amounts_hash(cache, tx.vout, leading);
        }
        if (!individual.empty()) {
            ss << selected_amounts_hash(cache, tx.vout, individual);
        }
    }

    hash_out = ss.GetSHA256();
    return true;
}

template
bool calculate_txhash(
    uint256&,
    Span<const unsigned char>,
    TxHashCache&,
    const CTransaction&,
    const std::vector<CTxOut>&,
    uint32_t,
    uint32_t
);

template
bool calculate_txhash(
    uint256&,
    Span<const unsigned char>,
    TxHashCache&,
    const CMutableTransaction&,
    const std::vector<CTxOut>&,
    uint32_t,
    uint32_t
);
