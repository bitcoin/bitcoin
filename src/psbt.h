// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PSBT_H
#define BITCOIN_PSBT_H

#include <common/types.h>
#include <node/transaction.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <span.h>
#include <streams.h>

#include <optional>

namespace node {
enum class TransactionError;
} // namespace node

using common::PSBTError;

// Magic bytes
static constexpr uint8_t PSBT_MAGIC_BYTES[5] = {'p', 's', 'b', 't', 0xff};

// Global types
static constexpr uint8_t PSBT_GLOBAL_UNSIGNED_TX = 0x00;
static constexpr uint8_t PSBT_GLOBAL_XPUB = 0x01;
static constexpr uint8_t PSBT_GLOBAL_VERSION = 0xFB;
static constexpr uint8_t PSBT_GLOBAL_PROPRIETARY = 0xFC;

// Input types
static constexpr uint8_t PSBT_IN_NON_WITNESS_UTXO = 0x00;
static constexpr uint8_t PSBT_IN_WITNESS_UTXO = 0x01;
static constexpr uint8_t PSBT_IN_PARTIAL_SIG = 0x02;
static constexpr uint8_t PSBT_IN_SIGHASH = 0x03;
static constexpr uint8_t PSBT_IN_REDEEMSCRIPT = 0x04;
static constexpr uint8_t PSBT_IN_WITNESSSCRIPT = 0x05;
static constexpr uint8_t PSBT_IN_BIP32_DERIVATION = 0x06;
static constexpr uint8_t PSBT_IN_SCRIPTSIG = 0x07;
static constexpr uint8_t PSBT_IN_SCRIPTWITNESS = 0x08;
static constexpr uint8_t PSBT_IN_RIPEMD160 = 0x0A;
static constexpr uint8_t PSBT_IN_SHA256 = 0x0B;
static constexpr uint8_t PSBT_IN_HASH160 = 0x0C;
static constexpr uint8_t PSBT_IN_HASH256 = 0x0D;
static constexpr uint8_t PSBT_IN_TAP_KEY_SIG = 0x13;
static constexpr uint8_t PSBT_IN_TAP_SCRIPT_SIG = 0x14;
static constexpr uint8_t PSBT_IN_TAP_LEAF_SCRIPT = 0x15;
static constexpr uint8_t PSBT_IN_TAP_BIP32_DERIVATION = 0x16;
static constexpr uint8_t PSBT_IN_TAP_INTERNAL_KEY = 0x17;
static constexpr uint8_t PSBT_IN_TAP_MERKLE_ROOT = 0x18;
static constexpr uint8_t PSBT_IN_MUSIG2_PARTICIPANT_PUBKEYS = 0x1a;
static constexpr uint8_t PSBT_IN_MUSIG2_PUB_NONCE = 0x1b;
static constexpr uint8_t PSBT_IN_MUSIG2_PARTIAL_SIG = 0x1c;
static constexpr uint8_t PSBT_IN_PROPRIETARY = 0xFC;

// Output types
static constexpr uint8_t PSBT_OUT_REDEEMSCRIPT = 0x00;
static constexpr uint8_t PSBT_OUT_WITNESSSCRIPT = 0x01;
static constexpr uint8_t PSBT_OUT_BIP32_DERIVATION = 0x02;
static constexpr uint8_t PSBT_OUT_TAP_INTERNAL_KEY = 0x05;
static constexpr uint8_t PSBT_OUT_TAP_TREE = 0x06;
static constexpr uint8_t PSBT_OUT_TAP_BIP32_DERIVATION = 0x07;
static constexpr uint8_t PSBT_OUT_MUSIG2_PARTICIPANT_PUBKEYS = 0x08;
static constexpr uint8_t PSBT_OUT_PROPRIETARY = 0xFC;

// The separator is 0x00. Reading this in means that the unserializer can interpret it
// as a 0 length key which indicates that this is the separator. The separator has no value.
static constexpr uint8_t PSBT_SEPARATOR = 0x00;

// BIP 174 does not specify a maximum file size, but we set a limit anyway
// to prevent reading a stream indefinitely and running out of memory.
const std::streamsize MAX_FILE_SIZE_PSBT = 100000000; // 100 MB

// PSBT version number
static constexpr uint32_t PSBT_HIGHEST_VERSION = 0;

/** A structure for PSBT proprietary types */
struct PSBTProprietary
{
    uint64_t subtype;
    std::vector<unsigned char> identifier;
    std::vector<unsigned char> key;
    std::vector<unsigned char> value;

    bool operator<(const PSBTProprietary &b) const {
        return key < b.key;
    }
    bool operator==(const PSBTProprietary &b) const {
        return key == b.key;
    }
};

// Takes a stream and multiple arguments and serializes them as if first serialized into a vector and then into the stream
// The resulting output into the stream has the total serialized length of all of the objects followed by all objects concatenated with each other.
template<typename Stream, typename... X>
void SerializeToVector(Stream& s, const X&... args)
{
    SizeComputer sizecomp;
    SerializeMany(sizecomp, args...);
    WriteCompactSize(s, sizecomp.size());
    SerializeMany(s, args...);
}

// Takes a stream and multiple arguments and unserializes them first as a vector then each object individually in the order provided in the arguments
template<typename Stream, typename... X>
void UnserializeFromVector(Stream& s, X&&... args)
{
    size_t expected_size = ReadCompactSize(s);
    size_t remaining_before = s.size();
    UnserializeMany(s, args...);
    size_t remaining_after = s.size();
    if (remaining_after + expected_size != remaining_before) {
        throw std::ios_base::failure("Size of value was not the stated size");
    }
}

// Deserialize bytes of given length from the stream as a KeyOriginInfo
template<typename Stream>
KeyOriginInfo DeserializeKeyOrigin(Stream& s, uint64_t length)
{
    // Read in key path
    if (length % 4 || length == 0) {
        throw std::ios_base::failure("Invalid length for HD key path");
    }

    KeyOriginInfo hd_keypath;
    s >> hd_keypath.fingerprint;
    for (unsigned int i = 4; i < length; i += sizeof(uint32_t)) {
        uint32_t index;
        s >> index;
        hd_keypath.path.push_back(index);
    }
    return hd_keypath;
}

