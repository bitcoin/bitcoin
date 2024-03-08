// Copyright (c) 2024 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/helpers.h>

namespace blsct {
uint64_t CalculateViewTag(const MclG1Point& blindingKey, const MclScalar& viewKey)
{
    HashWriter hash{};
    hash << (blindingKey * viewKey);

    return (hash.GetHash().GetUint64(0) & 0xFFFF);
}

CKeyID CalculateHashId(const MclG1Point& blindingKey, const MclG1Point& spendingKey, const MclScalar& viewKey)
{
    auto t = blindingKey * viewKey;
    auto dh = MclG1Point::GetBasePoint() * t.GetHashWithSalt(0).Negate();
    auto D_prime = spendingKey + dh;

    return blsct::PublicKey(D_prime).GetID();
}

MclScalar CalculatePrivateSpendingKey(const MclG1Point& blindingKey, const MclScalar& viewKey, const MclScalar& spendingKey, const int64_t& account, const uint64_t& address)
{
    HashWriter string{};

    string << std::vector<unsigned char>(subAddressHeader.begin(), subAddressHeader.end());
    string << viewKey;
    string << account;
    string << address;

    MclG1Point t = blindingKey * viewKey;

    return t.GetHashWithSalt(0) + spendingKey + MclScalar(string.GetHash());
}

MclG1Point CalculateNonce(const MclG1Point& blindingKey, const MclScalar& viewKey)
{
    return blindingKey * viewKey;
}
} // namespace blsct
