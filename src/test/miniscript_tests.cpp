// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <addresstype.h>
#include <core_io.h>
#include <hash.h>
#include <pubkey.h>
#include <uint256.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <script/interpreter.h>
#include <script/miniscript.h>
#include <script/script_error.h>
#include <script/signingprovider.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace util::hex_literals;

namespace {

/** TestData groups various kinds of precomputed data necessary in this test. */
struct TestData {
    //! The only public keys used in this test.
    std::vector<CPubKey> pubkeys;
    //! A map from the public keys to their CKeyIDs (faster than hashing every time).
    std::map<CPubKey, CKeyID> pkhashes;
    std::map<CKeyID, CPubKey> pkmap;
    std::map<XOnlyPubKey, CKeyID> xonly_pkhashes;
    std::map<CPubKey, std::vector<unsigned char>> signatures;
    std::map<XOnlyPubKey, std::vector<unsigned char>> schnorr_signatures;

    // Various precomputed hashes
    std::vector<std::vector<unsigned char>> sha256;
    std::vector<std::vector<unsigned char>> ripemd160;
    std::vector<std::vector<unsigned char>> hash256;
    std::vector<std::vector<unsigned char>> hash160;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> sha256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> ripemd160_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash160_preimages;

    TestData()
    {
        // All our signatures sign (and are required to sign) this constant message.
        constexpr uint256 MESSAGE_HASH{"0000000000000000f5cd94e18b6fe77dd7aca9e35c2b0c9cbd86356c80a71065"};
        // We don't pass additional randomness when creating a schnorr signature.
        const auto EMPTY_AUX{uint256::ZERO};

        // We generate 255 public keys and 255 hashes of each type.
        for (int i = 1; i <= 255; ++i) {
            // This 32-byte array functions as both private key data and hash preimage (31 zero bytes plus any nonzero byte).
            unsigned char keydata[32] = {0};
            keydata[31] = i;

            // Compute CPubkey and CKeyID
            CKey key;
            key.Set(keydata, keydata + 32, true);
            CPubKey pubkey = key.GetPubKey();
            CKeyID keyid = pubkey.GetID();
            pubkeys.push_back(pubkey);
            pkhashes.emplace(pubkey, keyid);
            pkmap.emplace(keyid, pubkey);
            XOnlyPubKey xonly_pubkey{pubkey};
            uint160 xonly_hash{Hash160(xonly_pubkey)};
            xonly_pkhashes.emplace(xonly_pubkey, xonly_hash);
            pkmap.emplace(xonly_hash, pubkey);

            // Compute ECDSA signatures on MESSAGE_HASH with the private keys.
            std::vector<unsigned char> sig, schnorr_sig(64);
            BOOST_CHECK(key.Sign(MESSAGE_HASH, sig));
            sig.push_back(1); // sighash byte
            signatures.emplace(pubkey, sig);
            BOOST_CHECK(key.SignSchnorr(MESSAGE_HASH, schnorr_sig, nullptr, EMPTY_AUX));
            schnorr_sig.push_back(1); // Maximally sized Schnorr sigs have a sighash byte.
            schnorr_signatures.emplace(XOnlyPubKey{pubkey}, schnorr_sig);

            // Compute various hashes
            std::vector<unsigned char> hash;
            hash.resize(32);
            CSHA256().Write(keydata, 32).Finalize(hash.data());
            sha256.push_back(hash);
            sha256_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            CHash256().Write(keydata).Finalize(hash);
            hash256.push_back(hash);
            hash256_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            hash.resize(20);
            CRIPEMD160().Write(keydata, 32).Finalize(hash.data());
            ripemd160.push_back(hash);
            ripemd160_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
            CHash160().Write(keydata).Finalize(hash);
            hash160.push_back(hash);
            hash160_preimages[hash] = std::vector<unsigned char>(keydata, keydata + 32);
        }
    }
};

//! Global TestData object
std::unique_ptr<const TestData> g_testdata;

//! A classification of leaf conditions in miniscripts (excluding true/false).
enum class ChallengeType {
    SHA256,
    RIPEMD160,
    HASH256,
    HASH160,
    OLDER,
    AFTER,
    PK
};

/* With each leaf condition we associate a challenge number.
 * For hashes it's just the first 4 bytes of the hash. For pubkeys, it's the last 4 bytes.
 */
uint32_t ChallengeNumber(const CPubKey& pubkey) { return ReadLE32(pubkey.data() + 29); }
uint32_t ChallengeNumber(const std::vector<unsigned char>& hash) { return ReadLE32(hash.data()); }

//! A Challenge is a combination of type of leaf condition and its challenge number.
typedef std::pair<ChallengeType, uint32_t> Challenge;

/** A class encapulating conversion routing for CPubKey. */
struct KeyConverter {
    typedef CPubKey Key;

    const miniscript::MiniscriptContext m_script_ctx;

    constexpr KeyConverter(miniscript::MiniscriptContext ctx) noexcept : m_script_ctx{ctx} {}

    bool KeyCompare(const Key& a, const Key& b) const {
        return a < b;
    }

    //! Convert a public key to bytes.
    std::vector<unsigned char> ToPKBytes(const CPubKey& key) const {
        if (!miniscript::IsTapscript(m_script_ctx)) {
            return {key.begin(), key.end()};
        }
        const XOnlyPubKey xonly_pubkey{key};
        return {xonly_pubkey.begin(), xonly_pubkey.end()};
    }

    //! Convert a public key to its Hash160 bytes (precomputed).
    std::vector<unsigned char> ToPKHBytes(const CPubKey& key) const {
        if (!miniscript::IsTapscript(m_script_ctx)) {
            auto hash = g_testdata->pkhashes.at(key);
            return {hash.begin(), hash.end()};
        }
        const XOnlyPubKey xonly_key{key};
        auto hash = g_testdata->xonly_pkhashes.at(xonly_key);
        return {hash.begin(), hash.end()};
    }

    //! Parse a public key from a range of hex characters.
    template<typename I>
    std::optional<Key> FromString(I first, I last) const {
        auto bytes = ParseHex(std::string(first, last));
        Key key{bytes.begin(), bytes.end()};
        if (key.IsValid()) return key;
        return {};
    }

    template<typename I>
    std::optional<Key> FromPKBytes(I first, I last) const {
        if (!miniscript::IsTapscript(m_script_ctx)) {
            Key key{first, last};
            if (key.IsValid()) return key;
            return {};
        }
        if (last - first != 32) return {};
        XOnlyPubKey xonly_pubkey;
        std::copy(first, last, xonly_pubkey.begin());
        return xonly_pubkey.GetEvenCorrespondingCPubKey();
    }

    template<typename I>
    std::optional<Key> FromPKHBytes(I first, I last) const {
        assert(last - first == 20);
        CKeyID keyid;
        std::copy(first, last, keyid.begin());
        return g_testdata->pkmap.at(keyid);
    }

    std::optional<std::string> ToString(const Key& key) const {
        return HexStr(ToPKBytes(key));
    }

    miniscript::MiniscriptContext MsContext() const {
        return m_script_ctx;
    }
};

/** A class that encapsulates all signing/hash revealing operations. */
struct Satisfier : public KeyConverter {