// Deserialize a length prefixed KeyOriginInfo from a stream
template<typename Stream>
void DeserializeHDKeypath(Stream& s, KeyOriginInfo& hd_keypath)
{
    hd_keypath = DeserializeKeyOrigin(s, ReadCompactSize(s));
}

// Deserialize HD keypaths into a map
template<typename Stream>
void DeserializeHDKeypaths(Stream& s, const std::vector<unsigned char>& key, std::map<CPubKey, KeyOriginInfo>& hd_keypaths)
{
    // Make sure that the key is the size of pubkey + 1
    if (key.size() != CPubKey::SIZE + 1 && key.size() != CPubKey::COMPRESSED_SIZE + 1) {
        throw std::ios_base::failure("Size of key was not the expected size for the type BIP32 keypath");
    }
    // Read in the pubkey from key
    CPubKey pubkey(key.begin() + 1, key.end());
    if (!pubkey.IsFullyValid()) {
       throw std::ios_base::failure("Invalid pubkey");
    }
    if (hd_keypaths.count(pubkey) > 0) {
        throw std::ios_base::failure("Duplicate Key, pubkey derivation path already provided");
    }

    KeyOriginInfo keypath;
    DeserializeHDKeypath(s, keypath);

    // Add to map
    hd_keypaths.emplace(pubkey, std::move(keypath));
}

// Serialize a KeyOriginInfo to a stream
template<typename Stream>
void SerializeKeyOrigin(Stream& s, KeyOriginInfo hd_keypath)
{
    s << hd_keypath.fingerprint;
    for (const auto& path : hd_keypath.path) {
        s << path;
    }
}

// Serialize a length prefixed KeyOriginInfo to a stream
template<typename Stream>
void SerializeHDKeypath(Stream& s, KeyOriginInfo hd_keypath)
{
    WriteCompactSize(s, (hd_keypath.path.size() + 1) * sizeof(uint32_t));
    SerializeKeyOrigin(s, hd_keypath);
}

// Serialize HD keypaths to a stream from a map
template<typename Stream>
void SerializeHDKeypaths(Stream& s, const std::map<CPubKey, KeyOriginInfo>& hd_keypaths, CompactSizeWriter type)
{
    for (const auto& keypath_pair : hd_keypaths) {
        if (!keypath_pair.first.IsValid()) {
            throw std::ios_base::failure("Invalid CPubKey being serialized");
        }
        SerializeToVector(s, type, std::span{keypath_pair.first});
        SerializeHDKeypath(s, keypath_pair.second);
    }
}

// Deserialize a PSBT_{IN/OUT}_MUSIG2_PARTICIPANT_PUBKEYS field
template<typename Stream>
void DeserializeMuSig2ParticipantPubkeys(Stream& s, SpanReader& skey, std::map<CPubKey, std::vector<CPubKey>>& out, std::string context)
{
    std::array<unsigned char, CPubKey::COMPRESSED_SIZE> agg_pubkey_bytes;
    skey >> std::as_writable_bytes(std::span{agg_pubkey_bytes});
    CPubKey agg_pubkey(agg_pubkey_bytes);

    std::vector<CPubKey> participants;
    std::vector<unsigned char> val;
    s >> val;
    SpanReader s_val{val};
    while (s_val.size() >= CPubKey::COMPRESSED_SIZE) {
        std::array<unsigned char, CPubKey::COMPRESSED_SIZE> part_pubkey_bytes;
        s_val >> std::as_writable_bytes(std::span{part_pubkey_bytes});
        participants.emplace_back(std::span{part_pubkey_bytes});
    }
    if (!s_val.empty()) {
        throw std::ios_base::failure(context + " musig2 participants pubkeys value size is not a multiple of 33");
    }

    out.emplace(agg_pubkey, participants);
}

// Deserialize the MuSig2 participant identifiers from PSBT_MUSIG2_{PUBNONCE/PARTIAL_SIG} fields
// Both fields contain the same data after the type byte - aggregate pubkey | participant pubkey | leaf script hash
template<typename Stream>
void DeserializeMuSig2ParticipantDataIdentifier(Stream& skey, CPubKey& agg_pub, CPubKey& part_pub, uint256& leaf_hash)
{
    leaf_hash.SetNull();

    std::array<unsigned char, CPubKey::COMPRESSED_SIZE> part_pubkey_bytes;
    std::array<unsigned char, CPubKey::COMPRESSED_SIZE> agg_pubkey_bytes;

    skey >> std::as_writable_bytes(std::span{part_pubkey_bytes}) >> std::as_writable_bytes(std::span{agg_pubkey_bytes});
    agg_pub.Set(agg_pubkey_bytes.begin(), agg_pubkey_bytes.end());
    part_pub.Set(part_pubkey_bytes.begin(), part_pubkey_bytes.end());

    if (!skey.empty()) {
        skey >> leaf_hash;
    }
}

/** A structure for PSBTs which contain per-input information */
struct PSBTInput
{
    CTransactionRef non_witness_utxo;
    CTxOut witness_utxo;
    CScript redeem_script;
    CScript witness_script;
    CScript final_script_sig;
    CScriptWitness final_script_witness;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
    std::map<CKeyID, SigPair> partial_sigs;
    std::map<uint160, std::vector<unsigned char>> ripemd160_preimages;
    std::map<uint256, std::vector<unsigned char>> sha256_preimages;
    std::map<uint160, std::vector<unsigned char>> hash160_preimages;
    std::map<uint256, std::vector<unsigned char>> hash256_preimages;

    // Taproot fields
    std::vector<unsigned char> m_tap_key_sig;
    std::map<std::pair<XOnlyPubKey, uint256>, std::vector<unsigned char>> m_tap_script_sigs;
    std::map<std::pair<std::vector<unsigned char>, int>, std::set<std::vector<unsigned char>, ShortestVectorFirstComparator>> m_tap_scripts;
    std::map<XOnlyPubKey, std::pair<std::set<uint256>, KeyOriginInfo>> m_tap_bip32_paths;
    XOnlyPubKey m_tap_internal_key;
    uint256 m_tap_merkle_root;

