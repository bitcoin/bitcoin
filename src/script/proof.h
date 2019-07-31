// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_PROOF_H
#define BITCOIN_SCRIPT_PROOF_H

#include <script/bitcoinconsensus.h> // Block script flags
#include <script/interpreter.h>      // SimpleSignatureChecker
#include <script/standard.h>         // CTxDestination
#include <script/sign.h>             // ProduceSignature, SimpleSignatureCreator
#include <serialize.h>
#include <outputtype.h>              // GetDestinationForKey
#include <policy/policy.h>           // for STANDARD_SCRIPT_VERIFY_FLAGS
#include <hash.h>                    // CHashWriter
#include <util/memory.h>             // MakeUnique

extern const std::string MESSAGE_MAGIC;

namespace proof {

class dest_unavailable_error : public std::runtime_error { public: explicit dest_unavailable_error(const std::string& str = "Destination is not available") : std::runtime_error(str) {} };
class privkey_unavailable_error : public std::runtime_error { public: explicit privkey_unavailable_error(const std::string& str = "Private key is not available") : std::runtime_error(str) {} };
class signing_error : public std::runtime_error { public: explicit signing_error(const std::string& str = "Sign failed") : std::runtime_error(str) {} };
class serialization_error : public std::runtime_error { public: explicit serialization_error(const std::string& str = "Serialization error") : std::runtime_error(str) {} };

/**
 * Result codes ordered numerically by severity, so that A is reported, if A <= B and A and B are
 * two results for a verification attempt.
 */
enum class Result : int8_t {
    Valid = 0,           //!< All proofs were deemed valid.
    Inconclusive = -1,   //!< One or several of the given proofs used unknown opcodes or the scriptPubKey had an unknown witness version, perhaps due to the verifying node being outdated.
    Incomplete = -2,     //!< One or several of the given challenges had an empty proof. The prover may need some other entity to complete the proof.
    Invalid = -3,        //!< One or more of the given proofs were invalid
    Error = -4,          //!< An error was encountered
};

inline std::string ResultString(const Result r) {
    static const char *strings[] = {"ERROR", "INVALID", "INCOMPLETE", "INCONCLUSIVE", "VALID"};
    return r < Result::Error || r > Result::Valid ? "???" : strings[(int)r + 4];
}

inline Result ResultFromBool(bool success) {
    return success ? Result::Valid : Result::Invalid;
}

static constexpr uint32_t BIP322_FORMAT_VERSION = 1;

struct Header {
    uint32_t m_version;     //!< Format version
    uint8_t m_entries;      //!< Number of proof entries

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        m_version = BIP322_FORMAT_VERSION;
        READWRITE(m_version);
        if (m_version > BIP322_FORMAT_VERSION) throw serialization_error("Unknown BIP322 proof format; you may need to update your node");
        if (m_version < BIP322_FORMAT_VERSION) throw serialization_error("Outdated BIP322 proof format; ask prover to re-sign using newer software");
        READWRITE(m_entries);
    }
};

struct SignatureProof {
    CScript m_scriptsig; //!< ScriptSig data
    CScriptWitness m_witness;   //!< Witness

    explicit SignatureProof(const SignatureData& sigdata = SignatureData()) {
        m_scriptsig = sigdata.scriptSig;
        m_witness = sigdata.scriptWitness;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(m_scriptsig);
        READWRITE(m_witness.stack);
    }
};

struct Purpose {
    template<typename T>
    Result Prepare(const T& input, std::set<T>& inputs_out) const {
        if (inputs_out.count(input)) return Result::Error;
        inputs_out.insert(input);
        return Result::Valid;
    }
};

/**
 * Purpose: SignMessage
 *
 * Generate a sighash based on a scriptPubKey and a message. Emits VALID on success.
 */
struct SignMessage: Purpose {
    CScript m_scriptpubkey;

    explicit SignMessage(const CScript& scriptpubkey = CScript()) : m_scriptpubkey(scriptpubkey) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(m_scriptpubkey);
    }

    Result Prepare(const std::string& message, std::set<CScript>& inputs_out, uint256& sighash_out, CScript& spk_out) const;

    inline std::set<CScript> InputsSet() { return std::set<CScript>(); }
};

struct Proof: public Header {
    std::vector<SignatureProof> m_items;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        Header::SerializationOp(s, ser_action);
        m_items.resize(m_entries);
        for (auto& e : m_items) {
            READWRITE(e);
        }
    }
};

template<typename T>
struct BaseWorkSpace {
    std::map<CTxDestination, CKey> privkeys;
    std::vector<T> m_challenge;
    Proof m_proof;

    virtual void GenerateSingleProof(std::unique_ptr<SigningProvider> sp, const uint256& sighash, const CScript& scriptPubKey) = 0;
    virtual Result VerifySingleProof(unsigned int flags, const SignatureProof& proof, const uint256& sighash, const CScript& scriptPubKey) = 0;

