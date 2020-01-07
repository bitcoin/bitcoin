// Copyright (c) 2018 The Bitcoin Core developers
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
        return keys.size() >= 1 && keys.size() <= 15;
    });
};
