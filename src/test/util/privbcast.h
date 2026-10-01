// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_PRIVBCAST_H
#define BITCOIN_TEST_UTIL_PRIVBCAST_H

#include <key.h>
#include <net_transport.h>
#include <netaddress.h>
#include <netmessagemaker.h>
#include <primitives/transaction.h>
#include <privbcast/attempt.h>
#include <protocol.h>
#include <random.h>
#include <script/script.h>
#include <span.h>
#include <uint256.h>

#include <cstdint>
#include <string>
#include <utility>

/** A key source that draws each attempt's BIP324 key, entropy and garbage from rng, which must
 *  outlive it, so that the attempt's bytes are a function of rng. */
inline privbcast::KeySource KeysFrom(FastRandomContext& rng)
{
    return [&rng] {
        privbcast::TransportKeys keys;
        do {
            const uint256 secret{rng.rand256()};
            keys.key.Set(secret.begin(), secret.end(), /*fCompressedIn=*/true);
        } while (!keys.key.IsValid());
        keys.ellswift_entropy = rng.rand256();
        keys.garbage = rng.randbytes(rng.randrange(V2Transport::MAX_GARBAGE_LEN + 1));
        return keys;
    };
}

/** A BIP324 responder whose key, entropy and garbage come from rng, as KeysFrom draws them. */
inline V2Transport MakeResponder(FastRandomContext& rng)
{
    privbcast::TransportKeys keys{KeysFrom(rng)()};
    return V2Transport{NodeId{1}, /*initiating=*/false, keys.key, MakeByteSpan(keys.ellswift_entropy), std::move(keys.garbage)};
}

/** The transaction the tests announce. It has a witness, so that its txid and wtxid differ. */
inline CTransactionRef MakeTx()
{
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256{7}), 1});
    tx.vin[0].scriptWitness.stack.push_back({1, 2, 3});
    tx.vout.emplace_back(10'000, CScript{} << OP_TRUE);
    return MakeTransactionRef(std::move(tx));
}

/** A peer's VERSION, with null addresses. */
inline CSerializedNetMsg PeerVersion(int32_t version, uint64_t services, bool relay, const std::string& user_agent)
{
    return NetMsg::Make(NetMsgType::VERSION, version, services, int64_t{1'700'000'000},
                        uint64_t{NODE_NONE}, CNetAddr::V1(CService{}), services, CNetAddr::V1(CService{}),
                        uint64_t{0x0123456789abcdef}, user_agent, int32_t{900'000}, relay);
}

#endif // BITCOIN_TEST_UTIL_PRIVBCAST_H
