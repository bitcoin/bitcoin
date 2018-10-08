// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/crypto_gen.h>

#include <key.h>

#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Predicate.h>
#include <rapidcheck/gen/Container.h>

/** Generates 1 to 20 keys for OP_CHECKMULTISIG */
rc::Gen<std::vector<CKey>> MultisigKeys()
{
    return rc::gen::suchThat(rc::gen::arbitrary<std::vector<CKey>>(), [](const std::vector<CKey>& keys) {
        //TODO: Investigate why we can only allow 15 keys. Consensus rules
        // dictate we can up to 20 keys
        //https://github.com/bitcoin/bitcoin/blob/10bee0dd4f37eb6cb7a0f1d565fa0fecf8109c35/src/script/script.h#L29
        //needs to be <= 16 keys because this assertion fails
        //https://github.com/bitcoin/bitcoin/blob/10bee0dd4f37eb6cb7a0f1d565fa0fecf8109c35/src/script/script.h#L585
        //ProduceSignature() fails for p2sh(multisig) if there are >= 16 keys
        //this is why we are currently limited to the range >= 1 && <= 15
        return keys.size() >= 1 && keys.size() <= 15;
    });
};