    // MuSig2 fields
    std::map<CPubKey, std::vector<CPubKey>> m_musig2_participants;
    // Key is the aggregate pubkey and the script leaf hash, value is a map of participant pubkey to pubnonce
    std::map<std::pair<CPubKey, uint256>, std::map<CPubKey, std::vector<uint8_t>>> m_musig2_pubnonces;
    // Key is the aggregate pubkey and the script leaf hash, value is a map of participant pubkey to partial_sig
    std::map<std::pair<CPubKey, uint256>, std::map<CPubKey, uint256>> m_musig2_partial_sigs;

    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::set<PSBTProprietary> m_proprietary;
    std::optional<int> sighash_type;

    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSBTInput& input);
    PSBTInput() = default;

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        // Write the utxo
        if (non_witness_utxo) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_NON_WITNESS_UTXO));
            SerializeToVector(s, TX_NO_WITNESS(non_witness_utxo));
        }
        if (!witness_utxo.IsNull()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_WITNESS_UTXO));
            SerializeToVector(s, witness_utxo);
        }

        if (final_script_sig.empty() && final_script_witness.IsNull()) {
            // Write any partial signatures
            for (const auto& sig_pair : partial_sigs) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_PARTIAL_SIG), std::span{sig_pair.second.first});
                s << sig_pair.second.second;
            }

            // Write the sighash type
            if (sighash_type != std::nullopt) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_SIGHASH));
                SerializeToVector(s, *sighash_type);
            }

            // Write the redeem script
            if (!redeem_script.empty()) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_REDEEMSCRIPT));
                s << redeem_script;
            }

            // Write the witness script
            if (!witness_script.empty()) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_WITNESSSCRIPT));
                s << witness_script;
            }

            // Write any hd keypaths
            SerializeHDKeypaths(s, hd_keypaths, CompactSizeWriter(PSBT_IN_BIP32_DERIVATION));

            // Write any ripemd160 preimage
            for (const auto& [hash, preimage] : ripemd160_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_RIPEMD160), std::span{hash});
                s << preimage;
            }

            // Write any sha256 preimage
            for (const auto& [hash, preimage] : sha256_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_SHA256), std::span{hash});
                s << preimage;
            }

            // Write any hash160 preimage
            for (const auto& [hash, preimage] : hash160_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_HASH160), std::span{hash});
                s << preimage;
            }

            // Write any hash256 preimage
            for (const auto& [hash, preimage] : hash256_preimages) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_HASH256), std::span{hash});
                s << preimage;
            }

            // Write taproot key sig
            if (!m_tap_key_sig.empty()) {
                SerializeToVector(s, PSBT_IN_TAP_KEY_SIG);
                s << m_tap_key_sig;
            }

            // Write taproot script sigs
            for (const auto& [pubkey_leaf, sig] : m_tap_script_sigs) {
                const auto& [xonly, leaf_hash] = pubkey_leaf;
                SerializeToVector(s, PSBT_IN_TAP_SCRIPT_SIG, xonly, leaf_hash);
                s << sig;
            }

            // Write taproot leaf scripts
            for (const auto& [leaf, control_blocks] : m_tap_scripts) {
                const auto& [script, leaf_ver] = leaf;
                for (const auto& control_block : control_blocks) {
                    SerializeToVector(s, PSBT_IN_TAP_LEAF_SCRIPT, std::span{control_block});
                    std::vector<unsigned char> value_v(script.begin(), script.end());
                    value_v.push_back((uint8_t)leaf_ver);
                    s << value_v;
                }
            }

            // Write taproot bip32 keypaths
            for (const auto& [xonly, leaf_origin] : m_tap_bip32_paths) {
                const auto& [leaf_hashes, origin] = leaf_origin;
                SerializeToVector(s, PSBT_IN_TAP_BIP32_DERIVATION, xonly);
                std::vector<unsigned char> value;
                VectorWriter s_value{value, 0};
                s_value << leaf_hashes;
                SerializeKeyOrigin(s_value, origin);
                s << value;
            }

            // Write taproot internal key
            if (!m_tap_internal_key.IsNull()) {
                SerializeToVector(s, PSBT_IN_TAP_INTERNAL_KEY);
                s << ToByteVector(m_tap_internal_key);
            }

            // Write taproot merkle root
            if (!m_tap_merkle_root.IsNull()) {
                SerializeToVector(s, PSBT_IN_TAP_MERKLE_ROOT);
                SerializeToVector(s, m_tap_merkle_root);
            }

            // Write MuSig2 Participants
            for (const auto& [agg_pubkey, part_pubs] : m_musig2_participants) {
                SerializeToVector(s, CompactSizeWriter(PSBT_IN_MUSIG2_PARTICIPANT_PUBKEYS), std::span{agg_pubkey});
                std::vector<unsigned char> value;
                VectorWriter s_value{value, 0};
                for (auto& pk : part_pubs) {
                    s_value << std::span{pk};
                }
                s << value;
            }

            // Write MuSig2 pubnonces
            for (const auto& [agg_pubkey_leaf_hash, pubnonces] : m_musig2_pubnonces) {
                const auto& [agg_pubkey, leaf_hash] = agg_pubkey_leaf_hash;
                for (const auto& [part_pubkey, pubnonce] : pubnonces) {
                    if (leaf_hash.IsNull()) {
                        SerializeToVector(s, CompactSizeWriter(PSBT_IN_MUSIG2_PUB_NONCE), std::span{part_pubkey}, std::span{agg_pubkey});
                    } else {
                        SerializeToVector(s, CompactSizeWriter(PSBT_IN_MUSIG2_PUB_NONCE), std::span{part_pubkey}, std::span{agg_pubkey}, leaf_hash);
                    }
                    s << pubnonce;
                }
            }

            // Write MuSig2 partial signatures
            for (const auto& [agg_pubkey_leaf_hash, psigs] : m_musig2_partial_sigs) {
                const auto& [agg_pubkey, leaf_hash] = agg_pubkey_leaf_hash;
                for (const auto& [pubkey, psig] : psigs) {
                    if (leaf_hash.IsNull()) {
                        SerializeToVector(s, CompactSizeWriter(PSBT_IN_MUSIG2_PARTIAL_SIG), std::span{pubkey}, std::span{agg_pubkey});
                    } else {
                        SerializeToVector(s, CompactSizeWriter(PSBT_IN_MUSIG2_PARTIAL_SIG), std::span{pubkey}, std::span{agg_pubkey}, leaf_hash);
                    }
                    SerializeToVector(s, psig);
                }
            }
        }

        // Write script sig
        if (!final_script_sig.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_SCRIPTSIG));
            s << final_script_sig;
        }
        // write script witness
        if (!final_script_witness.IsNull()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_IN_SCRIPTWITNESS));
            SerializeToVector(s, final_script_witness.stack);
        }

        // Write proprietary things
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }

        // Write unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        s << PSBT_SEPARATOR;
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Used for duplicate key detection
        std::set<std::vector<unsigned char>> key_lookup;

        // Read loop
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // Type is compact size uint at beginning of key
            SpanReader skey{key};
            uint64_t type = ReadCompactSize(skey);

            // Do stuff based on type
            switch(type) {
                case PSBT_IN_NON_WITNESS_UTXO:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input non-witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Non-witness utxo key is more than one byte type");
                    }
                    // Set the stream to unserialize with witness since this is always a valid network transaction
                    UnserializeFromVector(s, TX_WITH_WITNESS(non_witness_utxo));
                    break;
                }
                case PSBT_IN_WITNESS_UTXO:
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input witness utxo already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Witness utxo key is more than one byte type");
                    }
                    UnserializeFromVector(s, witness_utxo);
                    break;
                case PSBT_IN_PARTIAL_SIG:
                {
                    // Make sure that the key is the size of pubkey + 1
                    if (key.size() != CPubKey::SIZE + 1 && key.size() != CPubKey::COMPRESSED_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type partial signature pubkey");
                    }
                    // Read in the pubkey from key
                    CPubKey pubkey(key.begin() + 1, key.end());
                    if (!pubkey.IsFullyValid()) {
                       throw std::ios_base::failure("Invalid pubkey");
                    }
                    if (partial_sigs.count(pubkey.GetID()) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input partial signature for pubkey already provided");
                    }

                    // Read in the signature from value
                    std::vector<unsigned char> sig;
                    s >> sig;

                    // Check that the signature is validly encoded
                    if (sig.empty() || !CheckSignatureEncoding(sig, SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_STRICTENC, nullptr)) {
                        throw std::ios_base::failure("Signature is not a valid encoding");
                    }

                    // Add to list
                    partial_sigs.emplace(pubkey.GetID(), SigPair(pubkey, std::move(sig)));
                    break;
                }
                case PSBT_IN_SIGHASH:
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input sighash type already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Sighash type key is more than one byte type");
                    }
                    int sighash;
                    UnserializeFromVector(s, sighash);
                    sighash_type = sighash;
                    break;
                case PSBT_IN_REDEEMSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PSBT_IN_WITNESSSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PSBT_IN_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                case PSBT_IN_SCRIPTSIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptSig already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptSig key is more than one byte type");
                    }
                    s >> final_script_sig;
                    break;
                }
                case PSBT_IN_SCRIPTWITNESS:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input final scriptWitness already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Final scriptWitness key is more than one byte type");
                    }
                    UnserializeFromVector(s, final_script_witness.stack);
                    break;
                }
                case PSBT_IN_RIPEMD160:
                {
                    // Make sure that the key is the size of a ripemd160 hash + 1
                    if (key.size() != CRIPEMD160::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type ripemd160 preimage");
                    }
                    // Read in the hash from key
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint160 hash(hash_vec);
                    if (ripemd160_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input ripemd160 preimage already provided");
                    }

                    // Read in the preimage from value
                    std::vector<unsigned char> preimage;
                    s >> preimage;

                    // Add to preimages list
                    ripemd160_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_SHA256:
                {
                    // Make sure that the key is the size of a sha256 hash + 1
                    if (key.size() != CSHA256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type sha256 preimage");
                    }
                    // Read in the hash from key
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint256 hash(hash_vec);
                    if (sha256_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input sha256 preimage already provided");
                    }

                    // Read in the preimage from value
                    std::vector<unsigned char> preimage;
                    s >> preimage;

                    // Add to preimages list
                    sha256_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_HASH160:
                {
                    // Make sure that the key is the size of a hash160 hash + 1
                    if (key.size() != CHash160::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type hash160 preimage");
                    }
                    // Read in the hash from key
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint160 hash(hash_vec);
                    if (hash160_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input hash160 preimage already provided");
                    }

                    // Read in the preimage from value
                    std::vector<unsigned char> preimage;
                    s >> preimage;

                    // Add to preimages list
                    hash160_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_HASH256:
                {
                    // Make sure that the key is the size of a hash256 hash + 1
                    if (key.size() != CHash256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type hash256 preimage");
                    }
                    // Read in the hash from key
                    std::vector<unsigned char> hash_vec(key.begin() + 1, key.end());
                    uint256 hash(hash_vec);
                    if (hash256_preimages.count(hash) > 0) {
                        throw std::ios_base::failure("Duplicate Key, input hash256 preimage already provided");
                    }

                    // Read in the preimage from value
                    std::vector<unsigned char> preimage;
                    s >> preimage;

                    // Add to preimages list
                    hash256_preimages.emplace(hash, std::move(preimage));
                    break;
                }
                case PSBT_IN_TAP_KEY_SIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot key signature already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot key signature key is more than one byte type");
                    }
                    s >> m_tap_key_sig;
                    if (m_tap_key_sig.size() < 64) {
                        throw std::ios_base::failure("Input Taproot key path signature is shorter than 64 bytes");
                    } else if (m_tap_key_sig.size() > 65) {
                        throw std::ios_base::failure("Input Taproot key path signature is longer than 65 bytes");
                    }
                    break;
                }
                case PSBT_IN_TAP_SCRIPT_SIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot script signature already provided");
                    } else if (key.size() != 65) {
                        throw std::ios_base::failure("Input Taproot script signature key is not 65 bytes");
                    }
                    SpanReader s_key{std::span{key}.subspan(1)};
                    XOnlyPubKey xonly;
                    uint256 hash;
                    s_key >> xonly;
                    s_key >> hash;
                    std::vector<unsigned char> sig;
                    s >> sig;
                    if (sig.size() < 64) {
                        throw std::ios_base::failure("Input Taproot script path signature is shorter than 64 bytes");
                    } else if (sig.size() > 65) {
                        throw std::ios_base::failure("Input Taproot script path signature is longer than 65 bytes");
                    }
                    m_tap_script_sigs.emplace(std::make_pair(xonly, hash), sig);
                    break;
                }
                case PSBT_IN_TAP_LEAF_SCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot leaf script already provided");
                    } else if (key.size() < 34) {
                        throw std::ios_base::failure("Taproot leaf script key is not at least 34 bytes");
                    } else if ((key.size() - 2) % 32 != 0) {
                        throw std::ios_base::failure("Input Taproot leaf script key's control block size is not valid");
                    }
                    std::vector<unsigned char> script_v;
                    s >> script_v;
                    if (script_v.empty()) {
                        throw std::ios_base::failure("Input Taproot leaf script must be at least 1 byte");
                    }
                    uint8_t leaf_ver = script_v.back();
                    script_v.pop_back();
                    const auto leaf_script = std::make_pair(script_v, (int)leaf_ver);
                    m_tap_scripts[leaf_script].insert(std::vector<unsigned char>(key.begin() + 1, key.end()));
                    break;
                }
                case PSBT_IN_TAP_BIP32_DERIVATION:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot BIP32 keypath already provided");
                    } else if (key.size() != 33) {
                        throw std::ios_base::failure("Input Taproot BIP32 keypath key is not at 33 bytes");
                    }
                    SpanReader s_key{std::span{key}.subspan(1)};
                    XOnlyPubKey xonly;
                    s_key >> xonly;
                    std::set<uint256> leaf_hashes;
                    uint64_t value_len = ReadCompactSize(s);
                    size_t before_hashes = s.size();
                    s >> leaf_hashes;
                    size_t after_hashes = s.size();
                    size_t hashes_len = before_hashes - after_hashes;
                    if (hashes_len > value_len) {
                        throw std::ios_base::failure("Input Taproot BIP32 keypath has an invalid length");
                    }
                    size_t origin_len = value_len - hashes_len;
                    m_tap_bip32_paths.emplace(xonly, std::make_pair(leaf_hashes, DeserializeKeyOrigin(s, origin_len)));
                    break;
                }
                case PSBT_IN_TAP_INTERNAL_KEY:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot internal key already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot internal key key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_internal_key);
                    break;
                }
                case PSBT_IN_TAP_MERKLE_ROOT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input Taproot merkle root already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Input Taproot merkle root key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_merkle_root);
                    break;
                }
                case PSBT_IN_MUSIG2_PARTICIPANT_PUBKEYS:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input participant pubkeys for an aggregate key already provided");
                    } else if (key.size() != CPubKey::COMPRESSED_SIZE + 1) {
                        throw std::ios_base::failure("Input musig2 participants pubkeys aggregate key is not 34 bytes");
                    }
                    DeserializeMuSig2ParticipantPubkeys(s, skey, m_musig2_participants, std::string{"Input"});
                    break;
                }
                case PSBT_IN_MUSIG2_PUB_NONCE:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input musig2 pubnonce already provided");
                    } else if (key.size() != 2 * CPubKey::COMPRESSED_SIZE + 1 && key.size() != 2 * CPubKey::COMPRESSED_SIZE + CSHA256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Input musig2 pubnonce key is not expected size of 67 or 99 bytes");
                    }
                    CPubKey agg_pub, part_pub;
                    uint256 leaf_hash;
                    DeserializeMuSig2ParticipantDataIdentifier(skey, agg_pub, part_pub, leaf_hash);

                    std::vector<uint8_t> pubnonce;
                    s >> pubnonce;
                    if (pubnonce.size() != 66) {
                        throw std::ios_base::failure("Input musig2 pubnonce value is not 66 bytes");
                    }

                    m_musig2_pubnonces[std::make_pair(agg_pub, leaf_hash)].emplace(part_pub, pubnonce);
                    break;
                }
                case PSBT_IN_MUSIG2_PARTIAL_SIG:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, input musig2 partial sig already provided");
                    } else if (key.size() != 2 * CPubKey::COMPRESSED_SIZE + 1 && key.size() != 2 * CPubKey::COMPRESSED_SIZE + CSHA256::OUTPUT_SIZE + 1) {
                        throw std::ios_base::failure("Input musig2 partial sig key is not expected size of 67 or 99 bytes");
                    }
                    CPubKey agg_pub, part_pub;
                    uint256 leaf_hash;
                    DeserializeMuSig2ParticipantDataIdentifier(skey, agg_pub, part_pub, leaf_hash);

                    uint256 partial_sig;
                    UnserializeFromVector(s, partial_sig);

                    m_musig2_partial_sigs[std::make_pair(agg_pub, leaf_hash)].emplace(part_pub, partial_sig);
                    break;
                }
                case PSBT_IN_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;

                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                // Unknown stuff
                default:
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an input map");
        }
    }

    template <typename Stream>
    PSBTInput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

