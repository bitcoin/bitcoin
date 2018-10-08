// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TEST_GEN_SCRIPT_GEN_H
#define BITCOIN_TEST_GEN_SCRIPT_GEN_H

#include <key.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Container.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gen/Select.h>
#include <script/script.h>

using SPKCKeyPair = std::pair<CScript, std::vector<CKey>>;

//non witness SPKs
rc::Gen<SPKCKeyPair> P2PKSPK();

rc::Gen<SPKCKeyPair> P2PKHSPK();

rc::Gen<SPKCKeyPair> MultisigSPK();

/** Generates a non-P2SH/P2WSH spk */
rc::Gen<SPKCKeyPair> RawSPK();

rc::Gen<SPKCKeyPair> P2SHSPK();

//witness spks

rc::Gen<SPKCKeyPair> P2WPKHSPK();

rc::Gen<SPKCKeyPair> P2WSHSPK();

namespace rc {
template <>
struct Arbitrary<CScript> {
    static Gen<CScript> arbitrary()
    {
        return gen::map(gen::arbitrary<std::vector<unsigned char>>(), [](std::vector<unsigned char> script) {
            return CScript(script.begin(), script.end());
        });
    };
};
} // namespace rc
#endif