    Satisfier(miniscript::MiniscriptContext ctx) noexcept : KeyConverter{ctx} {}

    //! Which keys/timelocks/hash preimages are available.
    std::set<Challenge> supported;

    //! Implement simplified CLTV logic: stack value must exactly match an entry in `supported`.
    bool CheckAfter(uint32_t value) const {
        return supported.count(Challenge(ChallengeType::AFTER, value));
    }

    //! Implement simplified CSV logic: stack value must exactly match an entry in `supported`.
    bool CheckOlder(uint32_t value) const {
        return supported.count(Challenge(ChallengeType::OLDER, value));
    }

    //! Produce a signature for the given key.
    miniscript::Availability Sign(const CPubKey& key, std::vector<unsigned char>& sig) const {
        if (supported.count(Challenge(ChallengeType::PK, ChallengeNumber(key)))) {
            if (!miniscript::IsTapscript(m_script_ctx)) {
                auto it = g_testdata->signatures.find(key);
                if (it == g_testdata->signatures.end()) return miniscript::Availability::NO;
                sig = it->second;
            } else {
                auto it = g_testdata->schnorr_signatures.find(XOnlyPubKey{key});
                if (it == g_testdata->schnorr_signatures.end()) return miniscript::Availability::NO;
                sig = it->second;
            }
            return miniscript::Availability::YES;
        }
        return miniscript::Availability::NO;
    }

    //! Helper function for the various hash based satisfactions.
    miniscript::Availability SatHash(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage, ChallengeType chtype) const {
        if (!supported.count(Challenge(chtype, ChallengeNumber(hash)))) return miniscript::Availability::NO;
        const auto& m =
            chtype == ChallengeType::SHA256 ? g_testdata->sha256_preimages :
            chtype == ChallengeType::HASH256 ? g_testdata->hash256_preimages :
            chtype == ChallengeType::RIPEMD160 ? g_testdata->ripemd160_preimages :
            g_testdata->hash160_preimages;
        auto it = m.find(hash);
        if (it == m.end()) return miniscript::Availability::NO;
        preimage = it->second;
        return miniscript::Availability::YES;
    }

    // Functions that produce the preimage for hashes of various types.
    miniscript::Availability SatSHA256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const { return SatHash(hash, preimage, ChallengeType::SHA256); }
    miniscript::Availability SatRIPEMD160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const { return SatHash(hash, preimage, ChallengeType::RIPEMD160); }
    miniscript::Availability SatHASH256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const { return SatHash(hash, preimage, ChallengeType::HASH256); }
    miniscript::Availability SatHASH160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const { return SatHash(hash, preimage, ChallengeType::HASH160); }
};

/** Mocking signature/timelock checker.
 *
 * It holds a pointer to a Satisfier object, to determine which timelocks are supposed to be available.
 */
class TestSignatureChecker : public BaseSignatureChecker {
    const Satisfier& ctx;

public:
    TestSignatureChecker(const Satisfier& in_ctx LIFETIMEBOUND) : ctx(in_ctx) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& pubkey, const CScript& scriptcode, SigVersion sigversion) const override {
        CPubKey pk(pubkey);
        if (!pk.IsValid()) return false;
        // Instead of actually running signature validation, check if the signature matches the precomputed one for this key.
        auto it = g_testdata->signatures.find(pk);
        if (it == g_testdata->signatures.end()) return false;
        return sig == it->second;
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion,
                               ScriptExecutionData&, ScriptError*) const override {
        XOnlyPubKey pk{pubkey};
        auto it = g_testdata->schnorr_signatures.find(pk);
        if (it == g_testdata->schnorr_signatures.end()) return false;
        return std::ranges::equal(sig, it->second);
    }

    bool CheckLockTime(const CScriptNum& locktime) const override {
        // Delegate to Satisfier.
        return ctx.CheckAfter(locktime.GetInt64());
    }

    bool CheckSequence(const CScriptNum& sequence) const override {
        // Delegate to Satisfier.
        return ctx.CheckOlder(sequence.GetInt64());
    }
};

using Fragment = miniscript::Fragment;
using NodeRef = miniscript::NodeRef<CPubKey>;
using miniscript::operator""_mst;
using Node = miniscript::Node<CPubKey>;

/** Compute all challenges (pubkeys, hashes, timelocks) that occur in a given Miniscript. */
std::set<Challenge> FindChallenges(const Node* root)
{
    std::set<Challenge> chal;

    for (std::vector stack{root}; !stack.empty();) {
        const auto* ref{stack.back()};
        stack.pop_back();

        for (const auto& key : ref->keys) {
            chal.emplace(ChallengeType::PK, ChallengeNumber(key));
        }
        switch (ref->fragment) {
        case Fragment::OLDER: chal.emplace(ChallengeType::OLDER, ref->k); break;
        case Fragment::AFTER: chal.emplace(ChallengeType::AFTER, ref->k); break;
        case Fragment::SHA256: chal.emplace(ChallengeType::SHA256, ChallengeNumber(ref->data)); break;
        case Fragment::RIPEMD160: chal.emplace(ChallengeType::RIPEMD160, ChallengeNumber(ref->data)); break;
        case Fragment::HASH256: chal.emplace(ChallengeType::HASH256, ChallengeNumber(ref->data)); break;
        case Fragment::HASH160: chal.emplace(ChallengeType::HASH160, ChallengeNumber(ref->data)); break;
        default: break;
        }
        for (const auto& sub : ref->subs) {
            stack.push_back(sub.get());
        }
    }
    return chal;
}

//! The spk for this script under the given context. If it's a Taproot output also record the spend data.
CScript ScriptPubKey(miniscript::MiniscriptContext ctx, const CScript& script, TaprootBuilder& builder)
{
    if (!miniscript::IsTapscript(ctx)) return CScript() << OP_0 << WitnessV0ScriptHash(script);

    // For Taproot outputs we always use a tree with a single script and a dummy internal key.
    builder.Add(0, script, TAPROOT_LEAF_TAPSCRIPT);
    builder.Finalize(XOnlyPubKey::NUMS_H);
    return GetScriptForDestination(builder.GetOutput());
}

//! Fill the witness with the data additional to the script satisfaction.
void SatisfactionToWitness(miniscript::MiniscriptContext ctx, CScriptWitness& witness, const CScript& script, TaprootBuilder& builder) {
    // For P2WSH, it's only the witness script.
    witness.stack.emplace_back(script.begin(), script.end());
    if (!miniscript::IsTapscript(ctx)) return;
    // For Tapscript we also need the control block.
    witness.stack.push_back(*builder.GetSpendData().scripts.begin()->second.begin());
}

