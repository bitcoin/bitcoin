#include "nf-token-multiindex-utils.h"

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
