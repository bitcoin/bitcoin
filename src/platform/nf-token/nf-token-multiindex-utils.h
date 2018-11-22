#ifndef CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H
#define CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include "pubkey.h"

std::size_t hash_value(const uint256 & val)
{
    //TODO: place for possible optimization: boost::hash vs std::hash vs uint256::GetHash vs SipHashUint256
    return boost::hash_range(val.begin(), val.end());
}

std::size_t hash_value(const CKeyID & val)
{
    //TODO: place for possible optimization: boost::hash vs std::hash vs uint256::GetHash vs SipHashUint256
    return boost::hash_range(val.begin(), val.end());
}

namespace Platform
{
    namespace bmx = boost::multi_index;

    namespace Tags
    {
        class BlockHash {};
        class RegTxHash {};
        class ProtocolId {};
        class TokenId {};
        class ProtocolIdTokenId {};
        class ProtocolIdOwnerId {};
        class OwnerId {};
        class AdminId {};
    }
}

#endif // CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H