struct MiniScriptTest : BasicTestingSetup {
/** Run random satisfaction tests. */
void TestSatisfy(const KeyConverter& converter, const std::string& testcase, const NodeRef& node) {
    auto script = node->ToScript(converter);
    const auto challenges{FindChallenges(node.get())}; // Find all challenges in the generated miniscript.
    std::vector<Challenge> challist(challenges.begin(), challenges.end());
    for (int iter = 0; iter < 3; ++iter) {
        std::shuffle(challist.begin(), challist.end(), m_rng);
        Satisfier satisfier(converter.MsContext());
        TestSignatureChecker checker(satisfier);
        bool prev_mal_success = false, prev_nonmal_success = false;
        // Go over all challenges involved in this miniscript in random order.
        for (int add = -1; add < (int)challist.size(); ++add) {
            if (add >= 0) satisfier.supported.insert(challist[add]); // The first iteration does not add anything

            // Get the ScriptPubKey for this script, filling spend data if it's Taproot.
            TaprootBuilder builder;
            const CScript script_pubkey{ScriptPubKey(converter.MsContext(), script, builder)};

            // Run malleable satisfaction algorithm.
            CScriptWitness witness_mal;
            const bool mal_success = node->Satisfy(satisfier, witness_mal.stack, false) == miniscript::Availability::YES;
            SatisfactionToWitness(converter.MsContext(), witness_mal, script, builder);

            // Run non-malleable satisfaction algorithm.
            CScriptWitness witness_nonmal;
            const bool nonmal_success = node->Satisfy(satisfier, witness_nonmal.stack, true) == miniscript::Availability::YES;
            // Compute witness size (excluding script push, control block, and witness count encoding).
            const size_t wit_size = GetSerializeSize(witness_nonmal.stack) - GetSizeOfCompactSize(witness_nonmal.stack.size());
            SatisfactionToWitness(converter.MsContext(), witness_nonmal, script, builder);

            if (nonmal_success) {
                // Non-malleable satisfactions are bounded by the satisfaction size plus:
                // - For P2WSH spends, the witness script
                // - For Tapscript spends, both the witness script and the control block
                const size_t max_stack_size{*node->GetStackSize() + 1 + miniscript::IsTapscript(converter.MsContext())};
                BOOST_CHECK(witness_nonmal.stack.size() <= max_stack_size);
                // If a non-malleable satisfaction exists, the malleable one must also exist, and be identical to it.
                BOOST_CHECK(mal_success);
                BOOST_CHECK(witness_nonmal.stack == witness_mal.stack);
                assert(wit_size <= *node->GetWitnessSize());

                // Test non-malleable satisfaction.
                ScriptError serror;
                bool res = VerifyScript(CScript(), script_pubkey, &witness_nonmal, STANDARD_SCRIPT_VERIFY_FLAGS, checker, &serror);
                // Non-malleable satisfactions are guaranteed to be valid if ValidSatisfactions().
                if (node->ValidSatisfactions()) BOOST_CHECK(res);
                // More detailed: non-malleable satisfactions must be valid, or could fail with ops count error (if CheckOpsLimit failed),
                // or with a stack size error (if CheckStackSize check fails).
                BOOST_CHECK(res ||
                            (!node->CheckOpsLimit() && serror == ScriptError::SCRIPT_ERR_OP_COUNT) ||
                            (!node->CheckStackSize() && serror == ScriptError::SCRIPT_ERR_STACK_SIZE));
            }

            if (mal_success && (!nonmal_success || witness_mal.stack != witness_nonmal.stack)) {
                // Test malleable satisfaction only if it's different from the non-malleable one.
                ScriptError serror;
                bool res = VerifyScript(CScript(), script_pubkey, &witness_mal, STANDARD_SCRIPT_VERIFY_FLAGS, checker, &serror);
                // Malleable satisfactions are not guaranteed to be valid under any conditions, but they can only
                // fail due to stack or ops limits.
                BOOST_CHECK(res || serror == ScriptError::SCRIPT_ERR_OP_COUNT || serror == ScriptError::SCRIPT_ERR_STACK_SIZE);
            }

            if (node->IsSane()) {
                // For sane nodes, the two algorithms behave identically.
                BOOST_CHECK_EQUAL(mal_success, nonmal_success);
            }

            // Adding more satisfied conditions can never remove our ability to produce a satisfaction.
            BOOST_CHECK(mal_success >= prev_mal_success);
            // For nonmalleable solutions this is only true if the added condition is PK;
            // for other conditions, adding one may make an valid satisfaction become malleable. If the script
            // is sane, this cannot happen however.
            if (node->IsSane() || add < 0 || challist[add].first == ChallengeType::PK) {
                BOOST_CHECK(nonmal_success >= prev_nonmal_success);
            }
            // Remember results for the next added challenge.
            prev_mal_success = mal_success;
            prev_nonmal_success = nonmal_success;
        }

        bool satisfiable = node->IsSatisfiable([](const Node&) { return true; });
        // If the miniscript was satisfiable at all, a satisfaction must be found after all conditions are added.
        BOOST_CHECK_EQUAL(prev_mal_success, satisfiable);
        // If the miniscript is sane and satisfiable, a nonmalleable satisfaction must eventually be found.
        if (node->IsSane()) BOOST_CHECK_EQUAL(prev_nonmal_success, satisfiable);
    }
}

enum TestMode : int {
    //! Invalid under any context
    TESTMODE_INVALID = 0,
    //! Valid under any context unless overridden
    TESTMODE_VALID = 1,
    TESTMODE_NONMAL = 2,
    TESTMODE_NEEDSIG = 4,
    TESTMODE_TIMELOCKMIX = 8,
    //! Invalid only under P2WSH context
    TESTMODE_P2WSH_INVALID = 16,
    //! Invalid only under Tapscript context
    TESTMODE_TAPSCRIPT_INVALID = 32,
};

void Test(const std::string& ms, const std::string& hexscript, int mode, const KeyConverter& converter,
          int opslimit = -1, int stacklimit = -1, std::optional<uint32_t> max_wit_size = std::nullopt,
          std::optional<uint32_t> stack_exec = {})
{
    auto node = miniscript::FromString(ms, converter);
    const bool is_tapscript{miniscript::IsTapscript(converter.MsContext())};
    if (mode == TESTMODE_INVALID || ((mode & TESTMODE_P2WSH_INVALID) && !is_tapscript) || ((mode & TESTMODE_TAPSCRIPT_INVALID) && is_tapscript)) {
        BOOST_CHECK_MESSAGE(!node || !node->IsValid(), "Unexpectedly valid: " + ms);
    } else {
        BOOST_CHECK_MESSAGE(node, "Unparseable: " + ms);
        BOOST_CHECK_MESSAGE(node->IsValid(), "Invalid: " + ms);
        BOOST_CHECK_MESSAGE(node->IsValidTopLevel(), "Invalid top level: " + ms);
        auto computed_script = node->ToScript(converter);
        BOOST_CHECK_MESSAGE(node->ScriptSize() == computed_script.size(), "Script size mismatch: " + ms);
        if (hexscript != "?") BOOST_CHECK_MESSAGE(HexStr(computed_script) == hexscript, "Script mismatch: " + ms + " (" + HexStr(computed_script) + " vs " + hexscript + ")");
        BOOST_CHECK_MESSAGE(node->IsNonMalleable() == !!(mode & TESTMODE_NONMAL), "Malleability mismatch: " + ms);
        BOOST_CHECK_MESSAGE(node->NeedsSignature() == !!(mode & TESTMODE_NEEDSIG), "Signature necessity mismatch: " + ms);
        BOOST_CHECK_MESSAGE((node->GetType() << "k"_mst) == !(mode & TESTMODE_TIMELOCKMIX), "Timelock mix mismatch: " + ms);
        auto inferred_miniscript = miniscript::FromScript(computed_script, converter);
        BOOST_CHECK_MESSAGE(inferred_miniscript, "Cannot infer miniscript from script: " + ms);
        BOOST_CHECK_MESSAGE(inferred_miniscript->ToScript(converter) == computed_script, "Roundtrip failure: miniscript->script != miniscript->script->miniscript->script: " + ms);
        if (opslimit != -1) BOOST_CHECK_MESSAGE((int)*node->GetOps() == opslimit, "Ops limit mismatch: " << ms << " (" << *node->GetOps() << " vs " << opslimit << ")");
        if (stacklimit != -1) BOOST_CHECK_MESSAGE((int)*node->GetStackSize() == stacklimit, "Stack limit mismatch: " << ms << " (" << *node->GetStackSize() << " vs " << stacklimit << ")");
        if (max_wit_size) BOOST_CHECK_MESSAGE(*node->GetWitnessSize() == *max_wit_size, "Witness size limit mismatch: " << ms << " (" << *node->GetWitnessSize() << " vs " << *max_wit_size << ")");
        if (stack_exec) BOOST_CHECK_MESSAGE(*node->GetExecStackSize() == *stack_exec, "Stack execution limit mismatch: " << ms << " (" << *node->GetExecStackSize() << " vs " << *stack_exec << ")");
        TestSatisfy(converter, ms, node);
    }
}

void Test(const std::string& ms, const std::string& hexscript, const std::string& hextapscript, int mode,
          int opslimit, int stacklimit, std::optional<uint32_t> max_wit_size,
          std::optional<uint32_t> max_tap_wit_size,
          std::optional<uint32_t> stack_exec)
{
    KeyConverter wsh_converter(miniscript::MiniscriptContext::P2WSH);
    Test(ms, hexscript, mode, wsh_converter, opslimit, stacklimit, max_wit_size, stack_exec);
    KeyConverter tap_converter(miniscript::MiniscriptContext::TAPSCRIPT);
    Test(ms, hextapscript == "=" ? hexscript : hextapscript, mode, tap_converter, opslimit, stacklimit, max_tap_wit_size, stack_exec);
}

void Test(const std::string& ms, const std::string& hexscript, const std::string& hextapscript, int mode)
{
    Test(ms, hexscript, hextapscript, mode,
         /*opslimit=*/-1, /*stacklimit=*/-1,
         /*max_wit_size=*/std::nullopt, /*max_tap_wit_size=*/std::nullopt, /*stack_exec=*/std::nullopt);
}
}; // struct MiniScriptTest

} // namespace

