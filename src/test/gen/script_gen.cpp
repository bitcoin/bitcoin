// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/script_gen.h>

#include <base58.h>
#include <core_io.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gen/Predicate.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/gen/crypto_gen.h>

/** Generates a P2PK/CKey pair */
rc::Gen<SPKCKeyPair> P2PKSPK()
{
    return rc::gen::map(rc::gen::arbitrary<CKey>(), [](const CKey& key) {
        const CScript& s = GetScriptForRawPubKey(key.GetPubKey());
        std::vector<CKey> keys;
        keys.push_back(key);
        return std::make_pair(s, keys);
    });
}
/** Generates a P2PKH/CKey pair */
rc::Gen<SPKCKeyPair> P2PKHSPK()
{
    return rc::gen::map(rc::gen::arbitrary<CKey>(), [](const CKey& key) {
        PKHash id = PKHash(key.GetPubKey());
        std::vector<CKey> keys;
        keys.push_back(key);
        const CScript& s = GetScriptForDestination(id);
        return std::make_pair(s, keys);
    });
}

/** Generates a MultiSigSPK/CKey(s) pair */
rc::Gen<SPKCKeyPair> MultisigSPK()
{
    return rc::gen::mapcat(MultisigKeys(), [](const std::vector<CKey>& keys) {
        return rc::gen::map(rc::gen::inRange<int>(1, keys.size()), [keys](int required_sigs) {
            std::vector<CPubKey> pub_keys;
            for (unsigned int i = 0; i < keys.size(); i++) {
                pub_keys.push_back(keys[i].GetPubKey());
            }
            const CScript& s = GetScriptForMultisig(required_sigs, pub_keys);
            return std::make_pair(s, keys);
        });
    });
}

rc::Gen<SPKCKeyPair> RawSPK()
{
    return rc::gen::oneOf(P2PKSPK(), P2PKHSPK(), MultisigSPK(),
        P2WPKHSPK());
}

/** Generates a P2SHSPK/CKey(s) */
rc::Gen<SPKCKeyPair> P2SHSPK()
{
    return rc::gen::map(RawSPK(), [](const SPKCKeyPair& spk_keys) {
        const CScript& redeemScript = spk_keys.first;
        const std::vector<CKey>& keys = spk_keys.second;
        const CScript& p2sh = GetScriptForDestination(ScriptHash(redeemScript));
        return std::make_pair(redeemScript, keys);
    });
}

//witness SPKs

rc::Gen<SPKCKeyPair> P2WPKHSPK()
{
    rc::Gen<SPKCKeyPair> spks = rc::gen::oneOf(P2PKSPK(), P2PKHSPK());
    return rc::gen::map(spks, [](const SPKCKeyPair& spk_keys) {
        const CScript& p2pk = spk_keys.first;
        const std::vector<CKey>& keys = spk_keys.second;
        const CScript& wit_spk = GetScriptForWitness(p2pk);
        return std::make_pair(wit_spk, keys);
    });
}

rc::Gen<SPKCKeyPair> P2WSHSPK()
{
    return rc::gen::map(MultisigSPK(), [](const SPKCKeyPair& spk_keys) {
        const CScript& p2pk = spk_keys.first;
        const std::vector<CKey>& keys = spk_keys.second;
        const CScript& wit_spk = GetScriptForWitness(p2pk);
        return std::make_pair(wit_spk, keys);
    });
}
