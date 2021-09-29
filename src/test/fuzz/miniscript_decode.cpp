// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <hash.h>
#include <key.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>

#include <optional>


struct MockedSatisfier {
    typedef CPubKey Key;

    // Precomputed public keys, and a dummy signature for each of them.
    std::vector<Key> dummy_keys;
    std::map<CKeyID, Key> dummy_keys_map;
    std::map<Key, std::vector<unsigned char>> dummy_sigs;

    // Precomputed hashes of each kind.
    std::vector<std::vector<unsigned char>> sha256;
    std::vector<std::vector<unsigned char>> ripemd160;
    std::vector<std::vector<unsigned char>> hash256;
    std::vector<std::vector<unsigned char>> hash160;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> sha256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> ripemd160_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash256_preimages;
    std::map<std::vector<unsigned char>, std::vector<unsigned char>> hash160_preimages;

    //! Set the precomputed data.
    void Init() {
        unsigned char keydata[32] = {1};
        for (unsigned char i = 1; i <= 50; i++) {
            keydata[31] = i;
            CKey privkey;
            privkey.Set(keydata, keydata + 32, true);
            const Key pubkey = privkey.GetPubKey();

            dummy_keys.push_back(pubkey);
            dummy_keys_map.insert({pubkey.GetID(), pubkey});
            std::vector<unsigned char> sig;
            privkey.Sign(uint256S(""), sig);
            sig.push_back(1); // SIGHASH_ALL
            dummy_sigs.insert({pubkey, sig});

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

    // Conversion routines
    bool ToString(const Key& key, std::string& ret) const { ret = HexStr(key); return true; }
    const std::vector<unsigned char> ToPKBytes(const Key& key) const { return {key.begin(), key.end()}; }
    const std::vector<unsigned char> ToPKHBytes(const Key& key) const {
        const auto h = Hash160(key);
        return {h.begin(), h.end()};
    }
    template<typename I>
    bool FromString(I first, I last, Key& key) const {
        const auto bytes = ParseHex(std::string(first, last));
        key.Set(bytes.begin(), bytes.end());
        return key.IsValid();
    }
    template<typename I>
    bool FromPKBytes(I first, I last, CPubKey& key) const {
        key.Set(first, last);
        return key.IsValid();
    }
    template<typename I>
    bool FromPKHBytes(I first, I last, CPubKey& key) const {
        assert(last - first == 20);
        CKeyID keyid;
        std::copy(first, last, keyid.begin());
        const auto it = dummy_keys_map.find(keyid);
        if (it == dummy_keys_map.end()) return false;
        key = it->second;
        return true;
    }

    // Timelock challenges satisfaction. Make the value (deterministically) vary to explore different
    // paths.
    bool CheckAfter(uint32_t value) const { return value % 2; }
    bool CheckOlder(uint32_t value) const { return value % 2; }

    // Signature challenges fulfilled with a dummy signate, if it was one of our dummy keys.
    miniscript::Availability Sign(const CPubKey& key, std::vector<unsigned char>& sig) const {
        const auto it = dummy_sigs.find(key);
        if (it == dummy_sigs.end()) return miniscript::Availability::NO;
        sig = it->second;
        return miniscript::Availability::YES;
    }

    //! Lookup generalization for all the hash satisfactions below
    miniscript::Availability LookupHash(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage,
                                        const std::map<std::vector<unsigned char>, std::vector<unsigned char>>& map) const
    {
        const auto it = map.find(hash);
        if (it == map.end()) return miniscript::Availability::NO;
        preimage = it->second;
        return miniscript::Availability::YES;
    }
    miniscript::Availability SatSHA256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, sha256_preimages);
    }
    miniscript::Availability SatRIPEMD160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, ripemd160_preimages);
    }
    miniscript::Availability SatHASH256(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, hash256_preimages);
    }
    miniscript::Availability SatHASH160(const std::vector<unsigned char>& hash, std::vector<unsigned char>& preimage) const {
        return LookupHash(hash, preimage, hash160_preimages);
    }
};


MockedSatisfier SATISFIER;

// Since the mocked key converter replaces all the keys with a hardcoded one we can't just
// compare the two scripts. This asserts that the two scripts are equal except for the pushes.
void assertSameOps(const CScript& script_a, const CScript& script_b) {
    CScript::const_iterator it_a{script_a.begin()}, it_b{script_b.begin()};
    opcodetype op_a, op_b;

    while (script_a.GetOp(it_a, op_a)) {
        assert(script_b.GetOp(it_b, op_b));
        assert(op_a == op_b);
    }
}

void initialize_miniscript_decode() {
    ECC_Start();
    SATISFIER.Init();
}

FUZZ_TARGET_INIT(miniscript_decode, initialize_miniscript_decode)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CScript> script_opt = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (!script_opt) return;
    const CScript script{*script_opt};

    const auto ms = miniscript::FromScript(script, SATISFIER);
    if (ms) {
        // We can roundtrip it to its string representation.
        std::string ms_str;
        assert(ms->ToString(SATISFIER, ms_str));
        assert(*miniscript::FromScript(script, SATISFIER) == *ms);
        // The Script representation must roundtrip.
        const CScript ms_script = ms->ToScript(SATISFIER);
        assertSameOps(ms_script, script);
        // We can compute the costs for this script, and (maybe) produce a satisfaction
        std::vector<std::vector<unsigned char>> stack;
        ms->Satisfy(SATISFIER, stack, true);
        ms->Satisfy(SATISFIER, stack, false);
    }
}