BOOST_FIXTURE_TEST_SUITE(miniscript_tests, MiniScriptTest)

BOOST_AUTO_TEST_CASE(fixed_tests)
{
    g_testdata.reset(new TestData());

    // Validity rules
    Test("l:older(1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // older(1): valid
    Test("l:older(0)", "?", "?", TESTMODE_INVALID); // older(0): k must be at least 1
    Test("l:older(2147483647)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // older(2147483647): valid
    Test("l:older(2147483648)", "?", "?", TESTMODE_INVALID); // older(2147483648): k must be below 2^31
    Test("u:after(1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // after(1): valid
    Test("u:after(0)", "?", "?", TESTMODE_INVALID); // after(0): k must be at least 1
    Test("u:after(2147483647)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // after(2147483647): valid
    Test("u:after(2147483648)", "?", "?", TESTMODE_INVALID); // after(2147483648): k must be below 2^31
    Test("andor(0,1,1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // andor(Bdu,B,B): valid
    Test("andor(a:0,1,1)", "?", "?", TESTMODE_INVALID); // andor(Wdu,B,B): X must be B
    Test("andor(0,a:1,a:1)", "?", "?", TESTMODE_INVALID); // andor(Bdu,W,W): Y and Z must be B/V/K
    Test("andor(1,1,1)", "?", "?", TESTMODE_INVALID); // andor(Bu,B,B): X must be d
    Test("andor(n:or_i(0,after(1)),1,1)", "?", "?", TESTMODE_VALID); // andor(Bdu,B,B): valid
    Test("andor(or_i(0,after(1)),1,1)", "?", "?", TESTMODE_INVALID); // andor(Bd,B,B): X must be u
    Test("c:andor(0,pk_k(03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7),pk_k(036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00))", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // andor(Bdu,K,K): valid
    Test("t:andor(0,v:1,v:1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // andor(Bdu,V,V): valid
    Test("and_v(v:1,1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // and_v(V,B): valid
    Test("t:and_v(v:1,v:1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // and_v(V,V): valid
    Test("c:and_v(v:1,pk_k(036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00))", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // and_v(V,K): valid
    Test("and_v(1,1)", "?", "?", TESTMODE_INVALID); // and_v(B,B): X must be V
    Test("and_v(pk_k(02352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d5),1)", "?", "?", TESTMODE_INVALID); // and_v(K,B): X must be V
    Test("and_v(v:1,a:1)", "?", "?", TESTMODE_INVALID); // and_v(K,W): Y must be B/V/K
    Test("and_b(1,a:1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // and_b(B,W): valid
    Test("and_b(1,1)", "?", "?", TESTMODE_INVALID); // and_b(B,B): Y must W
    Test("and_b(v:1,a:1)", "?", "?", TESTMODE_INVALID); // and_b(V,W): X must be B
    Test("and_b(a:1,a:1)", "?", "?", TESTMODE_INVALID); // and_b(W,W): X must be B
    Test("and_b(pk_k(025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc),a:1)", "?", "?", TESTMODE_INVALID); // and_b(K,W): X must be B
    Test("or_b(0,a:0)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // or_b(Bd,Wd): valid
    Test("or_b(1,a:0)", "?", "?", TESTMODE_INVALID); // or_b(B,Wd): X must be d
    Test("or_b(0,a:1)", "?", "?", TESTMODE_INVALID); // or_b(Bd,W): Y must be d
    Test("or_b(0,0)", "?", "?", TESTMODE_INVALID); // or_b(Bd,Bd): Y must W
    Test("or_b(v:0,a:0)", "?", "?", TESTMODE_INVALID); // or_b(V,Wd): X must be B
    Test("or_b(a:0,a:0)", "?", "?", TESTMODE_INVALID); // or_b(Wd,Wd): X must be B
    Test("or_b(pk_k(025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc),a:0)", "?", "?", TESTMODE_INVALID); // or_b(Kd,Wd): X must be B
    Test("t:or_c(0,v:1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // or_c(Bdu,V): valid
    Test("t:or_c(a:0,v:1)", "?", "?", TESTMODE_INVALID); // or_c(Wdu,V): X must be B
    Test("t:or_c(1,v:1)", "?", "?", TESTMODE_INVALID); // or_c(Bu,V): X must be d
    Test("t:or_c(n:or_i(0,after(1)),v:1)", "?", "?", TESTMODE_VALID); // or_c(Bdu,V): valid
    Test("t:or_c(or_i(0,after(1)),v:1)", "?", "?", TESTMODE_INVALID); // or_c(Bd,V): X must be u
    Test("t:or_c(0,1)", "?", "?", TESTMODE_INVALID); // or_c(Bdu,B): Y must be V
    Test("or_d(0,1)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // or_d(Bdu,B): valid
    Test("or_d(a:0,1)", "?", "?", TESTMODE_INVALID); // or_d(Wdu,B): X must be B
    Test("or_d(1,1)", "?", "?", TESTMODE_INVALID); // or_d(Bu,B): X must be d
    Test("or_d(n:or_i(0,after(1)),1)", "?", "?", TESTMODE_VALID); // or_d(Bdu,B): valid
    Test("or_d(or_i(0,after(1)),1)", "?", "?", TESTMODE_INVALID); // or_d(Bd,B): X must be u
    Test("or_d(0,v:1)", "?", "?", TESTMODE_INVALID); // or_d(Bdu,V): Y must be B
    Test("or_i(1,1)", "?", "?", TESTMODE_VALID); // or_i(B,B): valid
    Test("t:or_i(v:1,v:1)", "?", "?", TESTMODE_VALID); // or_i(V,V): valid
    Test("c:or_i(pk_k(03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7),pk_k(036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00))", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // or_i(K,K): valid
    Test("or_i(a:1,a:1)", "?", "?", TESTMODE_INVALID); // or_i(W,W): X and Y must be B/V/K
    Test("or_b(l:after(100),al:after(1000000000))", "?", "?", TESTMODE_VALID); // or_b(timelock, heighlock) valid
    Test("and_b(after(100),a:after(1000000000))", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_TIMELOCKMIX); // and_b(timelock, heighlock) invalid
    Test("pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65)", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac", "20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // alias to c:pk_k
    Test("pkh(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65)", "76a914fcd35ddacad9f2d5be5e464639441c6065e6955d88ac", "76a914fd1690c37fa3b0f04395ddc9415b220ab1ccc59588ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG); // alias to c:pk_h

    // Randomly generated test set that covers the majority of type and node type combinations
    Test("lltvln:after(1231488000)", "6300676300676300670400046749b1926869516868", "=", TESTMODE_VALID | TESTMODE_NONMAL, 12, 3, 3, 3, 3);
    Test("uuj:and_v(v:multi(2,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a,025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc),after(1231488000))", "6363829263522103d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a21025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc52af0400046749b168670068670068", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 14, 5, 2 + 2 + 1 + 2 * 73, 0, 7);
    Test("or_b(un:multi(2,03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729,024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),al:older(16))", "63522103daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee872921024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c9752ae926700686b63006760b2686c9b", "?", TESTMODE_VALID | TESTMODE_TAPSCRIPT_INVALID, 14, 5, 2 + 1 + 2 * 73 + 2, 0, 8);
    Test("j:and_v(vdv:after(1567547623),older(2016))", "829263766304e7e06e5db169686902e007b268", "=", TESTMODE_VALID | TESTMODE_NONMAL, 11, 1, 2, 2, 2);
    Test("t:and_v(vu:hash256(131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b),v:sha256(ec4916dd28fc4c10d78e287ca5d9cc51ee1ae73cbfde08c6b37324cbfaac8bc5))", "6382012088aa20131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b876700686982012088a820ec4916dd28fc4c10d78e287ca5d9cc51ee1ae73cbfde08c6b37324cbfaac8bc58851", "6382012088aa20131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b876700686982012088a820ec4916dd28fc4c10d78e287ca5d9cc51ee1ae73cbfde08c6b37324cbfaac8bc58851", TESTMODE_VALID | TESTMODE_NONMAL, 12, 3, 2 + 33 + 33, 2 + 33 + 33, 4);
    Test("t:andor(multi(3,02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e,03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556,02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13),v:older(4194305),v:sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2))", "532102d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a14602975562102e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd1353ae6482012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2886703010040b2696851", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_TAPSCRIPT_INVALID, 13, 5, 1 + 3 * 73, 0, 10);
    Test("or_d(multi(1,02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9),or_b(multi(3,022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01,032fa2104d6b38d11b0230010559879124e42ab8dfeff5ff29dc9cdadd4ecacc3f,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a),su:after(500000)))", "512102f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f951ae73645321022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a0121032fa2104d6b38d11b0230010559879124e42ab8dfeff5ff29dc9cdadd4ecacc3f2103d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a53ae7c630320a107b16700689b68", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_TAPSCRIPT_INVALID, 15, 7, 2 + 1 + 3 * 73 + 1, 0, 10);
    Test("or_d(sha256(38df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b6),and_n(un:after(499999999),older(4194305)))", "82012088a82038df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b68773646304ff64cd1db19267006864006703010040b26868", "82012088a82038df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b68773646304ff64cd1db19267006864006703010040b26868", TESTMODE_VALID, 16, 1, 33, 33, 3);
    Test("and_v(or_i(v:multi(2,02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb),v:multi(2,03e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0a,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)),sha256(d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68))", "63522102c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee52103774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb52af67522103e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0a21025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc52af6882012088a820d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c6887", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 11, 5, 2 + 1 + 2 * 73 + 33, 0, 8);
    Test("j:and_b(multi(2,0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798,024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),s:or_i(older(1),older(4252898)))", "82926352210279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f8179821024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c9752ae7c6351b26703e2e440b2689a68", "?", TESTMODE_VALID | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 14, 4, 1 + 2 * 73 + 2, 0, 8);
    Test("and_b(older(16),s:or_d(sha256(e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f),n:after(1567547623)))", "60b27c82012088a820e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f87736404e7e06e5db192689a", "=", TESTMODE_VALID, 12, 1, 33, 33, 4);
    Test("j:and_v(v:hash160(20195b5a3d650c17f0f29f91c33f8f6335193d07),or_d(sha256(96de8fc8c256fa1e1556d41af431cace7dca68707c78dd88c3acab8b17164c47),older(16)))", "82926382012088a91420195b5a3d650c17f0f29f91c33f8f6335193d078882012088a82096de8fc8c256fa1e1556d41af431cace7dca68707c78dd88c3acab8b17164c4787736460b26868", "=", TESTMODE_VALID, 16, 2, 33 + 33, 33 + 33, 4);
    Test("and_b(hash256(32ba476771d01e37807990ead8719f08af494723de1d228f2c2c07cc0aa40bac),a:and_b(hash256(131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b),a:older(1)))", "82012088aa2032ba476771d01e37807990ead8719f08af494723de1d228f2c2c07cc0aa40bac876b82012088aa20131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b876b51b26c9a6c9a", "=", TESTMODE_VALID | TESTMODE_NONMAL, 15, 2, 33 + 33, 33 + 33, 4);
    Test("thresh(2,multi(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00),a:multi(1,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00),ac:pk_k(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01))", "522103a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c721036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a0052ae6b5121036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a0051ae6c936b21022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01ac6c935287", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 13, 6, 1 + 2 * 73 + 1 + 73 + 1, 0, 10);
    Test("and_n(sha256(d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68),t:or_i(v:older(4252898),v:older(144)))", "82012088a820d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68876400676303e2e440b26967029000b269685168", "=", TESTMODE_VALID, 14, 2, 33 + 2, 33 + 2, 4);
    Test("or_d(nd:and_v(v:older(4252898),v:older(4252898)),sha256(38df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b6))", "766303e2e440b26903e2e440b2696892736482012088a82038df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b68768", "=", TESTMODE_VALID, 15, 2, 1 + 33, 1 + 33, 3);
    Test("c:and_v(or_c(sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2),v:multi(1,02c44d12c7065d812e8acf28d7cbb19f9011ecd9e9fdf281b0e6a3b5e87d22e7db)),pk_k(03acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe))", "82012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed28764512102c44d12c7065d812e8acf28d7cbb19f9011ecd9e9fdf281b0e6a3b5e87d22e7db51af682103acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbeac", "?", TESTMODE_VALID | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 8, 2, 33 + 73, 0, 4);
    Test("c:and_v(or_c(multi(2,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00,02352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d5),v:ripemd160(1b0f3c404d12075c68c938f9f60ebea4f74941a0)),pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))", "5221036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a002102352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d552ae6482012088a6141b0f3c404d12075c68c938f9f60ebea4f74941a088682103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_TAPSCRIPT_INVALID, 10, 5, 1 + 2 * 73 + 73, 0, 9);
    Test("and_v(andor(hash256(8a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b25),v:hash256(939894f70e6c3a25da75da0cc2071b4076d9b006563cf635986ada2e93c0d735),v:older(50000)),after(499999999))", "82012088aa208a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b2587640350c300b2696782012088aa20939894f70e6c3a25da75da0cc2071b4076d9b006563cf635986ada2e93c0d735886804ff64cd1db1", "=", TESTMODE_VALID, 14, 2, 33 + 33, 33 + 33, 4);
    Test("andor(hash256(5f8d30e655a7ba0d7596bb3ddfb1d2d20390d23b1845000e1e118b3be1b3f040),j:and_v(v:hash160(3a2bff0da9d96868e66abc4427bea4691cf61ccd),older(4194305)),ripemd160(44d90e2d3714c8663b632fcf0f9d5f22192cc4c8))", "82012088aa205f8d30e655a7ba0d7596bb3ddfb1d2d20390d23b1845000e1e118b3be1b3f040876482012088a61444d90e2d3714c8663b632fcf0f9d5f22192cc4c8876782926382012088a9143a2bff0da9d96868e66abc4427bea4691cf61ccd8803010040b26868", "=", TESTMODE_VALID, 20, 2, 33 + 33, 33 + 33, 4);
    Test("or_i(c:and_v(v:after(500000),pk_k(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)),sha256(d9147961436944f43cd99d28b2bbddbf452ef872b30c8279e255e7daafc7f946))", "630320a107b1692102c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ac6782012088a820d9147961436944f43cd99d28b2bbddbf452ef872b30c8279e255e7daafc7f9468768", "630320a107b16920c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ac6782012088a820d9147961436944f43cd99d28b2bbddbf452ef872b30c8279e255e7daafc7f9468768", TESTMODE_VALID | TESTMODE_NONMAL, 10, 2, 2 + 73, 2 + 66, 3);
    Test("thresh(2,c:pk_h(025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc),s:sha256(e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f),a:hash160(dd69735817e0e3f6f826a9238dc2e291184f0131))", "76a9145dedfbf9ea599dd4e3ca6a80b333c472fd0b3f6988ac7c82012088a820e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f87936b82012088a914dd69735817e0e3f6f826a9238dc2e291184f0131876c935287", "76a9141a7ac36cfa8431ab2395d701b0050045ae4a37d188ac7c82012088a820e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f87936b82012088a914dd69735817e0e3f6f826a9238dc2e291184f0131876c935287", TESTMODE_VALID, 18, 4, 1 + 34 + 33 + 33, 1 + 33 + 33 + 33, 6);
    Test("and_n(sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2),uc:and_v(v:older(144),pk_k(03fe72c435413d33d48ac09c9161ba8b09683215439d62b7940502bda8b202e6ce)))", "82012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed28764006763029000b2692103fe72c435413d33d48ac09c9161ba8b09683215439d62b7940502bda8b202e6ceac67006868", "82012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed28764006763029000b26920fe72c435413d33d48ac09c9161ba8b09683215439d62b7940502bda8b202e6ceac67006868", TESTMODE_VALID | TESTMODE_NEEDSIG, 13, 3, 33 + 2 + 73, 33 + 2 + 66, 5);
    Test("and_n(c:pk_k(03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729),and_b(l:older(4252898),a:older(16)))", "2103daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729ac64006763006703e2e440b2686b60b26c9a68", "20daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729ac64006763006703e2e440b2686b60b26c9a68", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_TIMELOCKMIX, 12, 2, 73 + 1, 66 + 1, 3);
    Test("c:or_i(and_v(v:older(16),pk_h(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e)),pk_h(026a245bf6dc698504c89a20cfded60853152b695336c28063b61c65cbd269e6b4))", "6360b26976a9149fc5dbe5efdce10374a4dd4053c93af540211718886776a9142fbd32c8dd59ee7c17e66cb6ebea7e9846c3040f8868ac", "6360b26976a9144d4421361c3289bdad06441ffaee8be8e786f1ad886776a91460d4a7bcbd08f58e58bd208d1069837d7adb16ae8868ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG, 12, 3, 2 + 34 + 73, 2 + 33 + 66, 4);
    Test("or_d(c:pk_h(02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13),andor(c:pk_k(024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),older(2016),after(1567547623)))", "76a914c42e7ef92fdb603af844d064faad95db9bcdfd3d88ac736421024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97ac6404e7e06e5db16702e007b26868", "76a91421ab1a140d0d305b8ff62bdb887d9fef82c9899e88ac7364204ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97ac6404e7e06e5db16702e007b26868", TESTMODE_VALID | TESTMODE_NONMAL, 13, 3, 1 + 34 + 73, 1 + 33 + 66, 5);
    Test("c:andor(ripemd160(6ad07d21fd5dfc646f0b30577045ce201616b9ba),pk_h(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e),and_v(v:hash256(8a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b25),pk_h(03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a)))", "82012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba876482012088aa208a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b258876a914dd100be7d9aea5721158ebde6d6a1fd8fff93bb1886776a9149fc5dbe5efdce10374a4dd4053c93af5402117188868ac", "82012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba876482012088aa208a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b258876a914a63d1e4d2ed109246c600ec8c19cce546b65b1cc886776a9144d4421361c3289bdad06441ffaee8be8e786f1ad8868ac", TESTMODE_VALID | TESTMODE_NEEDSIG, 18, 3, 33 + 34 + 73, 33 + 33 + 66, 5);
    Test("c:andor(u:ripemd160(6ad07d21fd5dfc646f0b30577045ce201616b9ba),pk_h(03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729),or_i(pk_h(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01),pk_h(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)))", "6382012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba87670068646376a9149652d86bedf43ad264362e6e6eba6eb764508127886776a914751e76e8199196d454941c45d1b3a323f1433bd688686776a91420d637c1a6404d2227f3561fdbaff5a680dba6488868ac", "6382012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba87670068646376a914ceedcb44b38bdbcb614d872223964fd3dca8a434886776a914f678d9b79045452c8c64e9309d0f0046056e26c588686776a914a2a75e1819afa208f6c89ae0da43021116dfcb0c8868ac", TESTMODE_VALID | TESTMODE_NEEDSIG, 23, 4, 2 + 33 + 34 + 73, 2 + 33 + 33 + 66, 5);
    Test("c:or_i(andor(c:pk_h(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),pk_h(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01),pk_h(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)),pk_k(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e))", "6376a914fcd35ddacad9f2d5be5e464639441c6065e6955d88ac6476a91406afd46bcdfd22ef94ac122aa11f241244a37ecc886776a9149652d86bedf43ad264362e6e6eba6eb7645081278868672102d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e68ac", "6376a914fd1690c37fa3b0f04395ddc9415b220ab1ccc59588ac6476a9149b652a14674a506079f574d20ca7daef6f9a66bb886776a914ceedcb44b38bdbcb614d872223964fd3dca8a43488686720d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e68ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG, 17, 5, 2 + 34 + 73 + 34 + 73, 2 + 33 + 66 + 33 + 66, 6);
    Test("thresh(1,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),altv:after(1000000000),altv:after(100))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b6300670400ca9a3bb16951686c936b6300670164b16951686c935187", "20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b6300670400ca9a3bb16951686c936b6300670164b16951686c935187", TESTMODE_VALID, 18, 3, 73 + 2 + 2, 66 + 2 + 2, 4);
    Test("thresh(2,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),ac:pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556),altv:after(1000000000),altv:after(100))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac6c936b6300670400ca9a3bb16951686c936b6300670164b16951686c935287", "20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b20fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac6c936b6300670400ca9a3bb16951686c936b6300670164b16951686c935287", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_TIMELOCKMIX, 22, 4, 73 + 73 + 2 + 2, 66 + 66 + 2 + 2, 5);

    // Additional Tapscript-related tests
    // Edge cases when parsing multi_a from script:
    //  - no pubkey at all
    //  - no pubkey before a CHECKSIGADD
    //  - no pubkey before the CHECKSIG
    constexpr KeyConverter tap_converter{miniscript::MiniscriptContext::TAPSCRIPT};
    constexpr KeyConverter wsh_converter{miniscript::MiniscriptContext::P2WSH};
    const auto no_pubkey{"ac519c"_hex_u8};
    BOOST_CHECK(miniscript::FromScript({no_pubkey.begin(), no_pubkey.end()}, tap_converter) == nullptr);
    const auto incomplete_multi_a{"ba20c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ba519c"_hex_u8};
    BOOST_CHECK(miniscript::FromScript({incomplete_multi_a.begin(), incomplete_multi_a.end()}, tap_converter) == nullptr);
    const auto incomplete_multi_a_2{"ac2079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798ac20c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ba519c"_hex_u8};
    BOOST_CHECK(miniscript::FromScript({incomplete_multi_a_2.begin(), incomplete_multi_a_2.end()}, tap_converter) == nullptr);
    // Can use multi_a under Tapscript but not P2WSH.
    Test("and_v(v:multi_a(2,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a,025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc),after(1231488000))", "?", "20d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85aac205601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7ccba529d0400046749b1", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, 4, 2, {}, {}, 3);
    // Can use more than 20 keys in a multi_a.
    std::string ms_str_multi_a{"multi_a(1,"};
    for (size_t i = 0; i < 21; ++i) {
        ms_str_multi_a += HexStr(g_testdata->pubkeys[i]);
        if (i < 20) ms_str_multi_a += ",";
    }
    ms_str_multi_a += ")";
    Test(ms_str_multi_a, "?", "2079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798ac20c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ba20f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9ba20e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13ba202f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4ba20fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ba205cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bcba202f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01ba20acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbeba20a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7ba20774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cbba20d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85aba20f28773c2d975288bc7d1d205c3748651b075fbc6610e58cddeeddf8f19405aa8ba20499fdf9e895e719cfd64e67f07d38e3226aa7b63678949e6e49b241a60e823e4ba20d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080eba20e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0aba20defdea4cdb677750a420fee807eacf21eb9898ae79b9768766e4faa04a2d4a34ba205601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7ccba202b4ea0a797a443d293ef5cff444f4979f06acfebd7e86d277475656138385b6cba204ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97ba20352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d5ba519c", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, 22, 21, {}, {}, 22);
    // Since 'd:' is 'u' we can use it directly inside a thresh. But we can't under P2WSH.
    Test("thresh(2,dv:older(42),s:pk(025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc),s:pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65))", "?", "7663012ab269687c205cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bcac937c20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac935287", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, 12, 3, {}, {}, 4);
    // We can have a script that has more than 201 ops (n = 99), that needs a stack size > 100 (n = 110), or has a
    // script that is larger than 3600 bytes (n = 200). All that can't be under P2WSH.
    for (const auto pk_count: {99, 110, 200}) {
        std::string ms_str_large;
        for (auto i = 0; i < pk_count - 1; ++i) {
            ms_str_large += "and_b(pk(" + HexStr(g_testdata->pubkeys[i]) + "),a:";
        }
        ms_str_large += "pk(" + HexStr(g_testdata->pubkeys[pk_count - 1]) + ")";
        ms_str_large.insert(ms_str_large.end(), pk_count - 1, ')');
        Test(ms_str_large, "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, pk_count + (pk_count - 1) * 3, pk_count, {}, {}, pk_count + 1);
    }
    // We can have a script that reaches a stack size of 1000 during execution.
    std::string ms_stack_limit;
    auto count{998};
    for (auto i = 0; i < count; ++i) {
        ms_stack_limit += "and_b(older(1),a:";
    }
    ms_stack_limit += "pk(" + HexStr(g_testdata->pubkeys[0]) + ")";
    ms_stack_limit.insert(ms_stack_limit.end(), count, ')');
    const auto ms_stack_ok{miniscript::FromString(ms_stack_limit, tap_converter)};
    BOOST_CHECK(ms_stack_ok && ms_stack_ok->CheckStackSize());
    Test(ms_stack_limit, "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, 4 * count + 1, 1, {}, {}, 1 + count + 1);
    // But one more element on the stack during execution will make it fail. And we'd detect that.
    count++;
    ms_stack_limit = "and_b(older(1),a:" + ms_stack_limit + ")";
    const auto ms_stack_nok{miniscript::FromString(ms_stack_limit, tap_converter)};
    BOOST_CHECK(ms_stack_nok && !ms_stack_nok->CheckStackSize());
    Test(ms_stack_limit, "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG | TESTMODE_P2WSH_INVALID, 4 * count + 1, 1, {}, {}, 1 + count + 1);

    // Misc unit tests
    // A Script with a non minimal push is invalid
    constexpr auto nonminpush{"0000210232780000feff00ffffffffffff21ff005f00ae21ae00000000060602060406564c2102320000060900fe00005f00ae21ae00100000060606060606000000000000000000000000000000000000000000000000000000000000000000"_hex_u8};
    const CScript nonminpush_script(nonminpush.begin(), nonminpush.end());
    BOOST_CHECK(miniscript::FromScript(nonminpush_script, wsh_converter) == nullptr);
    BOOST_CHECK(miniscript::FromScript(nonminpush_script, tap_converter) == nullptr);
    // A non-minimal VERIFY (<key> CHECKSIG VERIFY 1)
    constexpr auto nonminverify{"2103a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7ac6951"_hex_u8};
    const CScript nonminverify_script(nonminverify.begin(), nonminverify.end());
    BOOST_CHECK(miniscript::FromScript(nonminverify_script, wsh_converter) == nullptr);
    BOOST_CHECK(miniscript::FromScript(nonminverify_script, tap_converter) == nullptr);
    // A threshold as large as the number of subs is valid.
    Test("thresh(2,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),altv:after(100))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b6300670164b16951686c935287", "20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac6b6300670164b16951686c935287", TESTMODE_VALID | TESTMODE_NEEDSIG | TESTMODE_NONMAL);
    // A threshold of 1 is valid.
    Test("thresh(1,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),sc:pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac7c2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac935187", "20d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac7c20fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac935187", TESTMODE_VALID | TESTMODE_NEEDSIG | TESTMODE_NONMAL);
    // A threshold with a k larger than the number of subs is invalid
    Test("thresh(3,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),sc:pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac7c2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac935187", "=", TESTMODE_INVALID);
    // A threshold with a k null is invalid
    Test("thresh(0,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),sc:pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))", "2103d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65ac7c2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac935187", "=", TESTMODE_INVALID);
    // For CHECKMULTISIG the OP cost is the number of keys, but the stack size is the number of sigs (+1)
    const auto ms_multi = miniscript::FromString("multi(1,03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65,03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556,0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)", wsh_converter);
    BOOST_CHECK(ms_multi);
    BOOST_CHECK_EQUAL(*ms_multi->GetOps(), 4); // 3 pubkeys + CMS
    BOOST_CHECK_EQUAL(*ms_multi->GetStackSize(), 2); // 1 sig + dummy elem
    // The 'd:' wrapper leaves on the stack what was DUP'ed at the beginning of its execution.
    // Since it contains an OP_IF just after on the same element, we can make sure that the element
    // in question must be OP_1 if OP_IF enforces that its argument must only be OP_1 or the empty
    // vector (since otherwise the execution would immediately fail). This is the MINIMALIF rule.
    // Unfortunately, this rule is consensus for Taproot but only policy for P2WSH. Therefore we can't
    // (for now) have 'd:' be 'u'. This tests we can't use a 'd:' wrapper for a thresh, which requires
    // its subs to all be 'u' (taken from https://github.com/rust-bitcoin/rust-miniscript/discussions/341).
    const auto ms_minimalif = miniscript::FromString("thresh(3,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),sc:pk_k(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556),sc:pk_k(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798),sdv:older(32))", wsh_converter);
    BOOST_CHECK(ms_minimalif && !ms_minimalif->IsValid());
    // A Miniscript with duplicate keys is not sane
    const auto ms_dup1 = miniscript::FromString("and_v(v:pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65))", wsh_converter);
    BOOST_CHECK(ms_dup1);
    BOOST_CHECK(!ms_dup1->IsSane() && !ms_dup1->CheckDuplicateKey());
    // Same with a disjunction, and different key nodes (pk and pkh)
    const auto ms_dup2 = miniscript::FromString("or_b(c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),ac:pk_h(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65))", wsh_converter);
    BOOST_CHECK(ms_dup2 && !ms_dup2->IsSane() && !ms_dup2->CheckDuplicateKey());
    // Same when the duplicates are leaves or a larger tree
    const auto ms_dup3 = miniscript::FromString("or_i(and_b(pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),s:pk(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556)),and_b(older(1),s:pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65)))", wsh_converter);
    BOOST_CHECK(ms_dup3 && !ms_dup3->IsSane() && !ms_dup3->CheckDuplicateKey());
    // Same when the duplicates are on different levels in the tree
    const auto ms_dup4 = miniscript::FromString("thresh(2,pkh(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),s:pk(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556),a:and_b(dv:older(1),s:pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65)))", wsh_converter);
    BOOST_CHECK(ms_dup4 && !ms_dup4->IsSane() && !ms_dup4->CheckDuplicateKey());
    // Sanity check the opposite is true, too. An otherwise sane Miniscript with no duplicate keys is sane.
    const auto ms_nondup = miniscript::FromString("pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65)", wsh_converter);
    BOOST_CHECK(ms_nondup && ms_nondup->CheckDuplicateKey() && ms_nondup->IsSane());
    // Test we find the first insane sub closer to be a leaf node. This fragment is insane for two reasons:
    // 1. It can be spent without a signature
    // 2. It contains timelock mixes
    // We'll report the timelock mix error, as it's "deeper" (closer to be a leaf node) than the "no 's' property"
    // error is.
    const auto ms_ins = miniscript::FromString("or_i(and_b(after(1),a:after(1000000000)),pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204))", wsh_converter);
    BOOST_CHECK(ms_ins && ms_ins->IsValid() && !ms_ins->IsSane());
    const auto insane_sub = ms_ins->FindInsaneSub();
    BOOST_CHECK(insane_sub && *insane_sub->ToString(wsh_converter) == "and_b(after(1),a:after(1000000000))");

    // Numbers can't be prefixed by a sign.
    BOOST_CHECK(!miniscript::FromString("after(-1)", wsh_converter));
    BOOST_CHECK(!miniscript::FromString("after(+1)", wsh_converter));
    BOOST_CHECK(!miniscript::FromString("thresh(-1,pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204))", wsh_converter));
    BOOST_CHECK(!miniscript::FromString("multi(+1,03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)", wsh_converter));

    // Timelock tests
    Test("after(100)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // only heightlock
    Test("after(1000000000)", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL); // only timelock
    Test("or_b(l:after(100),al:after(1000000000))", "?", "?", TESTMODE_VALID); // or_b(timelock, heighlock) valid
    Test("and_b(after(100),a:after(1000000000))", "?", "?", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_TIMELOCKMIX); // and_b(timelock, heighlock) invalid
    /* This is correctly detected as non-malleable but for the wrong reason. The type system assumes that branches 1 and 2
       can be spent together to create a non-malleble witness, but because of mixing of timelocks they cannot be spent together.
       But since exactly one of the two after's can be satisfied, the witness involving the key cannot be malleated.
    */
    Test("thresh(2,ltv:after(1000000000),altv:after(100),a:pk(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65))", "?", "?", TESTMODE_VALID | TESTMODE_TIMELOCKMIX | TESTMODE_NONMAL); // thresh with k = 2
    // This is actually non-malleable in practice, but we cannot detect it in type system. See above rationale
    Test("thresh(1,c:pk_k(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),altv:after(1000000000),altv:after(100))", "?", "?", TESTMODE_VALID); // thresh with k = 1

    g_testdata.reset();
}

BOOST_AUTO_TEST_SUITE_END()