/** A structure for PSBTs which contains per output information */
struct PSBTOutput
{
    CScript redeem_script;
    CScript witness_script;
    std::map<CPubKey, KeyOriginInfo> hd_keypaths;
    XOnlyPubKey m_tap_internal_key;
    std::vector<std::tuple<uint8_t, uint8_t, std::vector<unsigned char>>> m_tap_tree;
    std::map<XOnlyPubKey, std::pair<std::set<uint256>, KeyOriginInfo>> m_tap_bip32_paths;
    std::map<CPubKey, std::vector<CPubKey>> m_musig2_participants;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::set<PSBTProprietary> m_proprietary;

    bool IsNull() const;
    void FillSignatureData(SignatureData& sigdata) const;
    void FromSignatureData(const SignatureData& sigdata);
    void Merge(const PSBTOutput& output);
    PSBTOutput() = default;

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        // Write the redeem script
        if (!redeem_script.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_OUT_REDEEMSCRIPT));
            s << redeem_script;
        }

        // Write the witness script
        if (!witness_script.empty()) {
            SerializeToVector(s, CompactSizeWriter(PSBT_OUT_WITNESSSCRIPT));
            s << witness_script;
        }

        // Write any hd keypaths
        SerializeHDKeypaths(s, hd_keypaths, CompactSizeWriter(PSBT_OUT_BIP32_DERIVATION));

        // Write proprietary things
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }

        // Write taproot internal key
        if (!m_tap_internal_key.IsNull()) {
            SerializeToVector(s, PSBT_OUT_TAP_INTERNAL_KEY);
            s << ToByteVector(m_tap_internal_key);
        }

        // Write taproot tree
        if (!m_tap_tree.empty()) {
            SerializeToVector(s, PSBT_OUT_TAP_TREE);
            std::vector<unsigned char> value;
            VectorWriter s_value{value, 0};
            for (const auto& [depth, leaf_ver, script] : m_tap_tree) {
                s_value << depth;
                s_value << leaf_ver;
                s_value << script;
            }
            s << value;
        }

        // Write taproot bip32 keypaths
        for (const auto& [xonly, leaf] : m_tap_bip32_paths) {
            const auto& [leaf_hashes, origin] = leaf;
            SerializeToVector(s, PSBT_OUT_TAP_BIP32_DERIVATION, xonly);
            std::vector<unsigned char> value;
            VectorWriter s_value{value, 0};
            s_value << leaf_hashes;
            SerializeKeyOrigin(s_value, origin);
            s << value;
        }

        // Write MuSig2 Participants
        for (const auto& [agg_pubkey, part_pubs] : m_musig2_participants) {
            SerializeToVector(s, CompactSizeWriter(PSBT_OUT_MUSIG2_PARTICIPANT_PUBKEYS), std::span{agg_pubkey});
            std::vector<unsigned char> value;
            VectorWriter s_value{value, 0};
            for (auto& pk : part_pubs) {
                s_value << std::span{pk};
            }
            s << value;
        }

        // Write unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        s << PSBT_SEPARATOR;
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Used for duplicate key detection
        std::set<std::vector<unsigned char>> key_lookup;

        // Read loop
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // Type is compact size uint at beginning of key
            SpanReader skey{key};
            uint64_t type = ReadCompactSize(skey);

            // Do stuff based on type
            switch(type) {
                case PSBT_OUT_REDEEMSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output redeemScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output redeemScript key is more than one byte type");
                    }
                    s >> redeem_script;
                    break;
                }
                case PSBT_OUT_WITNESSSCRIPT:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output witnessScript already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output witnessScript key is more than one byte type");
                    }
                    s >> witness_script;
                    break;
                }
                case PSBT_OUT_BIP32_DERIVATION:
                {
                    DeserializeHDKeypaths(s, key, hd_keypaths);
                    break;
                }
                case PSBT_OUT_TAP_INTERNAL_KEY:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot internal key already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output Taproot internal key key is more than one byte type");
                    }
                    UnserializeFromVector(s, m_tap_internal_key);
                    break;
                }
                case PSBT_OUT_TAP_TREE:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot tree already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Output Taproot tree key is more than one byte type");
                    }
                    std::vector<unsigned char> tree_v;
                    s >> tree_v;
                    SpanReader s_tree{tree_v};
                    if (s_tree.empty()) {
                        throw std::ios_base::failure("Output Taproot tree must not be empty");
                    }
                    TaprootBuilder builder;
                    while (!s_tree.empty()) {
                        uint8_t depth;
                        uint8_t leaf_ver;
                        std::vector<unsigned char> script;
                        s_tree >> depth;
                        s_tree >> leaf_ver;
                        s_tree >> script;
                        if (depth > TAPROOT_CONTROL_MAX_NODE_COUNT) {
                            throw std::ios_base::failure("Output Taproot tree has as leaf greater than Taproot maximum depth");
                        }
                        if ((leaf_ver & ~TAPROOT_LEAF_MASK) != 0) {
                            throw std::ios_base::failure("Output Taproot tree has a leaf with an invalid leaf version");
                        }
                        m_tap_tree.emplace_back(depth, leaf_ver, script);
                        builder.Add((int)depth, script, (int)leaf_ver, /*track=*/true);
                    }
                    if (!builder.IsComplete()) {
                        throw std::ios_base::failure("Output Taproot tree is malformed");
                    }
                    break;
                }
                case PSBT_OUT_TAP_BIP32_DERIVATION:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output Taproot BIP32 keypath already provided");
                    } else if (key.size() != 33) {
                        throw std::ios_base::failure("Output Taproot BIP32 keypath key is not at 33 bytes");
                    }
                    XOnlyPubKey xonly(uint256(std::span<uint8_t>(key).last(32)));
                    std::set<uint256> leaf_hashes;
                    uint64_t value_len = ReadCompactSize(s);
                    size_t before_hashes = s.size();
                    s >> leaf_hashes;
                    size_t after_hashes = s.size();
                    size_t hashes_len = before_hashes - after_hashes;
                    if (hashes_len > value_len) {
                        throw std::ios_base::failure("Output Taproot BIP32 keypath has an invalid length");
                    }
                    size_t origin_len = value_len - hashes_len;
                    m_tap_bip32_paths.emplace(xonly, std::make_pair(leaf_hashes, DeserializeKeyOrigin(s, origin_len)));
                    break;
                }
                case PSBT_OUT_MUSIG2_PARTICIPANT_PUBKEYS:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, output participant pubkeys for an aggregate key already provided");
                    } else if (key.size() != CPubKey::COMPRESSED_SIZE + 1) {
                        throw std::ios_base::failure("Output musig2 participants pubkeys aggregate key is not 34 bytes");
                    }
                    DeserializeMuSig2ParticipantPubkeys(s, skey, m_musig2_participants, std::string{"Output"});
                    break;
                }
                case PSBT_OUT_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;

                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                // Unknown stuff
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                    break;
                }
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of an output map");
        }
    }

    template <typename Stream>
    PSBTOutput(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

/** A version of CTransaction with the PSBT format*/
struct PartiallySignedTransaction
{
    std::optional<CMutableTransaction> tx;
    // We use a vector of CExtPubKey in the event that there happens to be the same KeyOriginInfos for different CExtPubKeys
    // Note that this map swaps the key and values from the serialization
    std::map<KeyOriginInfo, std::set<CExtPubKey>> m_xpubs;
    std::vector<PSBTInput> inputs;
    std::vector<PSBTOutput> outputs;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> unknown;
    std::optional<uint32_t> m_version;
    std::set<PSBTProprietary> m_proprietary;

    bool IsNull() const;
    uint32_t GetVersion() const;

    /** Merge psbt into this. The two psbts must have the same underlying CTransaction (i.e. the
      * same actual Bitcoin transaction.) Returns true if the merge succeeded, false otherwise. */
    [[nodiscard]] bool Merge(const PartiallySignedTransaction& psbt);
    bool AddInput(const CTxIn& txin, PSBTInput& psbtin);
    bool AddOutput(const CTxOut& txout, const PSBTOutput& psbtout);
    PartiallySignedTransaction() = default;
    explicit PartiallySignedTransaction(const CMutableTransaction& tx);
    /**
     * Finds the UTXO for a given input index
     *
     * @param[out] utxo The UTXO of the input if found
     * @param[in] input_index Index of the input to retrieve the UTXO of
     * @return Whether the UTXO for the specified input was found
     */
    bool GetInputUTXO(CTxOut& utxo, int input_index) const;

    template <typename Stream>
    inline void Serialize(Stream& s) const {

        // magic bytes
        s << PSBT_MAGIC_BYTES;

        // unsigned tx flag
        SerializeToVector(s, CompactSizeWriter(PSBT_GLOBAL_UNSIGNED_TX));

        // Write serialized tx to a stream
        SerializeToVector(s, TX_NO_WITNESS(*tx));

        // Write xpubs
        for (const auto& xpub_pair : m_xpubs) {
            for (const auto& xpub : xpub_pair.second) {
                unsigned char ser_xpub[BIP32_EXTKEY_WITH_VERSION_SIZE];
                xpub.EncodeWithVersion(ser_xpub);
                // Note that the serialization swaps the key and value
                // The xpub is the key (for uniqueness) while the path is the value
                SerializeToVector(s, PSBT_GLOBAL_XPUB, ser_xpub);
                SerializeHDKeypath(s, xpub_pair.first);
            }
        }

        // PSBT version
        if (GetVersion() > 0) {
            SerializeToVector(s, CompactSizeWriter(PSBT_GLOBAL_VERSION));
            SerializeToVector(s, *m_version);
        }

        // Write proprietary things
        for (const auto& entry : m_proprietary) {
            s << entry.key;
            s << entry.value;
        }

        // Write the unknown things
        for (auto& entry : unknown) {
            s << entry.first;
            s << entry.second;
        }

        // Separator
        s << PSBT_SEPARATOR;

        // Write inputs
        for (const PSBTInput& input : inputs) {
            s << input;
        }
        // Write outputs
        for (const PSBTOutput& output : outputs) {
            s << output;
        }
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        // Read the magic bytes
        uint8_t magic[5];
        s >> magic;
        if (!std::equal(magic, magic + 5, PSBT_MAGIC_BYTES)) {
            throw std::ios_base::failure("Invalid PSBT magic bytes");
        }

        // Used for duplicate key detection
        std::set<std::vector<unsigned char>> key_lookup;

        // Track the global xpubs we have already seen. Just for sanity checking
        std::set<CExtPubKey> global_xpubs;

        // Read global data
        bool found_sep = false;
        while(!s.empty()) {
            // Read
            std::vector<unsigned char> key;
            s >> key;

            // the key is empty if that was actually a separator byte
            // This is a special case for key lengths 0 as those are not allowed (except for separator)
            if (key.empty()) {
                found_sep = true;
                break;
            }

            // Type is compact size uint at beginning of key
            SpanReader skey{key};
            uint64_t type = ReadCompactSize(skey);

            // Do stuff based on type
            switch(type) {
                case PSBT_GLOBAL_UNSIGNED_TX:
                {
                    if (!key_lookup.emplace(key).second) {
                        throw std::ios_base::failure("Duplicate Key, unsigned tx already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Global unsigned tx key is more than one byte type");
                    }
                    CMutableTransaction mtx;
                    // Set the stream to serialize with non-witness since this should always be non-witness
                    UnserializeFromVector(s, TX_NO_WITNESS(mtx));
                    tx = std::move(mtx);
                    // Make sure that all scriptSigs and scriptWitnesses are empty
                    for (const CTxIn& txin : tx->vin) {
                        if (!txin.scriptSig.empty() || !txin.scriptWitness.IsNull()) {
                            throw std::ios_base::failure("Unsigned tx does not have empty scriptSigs and scriptWitnesses.");
                        }
                    }
                    break;
                }
                case PSBT_GLOBAL_XPUB:
                {
                    if (key.size() != BIP32_EXTKEY_WITH_VERSION_SIZE + 1) {
                        throw std::ios_base::failure("Size of key was not the expected size for the type global xpub");
                    }
                    // Read in the xpub from key
                    CExtPubKey xpub;
                    xpub.DecodeWithVersion(&key.data()[1]);
                    if (!xpub.pubkey.IsFullyValid()) {
                       throw std::ios_base::failure("Invalid pubkey");
                    }
                    if (global_xpubs.count(xpub) > 0) {
                       throw std::ios_base::failure("Duplicate key, global xpub already provided");
                    }
                    global_xpubs.insert(xpub);
                    // Read in the keypath from stream
                    KeyOriginInfo keypath;
                    DeserializeHDKeypath(s, keypath);

                    // Note that we store these swapped to make searches faster.
                    // Serialization uses xpub -> keypath to enqure key uniqueness
                    if (m_xpubs.count(keypath) == 0) {
                        // Make a new set to put the xpub in
                        m_xpubs[keypath] = {xpub};
                    } else {
                        // Insert xpub into existing set
                        m_xpubs[keypath].insert(xpub);
                    }
                    break;
                }
                case PSBT_GLOBAL_VERSION:
                {
                    if (m_version) {
                        throw std::ios_base::failure("Duplicate Key, version already provided");
                    } else if (key.size() != 1) {
                        throw std::ios_base::failure("Global version key is more than one byte type");
                    }
                    uint32_t v;
                    UnserializeFromVector(s, v);
                    m_version = v;
                    if (*m_version > PSBT_HIGHEST_VERSION) {
                        throw std::ios_base::failure("Unsupported version number");
                    }
                    break;
                }
                case PSBT_GLOBAL_PROPRIETARY:
                {
                    PSBTProprietary this_prop;
                    skey >> this_prop.identifier;
                    this_prop.subtype = ReadCompactSize(skey);
                    this_prop.key = key;

                    if (m_proprietary.count(this_prop) > 0) {
                        throw std::ios_base::failure("Duplicate Key, proprietary key already found");
                    }
                    s >> this_prop.value;
                    m_proprietary.insert(this_prop);
                    break;
                }
                // Unknown stuff
                default: {
                    if (unknown.count(key) > 0) {
                        throw std::ios_base::failure("Duplicate Key, key for unknown value already provided");
                    }
                    // Read in the value
                    std::vector<unsigned char> val_bytes;
                    s >> val_bytes;
                    unknown.emplace(std::move(key), std::move(val_bytes));
                }
            }
        }

        if (!found_sep) {
            throw std::ios_base::failure("Separator is missing at the end of the global map");
        }

        // Make sure that we got an unsigned tx
        if (!tx) {
            throw std::ios_base::failure("No unsigned transaction was provided");
        }

        // Read input data
        unsigned int i = 0;
        while (!s.empty() && i < tx->vin.size()) {
            PSBTInput input;
            s >> input;
            inputs.push_back(input);

            // Make sure the non-witness utxo matches the outpoint
            if (input.non_witness_utxo) {
                if (input.non_witness_utxo->GetHash() != tx->vin[i].prevout.hash) {
                    throw std::ios_base::failure("Non-witness UTXO does not match outpoint hash");
                }
                if (tx->vin[i].prevout.n >= input.non_witness_utxo->vout.size()) {
                    throw std::ios_base::failure("Input specifies output index that does not exist");
                }
            }
            ++i;
        }
        // Make sure that the number of inputs matches the number of inputs in the transaction
        if (inputs.size() != tx->vin.size()) {
            throw std::ios_base::failure("Inputs provided does not match the number of inputs in transaction.");
        }

        // Read output data
        i = 0;
        while (!s.empty() && i < tx->vout.size()) {
            PSBTOutput output;
            s >> output;
            outputs.push_back(output);
            ++i;
        }
        // Make sure that the number of outputs matches the number of outputs in the transaction
        if (outputs.size() != tx->vout.size()) {
            throw std::ios_base::failure("Outputs provided does not match the number of outputs in transaction.");
        }
    }

    template <typename Stream>
    PartiallySignedTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }
};

