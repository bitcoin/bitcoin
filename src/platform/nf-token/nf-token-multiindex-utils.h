#ifndef CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H
#define CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H

#include <boost/functional/hash.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include "pubkey.h"

std::size_t hash_value(const uint256 & val);
std::size_t hash_value(const CKeyID & val);

namespace Platform
{
    namespace bmx = boost::multi_index;

    namespace Tags
    {
        class BlockHash {};
        class RegTxHash {};
        class Height {};
        class ProtocolIdHeight{};
        class ProtocolId {};
        class TokenId {};
        class ProtocolIdTokenId {};
        class ProtocolIdOwnerId {};
        class OwnerId {};
        class AdminId {};
    }
}

#endif // CROWN_PLATFORM_NF_TOKEN_MULTIINDEX_UTILS_H