    void Prove(const std::string& message, std::unique_ptr<SigningProvider> signingProvider = nullptr) {
        assert(m_challenge.size() <= 255);
        m_proof.m_items.clear();
        m_proof.m_version = BIP322_FORMAT_VERSION;
        m_proof.m_entries = m_challenge.size();
        if (m_challenge.size() == 0) return;
        auto inputs = m_challenge.back().InputsSet();
        uint256 sighash;
        CScript scriptPubKey;
        for (auto& c : m_challenge) {
            auto r = c.Prepare(message, inputs, sighash, scriptPubKey);
            if (r != Result::Valid) {
                throw std::runtime_error("Prepare call failed (error: " + ResultString(r) + ")");
            }
            CTxDestination destination;
            if (!ExtractDestination(scriptPubKey, destination)) {
                throw dest_unavailable_error();
            }
            CKey secret;
            if (privkeys.count(destination)) {
                secret = privkeys.at(destination);
            } else if (signingProvider) {
                auto keyid = GetKeyForDestination(*signingProvider, destination);
                if (keyid.IsNull()) {
                    throw privkey_unavailable_error("ScriptPubKey does not refer to a key (note: multisig is not yet supported)");
                }
                if (!signingProvider->GetKey(keyid, secret)) {
                    throw privkey_unavailable_error("Private key for scriptPubKey is not known");
                }
            } else {
                throw privkey_unavailable_error("Failed to obtain private key for destination");
            }
            std::unique_ptr<FillableSigningProvider> sp = MakeUnique<FillableSigningProvider>();
            sp->AddKey(secret);
            GenerateSingleProof(std::move(sp), sighash, scriptPubKey);
        }
    }

    Result Verify(const std::string& message) {
        size_t proofs = m_proof.m_items.size();
        size_t challenges = m_challenge.size();
        if (challenges == 0) {
            throw std::runtime_error("Nothing to verify");
        }
        if (proofs != challenges) {
            // TODO: fill out vector with empty proofs if too few and get incomplete result? What to do about too many?
            throw std::runtime_error(proofs < challenges ? "Proofs missing" : "Too many proofs");
        }

        auto inputs = m_challenge.back().InputsSet();
        Result aggres = Result::Valid;
        for (size_t i = 0; i < proofs; ++i) {
            auto& proof = m_proof.m_items.at(i);
            if (proof.m_scriptsig.size() == 0 && proof.m_witness.stack.size() == 0) {
                if (aggres == Result::Valid) aggres = Result::Incomplete;
                continue;
            }
            auto& challenge = m_challenge.at(i);
            uint256 sighash;
            CScript scriptPubKey;
            Result res = challenge.Prepare(message, inputs, sighash, scriptPubKey);
            if (res != Result::Valid) return res;
            // verify using consensus rules first
            res = VerifySingleProof(bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL, proof, sighash, scriptPubKey);
            if (res == Result::Error || res == Result::Invalid) return res;
            if (res == Result::Valid) {
                res = VerifySingleProof(STANDARD_SCRIPT_VERIFY_FLAGS, proof, sighash, scriptPubKey);
                if (res == Result::Invalid) res = Result::Inconclusive;
            }
            if (res < aggres) {
                aggres = res;
            }
        }
        return aggres;
    }
};

template<typename T> struct Workspace: public BaseWorkSpace<T> {};

template<>
struct Workspace<SignMessage>: public BaseWorkSpace<SignMessage> {
    void AppendDestinationChallenge(const CTxDestination& destination) {
        auto a = GetScriptForDestination(destination);
        m_challenge.emplace_back(a);
    }
    void AppendPrivKeyChallenge(const CKey& key, OutputType address_type = OutputType::BECH32) {
        auto d = GetDestinationForKey(key.GetPubKey(), address_type);
        auto a = GetScriptForDestination(d);
        privkeys[d] = key;
        m_challenge.emplace_back(a);
    }
    void GenerateSingleProof(std::unique_ptr<SigningProvider> sp, const uint256& sighash, const CScript& scriptPubKey) override {
        SimpleSignatureCreator sc(sighash);
        SignatureData sigdata;
        if (!ProduceSignature(*sp, sc, scriptPubKey, sigdata)) {
            throw signing_error("Failed to produce a signature");
        }
        m_proof.m_items.emplace_back(sigdata);
    }
    Result VerifySingleProof(unsigned int flags, const SignatureProof& proof, const uint256& sighash, const CScript& scriptPubKey) override {
        auto& scriptSig = proof.m_scriptsig;
        auto& witness = proof.m_witness;
        SimpleSignatureChecker sc(sighash);
        ScriptError serror;
        if (!VerifyScript(scriptSig, scriptPubKey, witness.stack.size() ? &witness : nullptr, flags, sc, &serror)) {
            // TODO: inconclusive check
            return Result::Invalid;
        }
        return Result::Valid;
    }
};

typedef Workspace<SignMessage> SignMessageWorkspace;

/**
 * Attempt to sign a message with the given destination.
 */
void SignMessageWithSigningProvider(std::unique_ptr<SigningProvider> sp, const std::string& message, const CTxDestination& destination, std::vector<uint8_t>& signature_out);

/**
 * Attempt to sign a message with the given private key and address format.
 */
void SignMessageWithPrivateKey(CKey& key, OutputType address_type, const std::string& message, std::vector<uint8_t>& signature_out);

/**
 * Determine if a signature is valid for the given message.
 */
Result VerifySignature(const std::string& message, const CTxDestination& destination, const std::vector<uint8_t>& signature);

} // namespace proof

#endif // BITCOIN_SCRIPT_PROOF_H