enum class PSBTRole {
    CREATOR,
    UPDATER,
    SIGNER,
    FINALIZER,
    EXTRACTOR
};

std::string PSBTRoleName(PSBTRole role);

/** Compute a PrecomputedTransactionData object from a psbt. */
PrecomputedTransactionData PrecomputePSBTData(const PartiallySignedTransaction& psbt);

/** Checks whether a PSBTInput is already signed by checking for non-null finalized fields. */
bool PSBTInputSigned(const PSBTInput& input);

/** Checks whether a PSBTInput is already signed by doing script verification using final fields. */
bool PSBTInputSignedAndVerified(const PartiallySignedTransaction psbt, unsigned int input_index, const PrecomputedTransactionData* txdata);

/** Signs a PSBTInput, verifying that all provided data matches what is being signed.
 *
 * txdata should be the output of PrecomputePSBTData (which can be shared across
 * multiple SignPSBTInput calls). If it is nullptr, a dummy signature will be created.
 **/
[[nodiscard]] PSBTError SignPSBTInput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index, const PrecomputedTransactionData* txdata, std::optional<int> sighash = std::nullopt, SignatureData* out_sigdata = nullptr, bool finalize = true);

/**  Reduces the size of the PSBT by dropping unnecessary `non_witness_utxos` (i.e. complete previous transactions) from a psbt when all inputs are segwit v1. */
void RemoveUnnecessaryTransactions(PartiallySignedTransaction& psbtx);

