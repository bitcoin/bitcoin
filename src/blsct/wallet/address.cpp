// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/address.h>

namespace blsct {
SubAddress::SubAddress(const PrivateKey &viewKey, const PublicKey &spendKey, const SubAddressIdentifier &subAddressId)
{
    if(!viewKey.IsValid() || !spendKey.IsValid())
        return;

    CHashWriter string(SER_GETHASH, 0);

    string << std::vector<unsigned char>(subAddressHeader.begin(), subAddressHeader.end());
    string << viewKey;
    string << subAddressId.account;
    string << subAddressId.address;

    // m = Hs(a || i)
    // M = m*G
    // D = B + M
    // C = a*D
    MclScalar m{string.GetHash()};
    MclG1Point M, B;

    if (!PrivateKey(m).GetPublicKey().GetG1Point(M))
        return;

    if (!spendKey.GetG1Point(B))
        return;

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
}
