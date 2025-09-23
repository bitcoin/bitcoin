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

/** Returns platform port based on purpose and network info version */
template <bool is_p2p, typename Obj>
int32_t GetPlatformPort(const Obj& obj)
{
    // Currently, there is nothing that prevents PLATFORM_{HTTPS,P2P} to be registered to
    // an addr *different* from the primary addr for CORE_P2P. This breaks the assumptions
    // under which platform{HTTP,P2P}Port operate under (i.e. all three are hosted with the
    // same addr).
    //
    // TODO: Introduce restrictions that enforce this assumption *until* we remove legacy
    //       fields for good.
    //
    bool is_legacy{!(CHECK_NONFATAL(obj.netInfo)->CanStorePlatform())};
    if constexpr (is_p2p) {
        static_assert(!std::is_same_v<std::decay_t<Obj>, CSimplifiedMNListEntry>,
                      "CSimplifiedMNListEntry doesn't have platformP2PPort");
        if (is_legacy) {
            return obj.platformP2PPort;
        }
        if (obj.netInfo->IsEmpty()) {
            return -1; // Blank entry, nothing to report
        }
        CHECK_NONFATAL(obj.netInfo->HasEntries(NetInfoPurpose::PLATFORM_P2P));
        return obj.netInfo->GetEntries(NetInfoPurpose::PLATFORM_P2P)[0].GetPort();
    } else {
        if (is_legacy) {
            return obj.platformHTTPPort;
        }
        if (obj.netInfo->IsEmpty()) {
            return -1; // Blank entry, nothing to report
        }
        CHECK_NONFATAL(obj.netInfo->HasEntries(NetInfoPurpose::PLATFORM_HTTPS));
        return obj.netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS)[0].GetPort();
    }
}

#endif // BITCOIN_RPC_EVO_UTIL_H