/** Counts the unsigned inputs of a PSBT. */
size_t CountPSBTUnsignedInputs(const PartiallySignedTransaction& psbt);

/** Updates a PSBTOutput with information from provider.
 *
 * This fills in the redeem_script, witness_script, and hd_keypaths where possible.
 */
void UpdatePSBTOutput(const SigningProvider& provider, PartiallySignedTransaction& psbt, int index);

/**
 * Finalizes a PSBT if possible, combining partial signatures.
 *
 * @param[in,out] psbtx PartiallySignedTransaction to finalize
 * return True if the PSBT is now complete, false otherwise
 */
bool FinalizePSBT(PartiallySignedTransaction& psbtx);

/**
 * Finalizes a PSBT if possible, and extracts it to a CMutableTransaction if it could be finalized.
 *
 * @param[in]  psbtx PartiallySignedTransaction
 * @param[out] result CMutableTransaction representing the complete transaction, if successful
 * @return True if we successfully extracted the transaction, false otherwise
 */
bool FinalizeAndExtractPSBT(PartiallySignedTransaction& psbtx, CMutableTransaction& result);

/**
 * Combines PSBTs with the same underlying transaction, resulting in a single PSBT with all partial signatures from each input.
 *
 * @param[out] out   the combined PSBT, if successful
 * @param[in]  psbtxs the PSBTs to combine
 * @return True if we successfully combined the transactions, false if they were not compatible
 */
[[nodiscard]] bool CombinePSBTs(PartiallySignedTransaction& out, const std::vector<PartiallySignedTransaction>& psbtxs);

//! Decode a base64ed PSBT into a PartiallySignedTransaction
[[nodiscard]] bool DecodeBase64PSBT(PartiallySignedTransaction& decoded_psbt, const std::string& base64_psbt, std::string& error);
//! Decode a raw (binary blob) PSBT into a PartiallySignedTransaction
[[nodiscard]] bool DecodeRawPSBT(PartiallySignedTransaction& decoded_psbt, std::span<const std::byte> raw_psbt, std::string& error);

#endif // BITCOIN_PSBT_H
