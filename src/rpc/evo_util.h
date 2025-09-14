// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_EVO_UTIL_H
#define BITCOIN_RPC_EVO_UTIL_H

#include <netaddress.h>
#include <util/check.h>

#include <evo/dmn_types.h>
#include <evo/netinfo.h>

#include <cstdint>
#include <type_traits>

#include <univalue.h>

class CSimplifiedMNListEntry;

/** Process setting (legacy) Core network information field based on ProTx version */
template <typename ProTx>
void ProcessNetInfoCore(ProTx& ptx, const UniValue& input, const bool optional);

/** Process setting (legacy) Platform network information fields based on ProTx version */
template <typename ProTx>
void ProcessNetInfoPlatform(ProTx& ptx, const UniValue& input_p2p, const UniValue& input_http, const bool optional);

/** Reads network info reporting and appends data from legacy fields if applicable */
template <typename Obj>
UniValue GetNetInfoWithLegacyFields(const Obj& obj, const MnType& type)
{
    UniValue ret{CHECK_NONFATAL(obj.netInfo)->ToJson()};

    // Nothing to do here if not EvoNode, netInfo is empty or netInfo is capable of natively
    // storing Platform fields
    if (type != MnType::Evo || obj.netInfo->IsEmpty() || obj.netInfo->CanStorePlatform()) return ret;

    CNetAddr addr{obj.netInfo->GetPrimary()};
    ret.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_HTTPS).data(),
               ArrFromService(CService(addr, obj.platformHTTPPort)));

    if constexpr (!std::is_same_v<std::decay_t<Obj>, CSimplifiedMNListEntry>) {
        // CSimplifiedMNListEntry doesn't store platformP2PPort
        ret.pushKV(PurposeToString(NetInfoPurpose::PLATFORM_P2P).data(),
                   ArrFromService(CService(addr, obj.platformP2PPort)));
    }

    return ret;
}

#endif // BITCOIN_RPC_EVO_UTIL_H
