// Copyright (c) 2024 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/eip_2333/bls12_381_keygen.h>
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

SubAddress DeriveSubAddress(const PrivateKey& viewKey, const PublicKey& spendKey, const SubAddressIdentifier& subAddressId)
{
    return SubAddress(viewKey, spendKey, subAddressId);
}

MclScalar FromSeedToChildKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 130);
}

MclScalar FromChildToTransactionKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 0);
}

MclScalar FromChildToBlindingKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 1);
}

MclScalar FromChildToTokenKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 2);
}

MclScalar FromTransactionToViewKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 0);
}

MclScalar FromTransactionToSpendKey(const MclScalar& seed)
{
    return BLS12_381_KeyGen::derive_child_SK(seed, 1);
}

MclScalar GenRandomSeed()
{
    return BLS12_381_KeyGen::derive_master_SK(MclScalar::Rand(true).GetVch());
}
} // namespace blsct
