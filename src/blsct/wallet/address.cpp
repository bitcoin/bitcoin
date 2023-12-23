// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/address.h>

namespace blsct {
SubAddress::SubAddress(const PrivateKey& viewKey, const PublicKey& spendKey, const SubAddressIdentifier& subAddressId)
{
    if (!viewKey.IsValid() || !spendKey.IsValid()) {
        throw std::runtime_error(std::string(__func__) + ": no valid blsct keys");
    }

    HashWriter string{};

    string << std::vector<unsigned char>(subAddressHeader.begin(), subAddressHeader.end());
    string << viewKey;
    string << subAddressId.account;
    string << subAddressId.address;

    // m = Hs(a || i)
    // M = m*G
    // D = B + M
    // C = a*D
    MclScalar m{string.GetHash()};
    MclG1Point M = MclG1Point::GetBasePoint() * m;
    MclG1Point B = spendKey.GetG1Point();

    MclG1Point D = M + B;
    auto C = D * viewKey.GetScalar();
    pk = DoublePublicKey(C, D);
}

std::string SubAddress::GetString() const
{
    return EncodeDestination(pk);
}

CTxDestination SubAddress::GetDestination() const
{
    if (!IsValid())
        return CNoDestination();
    return pk;
}

bool SubAddress::IsValid() const
{
    return pk.IsValid();
}
} // namespace blsct
